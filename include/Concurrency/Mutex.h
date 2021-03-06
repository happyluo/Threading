// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_MUTEX_H
#define CONCURRENCY_MUTEX_H

#include <Config.h>
#include <Concurrency/Lock.h>

#ifdef LANG_CPP11
#   include <mutex>
#endif

THREADING_BEGIN

//
// Simple non-recursive Threading::Mutex implementation.
//
class THREADING_API Mutex : public noncopyable
{
    friend class Cond;

public:
    //
    // Lock & TryLock typedefs.
    //
    typedef LockT<Mutex> LockGuard;            
    typedef TryLockT<Mutex> TryLockGuard;

    Mutex(void);
    Mutex(MutexProtocol);
    ~Mutex(void);

    void Lock()    const;

    bool TryLock() const;

    void Unlock() const;

    bool WillUnlock() const;

#ifdef LANG_CPP11
    operator const std::mutex&() const
    {
        return m_mutex;
    }
#endif

private:

    void init(MutexProtocol);

    // noncopyable
    //Mutex(const Mutex&);
    //void operator=(const Mutex&);

    //
    // LockState and the lock/unlock variations are for use by the
    // Condition variable implementation.
    //
#ifdef LANG_CPP11
    struct LockState
    {
        std::mutex* m_pmutex;
    };
#elif defined(_WIN32)
    struct LockState
    {
#  ifdef HAS_WIN32_CONDVAR
        CRITICAL_SECTION* m_pmutex;
#  endif 
    };
#else
    struct LockState
    {
        pthread_mutex_t* m_pmutex;
    };
#endif

    void unlock(LockState&) const;
    void lock(LockState&) const;

#ifdef LANG_CPP11
    typedef std::mutex mutex_type;
    mutable std::mutex m_mutex;
#elif defined(_WIN32)
    mutable CRITICAL_SECTION m_mutex;
#else
    mutable pthread_mutex_t m_mutex;
#endif
};


//
// For performance reasons the following functions are inlined.
//
inline Mutex::Mutex()
{
    init(PrioNone);
}

inline Mutex::Mutex(MutexProtocol protocol)
{
#ifdef _WIN32
    init(PrioNone);
#else
    init(protocol);
#endif
}

THREADING_END

#endif