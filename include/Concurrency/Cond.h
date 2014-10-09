// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_COND_H
#define CONCURRENCY_COND_H

#include <Config.h>
#include <Concurrency/ThreadException.h>
#include <Concurrency/Semaphore.h>

#if defined(LANG_CPP11)
#    include <condition_variable>
#    include <chrono>
#elif defined(_WIN32) && !defined(HAS_WIN32_CONDVAR)
#    include <Concurrency/Mutex.h>
#endif


THREADING_BEGIN

//
// Forward declaration (for friend declarations).
//
template <class T> class Monitor;
class RecMutex;
class Mutex;

class THREADING_API Cond : public noncopyable
{
    friend class Monitor<Threading::Mutex>;
    friend class Monitor<RecMutex>;

public:
    Cond(void);
    ~Cond(void);

    void Signal();

    void Broadcast();

    template <typename L> 
    inline void Wait(const L& lock) const
    {
        if (!lock.Acquired())
        {
            throw ThreadLockedException(__FILE__, __LINE__);
        }

        waitImpl(lock.m_mutex);
    }

    template <typename L, typename Predicate>
    inline void Wait(const L& lck, Predicate pred) const
    {
        while (!pred())
        {
            Wait(lck);
        }
    }

    template <typename L>
    inline bool TimedWait(const L& lock, const Time& timeout) const
    {
        if (!lock.Acquired())
        {
            throw ThreadLockedException(__FILE__, __LINE__);
        }
        
        return timedWaitImpl(lock.m_mutex, timeout);
    }

private:
    template <typename M>
    inline void waitImpl(const M& mutex) const;
    
    template <typename M>
    inline bool  timedWaitImpl(const M& mutex, const Time& timeout) const;

#if defined(LANG_CPP11)            // cpp11
#  if defined(USING_COND_ANY)
    mutable std::condition_variable_any m_cond;
#  else
    mutable std::condition_variable m_cond;
#  endif
#elif defined(_WIN32)            // win32
#  ifdef HAS_WIN32_CONDVAR            // WinRT & Visual Studio 2012
    mutable CONDITION_VARIABLE m_cond;   
#  else                            // Windows XP or Windows Server 2003.
    void wake(bool broadcast);
    void doWait() const;
    bool timedDowait(const Time& timeout) const;
    /// ���ȴ��̼߳�����������
    void waitingToWait() const;
    
    /// �ȴ��߳̽��� postWait ʱ��������ڸ��������߳��豻����(BROADCAST), 
    /// �ź���m_worksem��V(Post)�������ٴα����ã��Ի��������ȴ��̡߳�����������߳��豻���
    /// ���ź���m_gatesem�ᱻ�ͷţ��Ӷ�����signaling��waiting to wait�̼߳���ִ��
    void postWait(bool timed_out_or_failed) const;

    enum State
    {
        IDLE, SIGNAL, BROADCAST
    };

    mutable State    m_condstate;

    /// ��¼�������̵߳���Ŀ
    mutable int    m_blockednum;

    /// ��¼���ŵ��̵߳���Ŀ
    mutable int    m_signaleddnum;

    /// �� m_signaleddnum ����ͬ���Ļ�������ͬһʱ��ֻ��һ���ȴ��߳����š�
    /// �ڹ㲥(Broadcast)������Ƕ���ȴ��߳������������
    Threading::Mutex            m_signaledmutex;

    /// ��m_blockednum����ͬ�����ź������������߳̽���ȴ�״̬
    /// �����߳����ڽ�������״̬ʱ�����ź����Ὣ������Ҫ����signal �� wait ״̬���߳�������
    /// ֱ�����������߳�(signaled threads)�ָ����������ͷ�m_gatesem�ź���
    Threading::Semaphore        m_gatesem;
     
    /// ���ڿ����߳����������ŵ��ź������������߳��ô��ź����ȴ�����������Ϊ����(be signaled)����m_worksem.Post()������
    Threading::Semaphore        m_worksem;

#  endif

#else                            // Linux like sys.
    mutable pthread_cond_t m_cond;
#endif

};

#if defined(LANG_CPP11)

template <typename M>
inline void Cond::waitImpl(const M& mutex) const
{
    typedef typename M::LockState LockState;
    LockState state;
    mutex.unlock(state);

    try
    {
# if defined(USING_COND_ANY)
        m_cond.wait(*state.m_pmutex);
# else
        std::unique_lock<std::mutex> ul(*state.m_pmutex, std::adopt_lock);
        m_cond.wait(ul);
        ul.release();
# endif
        
        mutex.lock(state);
    }
    catch(const std::system_error& ex)
    {
        mutex.lock(state);
        throw ThreadSyscallException(__FILE__, __LINE__, ex.code().value());
    }
}

template <typename M>
inline bool  Cond::timedWaitImpl(const M& mutex, const Time& timeout) const
{
    if (timeout < Time::MicroSeconds(0))
    {
        throw InvalidTimeoutException(__FILE__, __LINE__, timeout);
    }

    typedef typename M::LockState LockState;
    LockState state;
    mutex.unlock(state);

    try
    {
# if defined(USING_COND_ANY)
        std::cv_status result = 
            m_cond.wait_for (*state.m_pmutex, std::chrono::microseconds(timeout.ToMicroSeconds()));
# else
        std::unique_lock<std::mutex> ul(*state.m_pmutex, std::adopt_lock);
        std::cv_status result = 
            m_cond.wait_for (ul, std::chrono::microseconds(timeout.ToMicroSeconds()));
        ul.release();
# endif
        
        mutex.lock(state);

        return std::cv_status::no_timeout == result;
    }
    catch(const std::system_error& ex)
    {
        mutex.lock(state);
        throw ThreadSyscallException(__FILE__, __LINE__, ex.code().value());
    }
}

#elif defined(_WIN32)

# ifdef HAS_WIN32_CONDVAR

template <typename M> inline void
Cond::waitImpl(const M& mutex) const
{
    typedef typename M::LockState LockState;

    LockState state;
    mutex.unlock(state);
    BOOL ok = SleepConditionVariableCS(&m_cond, state.m_pmutex, INFINITE);  
    mutex.lock(state);

    if (!ok)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, GetLastError());
    }
}

template <typename M> inline bool
Cond::timedWaitImpl(const M& mutex, const Time& timeout) const
{
    Threading::Int64 msTimeout = timeout.ToMilliSeconds();
    if (msTimeout < 0 || msTimeout > 0x7FFFFFFF)
    {
        throw Threading::InvalidTimeoutException(__FILE__, __LINE__, timeout);
    } 

    typedef typename M::LockState LockState;

    LockState state;
    mutex.unlock(state);
    BOOL ok = SleepConditionVariableCS(&m_cond, state.m_pmutex, static_cast<DWORD>(msTimeout));  
    mutex.lock(state);

    if (!ok)
    {
        DWORD err = GetLastError();

        if (err != ERROR_TIMEOUT)
        {
            throw ThreadSyscallException(__FILE__, __LINE__, err);
        }
        return false;
    }
    return true;
}

# else

template <typename M>
inline void Cond::waitImpl(const M& mutex) const
{
    waitingToWait();

    typedef typename M::LockState LockState;
    LockState state;
    mutex.unlock(state); 

    try
    {
        doWait();        
        mutex.lock(state);
    }
    catch(...)
    {
        mutex.lock(state);
        throw;
    }
}

template <typename M>
inline bool  Cond::timedWaitImpl(const M& mutex, const Time& timeout) const
{
    if (timeout < Time::MicroSeconds(0))
    {
        throw InvalidTimeoutException(__FILE__, __LINE__, timeout);
    }

    waitingToWait();

    typedef typename M::LockState LockState;
    LockState state;
    mutex.unlock(state);

    try
    {
        bool result = timedDowait(timeout);
        mutex.lock(state);
        return result;
    }
    catch (...)
    {
        mutex.lock(state);
        throw;
    }
}

# endif

#else

template <typename M>
inline void Cond::waitImpl(const M& mutex) const
{
    typedef typename M::LockState LockState;
    LockState state;
    mutex.unlock(state);        
    int returnVal = pthread_cond_wait(&m_cond, state.m_pmutex);
    mutex.lock(state);

    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }
}

template <typename M>
inline bool  Cond::timedWaitImpl(const M& mutex, const Time& timeout) const
{
    if (timeout < Time::MicroSeconds(0))
    {
        throw InvalidTimeoutException(__FILE__, __LINE__, timeout);
    }

    typedef typename M::LockState LockState;
    LockState state;
    mutex.unlock(state);    

#  ifdef __APPLE__
    //
    // The monotonic time is based on mach_absolute_time and pthread
    // condition variables require time from gettimeofday  so we get
    // the realtime time.
    //
    timeval tv = Time::Now(Time::Realtime) + timeout;
#  else
    timeval tv = Time::Now(Time::Monotonic) + timeout;
#  endif

    timespec ts;
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;

    int returnVal = pthread_cond_timedwait(&m_cond, state.m_pmutex, &ts);
    mutex.lock(state);

    if (0 != returnVal)
    {
        if (ETIMEDOUT != returnVal)
        {
            throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
        }
        
        return false;
    }

    return true;
}

#endif

THREADING_END

#endif
