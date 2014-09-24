// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Concurrency/CountDownLatch.h>
#include <Concurrency/ThreadException.h>

#if defined(_WIN32)

Util::CountdownLatch::CountdownLatch(int count) : 
    m_count(count)
{
    if (m_count < 0)
    {
        throw Exception(__FILE__, __LINE__);
    }

#ifdef _WIN32
#   ifndef OS_WINRT
    m_event = CreateEvent(0, TRUE, FALSE, 0);    
#   else
    m_event = CreateEventExW(0, 0,  CREATE_EVENT_MANUAL_RESET, SEMAPHORE_ALL_ACCESS);
#   endif
    if (0 == m_event)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, GetLastError());
    }
}

Util::CountdownLatch::~CountdownLatch(void)
{
    CloseHandle(m_event);
}


void Util::CountdownLatch::CountDown() const
{
    if (InterlockedDecrement(&m_count) == 0)
    {
        if (TRUE != SetEvent(m_event))
        {
            throw ThreadSyscallException(__FILE__, __LINE__, GetLastError());
        }
    }
}

void Util::CountdownLatch::Await() const
{
    while (InterlockedExchangeAdd(&m_count, 0) > 0)
    {
        DWORD returnVal = WaitForSingleObject(m_event, INFINITE);
#   else
        DWORD rc = WaitForSingleObjectEx(m_event, INFINITE, false);
#   endif
        assert(WAIT_OBJECT_0 == returnVal || WAIT_FAILED == returnVal);

        if (WAIT_FAILED == returnVal)
        {
            throw ThreadSyscallException(__FILE__, __LINE__, GetLastError());
        }
    }
}

int Util::CountdownLatch::GetCount() const
{
    int result = InterlockedExchangeAdd(&m_count, 0);

    return result > 0 ? result : 0;
}

bool Util::CountdownLatch::Reset(int count)
{
    assert(0 >= InterlockedExchangeAdd(&m_count, 0));
    InterlockedExchange(&m_count, count);
    return ResetEvent(m_event);
}

#else

Util::CountdownLatch::CountdownLatch(int count) :
    m_count(count)
{
    int returnVal = pthread_mutex_init(&m_mutex, 0);
    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }

    returnVal = pthread_cond_init(&m_cond, 0);
    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }
}

Util::CountdownLatch::~CountdownLatch(void)
{
#  ifndef NDEBUG
    int returnVal = pthread_mutex_destroy(&m_mutex);
    assert (0 == returnVal)
    //if (0 != returnVal)
    //{
    //    throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    //}

    returnVal = pthread_cond_destroy(&m_cond);
    assert (0 == returnVal)
    //if (0 != returnVal)
    //{
    //    throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    //}
#  else
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
#  endif
}


void Util::CountdownLatch::CountDown() const
{
#if defined(__APPLE__)
    lock();

    if (m_count > 0 && --m_count == 0)
    {
        // On MacOS X we do the broadcast with the mutex held. This seems to be necessary to prevent the 
        // broadcast call to hang (spinning in an infinite loop).
        int returnVal = pthread_cond_broadcast(&m_cond);
        if (0 != returnVal)
        {
            pthread_mutex_unlock(&m_mutex);
            throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
        }
    }

    unlock();

#else

    bool broadcast = false;
    lock();

    if (m_count > 0 && --m_count == 0)
    {
        broadcast = true;
    }

    unlock();        

    if (broadcast)
    {
        int returnVal = pthread_cond_broadcast(&m_cond);
        if (0 != returnVal)
        {
            throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
        }
    }

#endif
}

void Util::CountdownLatch::Await() const
{
    lock();

    while (m_count > 0)
    {
        int returnVal = pthread_cond_wait(&m_cond, &m_mutex);
        if (0 != returnVal)
        {
            pthread_mutex_unlock(&m_mutex);
            throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
        }
    }

    unlock();
}

int Util::CountdownLatch::GetCount() const
{
    lock();
    int result = m_count;
    unlock();

    return result;
}

bool Util::CountdownLatch::Reset(int count)
{
    lock();
    assert(0 >= m_count);
    m_count = count;
    unlock();

    return true;
}

void Util::CountdownLatch::lock() const
{
    int returnVal = pthread_mutex_lock(&m_mutex);
    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }
}

void Util::CountdownLatch::unlock() const
{
    int returnVal = pthread_mutex_unlock(&m_mutex);
    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }
}

#endif