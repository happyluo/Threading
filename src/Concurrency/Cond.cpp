// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Concurrency/Cond.h>

#ifndef _WIN32
#    include <sys/time.h>
#endif

#if defined(LANG_CPP11)

Threading::Cond::Cond(void) 
try : 
# if defined(USING_COND_ANY)
    m_cond(std::condition_variable_any())
# else
    m_cond(std::condition_variable());
# endif
{
}  
catch(const std::system_error& ex)
{
    throw ThreadSyscallException(__FILE__, __LINE__, ex.code().value());
}

Threading::Cond::~Cond(void)
{
    
}

void Threading::Cond::Signal()
{
    m_cond.notify_one();
}

void Threading::Cond::Broadcast()
{
    m_cond.notify_all();
}

#elif defined(_WIN32)

#   ifdef HAS_WIN32_CONDVAR

Threading::Cond::Cond()
{
    InitializeConditionVariable(&m_cond);
}

Threading::Cond::~Cond()
{
}

void Threading::Cond::Signal()
{
    WakeConditionVariable(&m_cond);
}

void Threading::Cond::Broadcast()
{
    WakeAllConditionVariable(&m_cond);
}

#   else

Threading::Cond::Cond(void) :
    m_condstate(Threading::Cond::IDLE),
    m_blockednum(0),
    m_signaleddnum(0),
    m_gatesem(1)
{
}

Threading::Cond::~Cond(void)
{
}

void Threading::Cond::Signal()
{
    wake(false);
}

void Threading::Cond::Broadcast()
{
    wake(true);
}

void Threading::Cond::wake(bool broadcast)
{
    m_gatesem.Wait();

    Threading::Mutex::LockGuard syncSignal(m_signaledmutex);

    if (m_signaleddnum != 0)
    {
        m_blockednum -= m_signaleddnum;
        m_signaleddnum = 0;
    }

    if (0 < m_blockednum)
    {
        assert(IDLE == m_condstate);
        m_condstate = broadcast ? BROADCAST : SIGNAL;

        syncSignal.Release();
        m_worksem.Post();
    }
    else
    {
        syncSignal.Release();
        m_gatesem.Post();        // allow waitingToWait to execute.
    }

}

void Threading::Cond::doWait() const
{
    try
    {
        m_worksem.Wait();
        postWait(false);
    }
    catch (...)
    {
        postWait(true);
        throw;
    }
}

bool Threading::Cond::timedDowait(const Time& timeout) const
{
    try
    {
        bool returnVal = m_worksem.TimedWait(timeout);
        postWait(!returnVal);
        return returnVal;
    }
    catch (...)
    {
        postWait(true);
        throw;
    }
}

void Threading::Cond::waitingToWait() const
{
    m_gatesem.Wait();
    ++m_blockednum;
    m_gatesem.Post();
}

void Threading::Cond::postWait(bool timed_out_or_failed) const
{
    Threading::Mutex::LockGuard syncSignal(m_signaledmutex);

    ++m_signaleddnum;

    if (IDLE == m_condstate) 
    {
        assert(timed_out_or_failed);   
        return;
    }

    //
    // m_condstate == SIGNAL / BROADCAST
    // 
    if (timed_out_or_failed) 
    {
        if (m_signaleddnum == m_blockednum)    
        {
            m_condstate = IDLE;

            m_worksem.Wait();

            syncSignal.Release(); 
            m_gatesem.Post();
        }
    }
    else
    {
        if (SIGNAL == m_condstate || m_signaleddnum == m_blockednum)  
        {
            m_condstate = IDLE;  

            syncSignal.Release();
            m_gatesem.Post();        
        }
        else   
        {
            syncSignal.Release();
            m_worksem.Post(); 
        }
    }
}

#    endif // HAS_WIN32_CONDVAR

#else

Threading::Cond::Cond(void) 
{
    pthread_condattr_t condattr;

    int returnVal = pthread_condattr_init(&condattr);
    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }

#if ! defined(__hpux) && ! defined(__APPLE__)
    returnVal = pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC); 
    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }
#endif

    returnVal = pthread_cond_init(&m_cond, &condattr);
    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }

    returnVal = pthread_condattr_destroy(&condattr);
    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }
}

Threading::Cond::~Cond(void)
{
#ifndef NDEBUG
    int returnVal = pthread_cond_destroy(&m_cond);
    assert(0 == returnVal);
    //if (0 != returnVal)
    //{
    //    throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    //}
#else
    pthread_cond_destroy(&m_cond);
#endif
}

void Threading::Cond::Signal()
{
    int returnVal = pthread_cond_signal(&m_cond);
    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }
}

void Threading::Cond::Broadcast()
{
    int returnVal = pthread_cond_broadcast(&m_cond);
    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }
}

#endif