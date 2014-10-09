// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************


#ifndef CONCURRENCY_STATIC_MUTEX_H
#define CONCURRENCY_STATIC_MUTEX_H

#include <Config.h>
#include <Concurrency/Lock.h>
#include <Concurrency/ThreadException.h>

#if defined(_MSC_VER) && (_MSC_VER < 1300)
//
// Old versions of the Platform SDK don't have InterlockedCompareExchangePointer
//
#   ifndef InterlockedCompareExchangePointer
#      define InterlockedCompareExchangePointer(Destination, ExChange, Comperand) \
          InterlockedCompareExchange(Destination, ExChange, Comperand)
#   endif
#endif

THREADING_BEGIN

class Cond;

//
// Simple non-recursive Threading::Mutex implementation.
// These mutexes are POD types (see ISO C++ 9(4) and 8.5.1) and must be
// initialized statically using STATIC_MUTEX_INITIALIZER.
//

class THREADING_API StaticMutex
{
public:

    //
    // Lock & TryLock typedefs.
    //
    typedef LockT<StaticMutex> LockGuard;
    typedef TryLockT<StaticMutex> TryLockGuard;

    //
    // Note that lock/tryLock & unlock in general should not be used
    // directly. Instead use LockGuard & TryLockGuard.
    //
 
    void Lock() const;

    //
    // Returns true if the lock was acquired, and false otherwise.
    //
    bool TryLock() const;

    void Unlock() const;


#ifdef _WIN32
    mutable CRITICAL_SECTION* m_mutex;
#else
    mutable pthread_mutex_t m_mutex;
#endif



#if !defined(_MSC_VER) && !defined(__BCPLUSPLUS__)
// COMPILERBUG
// VC++ BC++ considers that aggregates should not have private members ...
// even if it's just functions.
private:
#endif

    //
    // LockState and the lock/unlock variations are for use by the
    // Condition variable implementation.
    //
#ifdef _WIN32
    struct LockState
    {
    };
#else
    struct LockState
    {
        pthread_mutex_t* m_pmutex;
    };
#endif

    void Unlock(LockState&) const;
    void Lock(LockState&) const;

#ifdef _WIN32
    bool Initialized() const;
    void Initialize() const;
#endif

#ifndef _MSC_VER
    friend class Cond;
#endif

};

#ifdef _WIN32
#   define STATIC_MUTEX_INITIALIZER { 0 }
#else
#   define STATIC_MUTEX_INITIALIZER { PTHREAD_MUTEX_INITIALIZER }
#endif

THREADING_END

#endif
