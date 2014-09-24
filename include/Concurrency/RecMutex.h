// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_RECMUTEX_H
#define CONCURRENCY_RECMUTEX_H

#include <Concurrency/Config.h>
#include <Concurrency/Lock.h>

#ifdef LANG_CPP11
#   include <mutex>
#endif

namespace Util
{
    
class CONCURRENCY_API RecMutex : public Base::noncopyable
{
    friend class Cond;

public:
    //
    // Lock & TryLock typedefs.
    //
    typedef LockT<RecMutex> LockGuard;
    typedef TryLockT<RecMutex> TryLockGuard;

    RecMutex(void);
    RecMutex(MutexProtocol);
    ~RecMutex(void);

    void Lock()    const;

    bool TryLock() const;

    void Unlock() const;

    bool WillUnlock() const;

#ifdef LANG_CPP11
    operator const std::recursive_mutex&() const
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
        std::recursive_mutex* m_pmutex;
    };
#elif defined(_WIN32)
    struct LockState
    {
#   ifdef HAS_WIN32_CONDVAR
        CRITICAL_SECTION* m_pmutex;
#   endif
        int m_count;
    };
#else
    struct LockState
    {
        pthread_mutex_t* m_pmutex;
        int m_count;
    };
#endif

    /// 将当前Mutex对象的锁的控制权转交给LockState对象
    /// 将当前Mutex的状态存储到LockState对象，并清除其锁状态，使此互斥量对其他线程可用
    void unlock(LockState&) const;

    /// 从LockState对象中恢复锁的状态到当前Mutex对象
    void lock(LockState&) const;

#ifdef LANG_CPP11
    typedef std::recursive_mutex mutex_type;
    mutable std::recursive_mutex m_mutex;
#elif defined(_WIN32)
    mutable CRITICAL_SECTION m_mutex;
    // 记录互斥量被(拥有拥有此互斥量的线程)循环加锁的次数
    mutable int m_count;
#else
    mutable pthread_mutex_t m_mutex;
    // 记录互斥量被(拥有拥有此互斥量的线程)循环加锁的次数
    mutable int m_count;
#endif
};

}

#endif