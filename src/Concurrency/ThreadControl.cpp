// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Concurrency/ThreadControl.h>
#include <Concurrency/ThreadException.h>

#ifdef LANG_CPP11

Threading::ThreadControl::ThreadControl() :
    m_id(this_thread::get_id())
{
}

Threading::ThreadControl::ThreadControl(const shared_ptr<thread>& thread) :
    m_thread(thread), 
    m_id(_thread->get_id())
{
}

bool
Threading::ThreadControl::operator ==(const ThreadControl& rhs) const
{
    return Id() == rhs.Id();
}

bool
Threading::ThreadControl::operator !=(const ThreadControl& rhs) const
{
    return Id() != rhs.Id();
}

void
Threading::ThreadControl::Join()
{
    if (!m_thread)
    {
        throw BadThreadControlException(__FILE__, __LINE__);
    }

    try
    {
        m_thread->join();
    }
    catch(const system_error& ex)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, ex.code().value());
    }
}

void
Threading::ThreadControl::Detach()
{
    if (!m_thread)
    {
        throw BadThreadControlException(__FILE__, __LINE__);
    }

    try
    {
        m_thread->detach();
    }
    catch(const system_error& ex)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, ex.code().value());
    }
}

Threading::ThreadControl::ID
Threading::ThreadControl::Id() const
{
    return m_id;
}

void
Threading::ThreadControl::Sleep(const Time& timeout)
{
    this_thread::sleep_for (chrono::microseconds(timeout.ToMicroSeconds()));
}

void
Threading::ThreadControl::Yield()
{
    this_thread::yield();
}

#elif defined(_WIN32)

Threading::ThreadControl::ThreadControl(void) :
    m_thread(0),
    m_id(GetCurrentThreadId())
{
}

Threading::ThreadControl::ThreadControl(HANDLE thread, Threading::ThreadControl::ID id) :
    m_thread(thread), m_id(id)
{
}

bool Threading::ThreadControl::operator ==(const ThreadControl& rhs) const
{
    return m_id == rhs.m_id;    
}

bool Threading::ThreadControl::operator !=(const ThreadControl& rhs) const
{
    return m_id != rhs.m_id;    
}

void Threading::ThreadControl::Join()
{
    if (0 == m_thread)
    {
        throw BadThreadControlException(__FILE__, __LINE__);
    }

    DWORD result = WaitForSingleObjectEx(m_thread, INFINITE, true);
    if (WAIT_OBJECT_0 != result)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, GetLastError());
    }

    Detach();
}

void Threading::ThreadControl::Detach()
{
    if (0 == m_thread)
    {
        throw BadThreadControlException(__FILE__, __LINE__);
    }

    if (FALSE == CloseHandle(m_thread))
    {
        throw ThreadSyscallException(__FILE__, __LINE__, GetLastError());
    }
}

Threading::ThreadControl::ID Threading::ThreadControl::Id() const
{
    return m_id;
}

void Threading::ThreadControl::Sleep(const Time& timeout)
{
    //::Sleep(static_cast<DWORD>(timeout.ToMilliSeconds()));

    Time end = Time::Now() + timeout;
    Time now;
    while ((now = Time::Now()) < end)
    {
        ::Sleep(static_cast<DWORD>((end - now).ToMilliSeconds()));
    }
}

void Threading::ThreadControl::Yield()
{
    ::Sleep(0);
}

#else

Threading::ThreadControl::ThreadControl(void) :
    m_thread(pthread_self()),
    m_detachable(false)
{
}

Threading::ThreadControl::ThreadControl(pthread_t thread) :
    m_thread(thread),
    m_detachable(true)
{

}

bool Threading::ThreadControl::operator ==(const ThreadControl& rhs) const
{
    return 0 != pthread_equal(m_thread, rhs.m_thread);
}

bool Threading::ThreadControl::operator !=(const ThreadControl& rhs) const
{
    return !operator ==(rhs);
}

void Threading::ThreadControl::Join()
{
    if (!m_detachable)
    {
        throw BadThreadControlException(__FILE__, __LINE__);
    }

    void *pignore = 0;
    int result = pthread_join(m_thread, &pignore);
    if (0 != result)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, result);
    }
}

void Threading::ThreadControl::Detach()
{
    if (!m_detachable)
    {
        throw BadThreadControlException(__FILE__, __LINE__);
    }

    int result = pthread_detach(m_thread);
    if (0 != result)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, result);
    }
}

Threading::ThreadControl::ID Threading::ThreadControl::Id() const
{
    return m_thread;
}

void Threading::ThreadControl::Sleep(const Time& timeout)
{
    struct timeval tv = timeout;
    struct timespec tosleep, remaining;
    tosleep.tv_sec = tv.tv_sec;
    tosleep.tv_nsec = tv.tv_usec * 1000L;

    while (nanosleep(&tosleep, &remaining) == -1 && errno == EINTR) 
    {
        tosleep = remaining;  
    }
}

void Threading::ThreadControl::Yield()
{
    sched_yield();
}

#endif
