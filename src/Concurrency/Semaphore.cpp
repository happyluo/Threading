// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Concurrency/Semaphore.h>
#include <Concurrency/ThreadException.h>

#if defined(_WIN32)

UtilInternal::Semaphore::Semaphore(long initialCount)
{
    m_sem = CreateSemaphore(NULL, initialCount, 0x7fffffff, NULL);
    if (INVALID_HANDLE_VALUE == m_sem)
    {
        throw Util::ThreadSyscallException(__FILE__, __LINE__, GetLastError());
    }
}

UtilInternal::Semaphore::~Semaphore()
{
    CloseHandle(m_sem);
}

// P
void UtilInternal::Semaphore::Wait() const
{
    DWORD returnVal = WaitForSingleObject(m_sem, INFINITE);
    if (WAIT_OBJECT_0 != returnVal)
    {
        throw Util::ThreadSyscallException(__FILE__, __LINE__, GetLastError());
    }
}

// P
bool UtilInternal::Semaphore::TimedWait(const Util::Time& timeout) const
{
    Util::Int64 usTimeout = timeout.ToMilliSeconds();

    if (usTimeout < 0 || usTimeout > 0x7fffffff)
    {
        throw Util::InvalidTimeoutException(__FILE__, __LINE__, timeout);
    }

    DWORD returnVal = WaitForSingleObject(m_sem, static_cast<DWORD>(usTimeout));
    if (WAIT_TIMEOUT != returnVal && WAIT_OBJECT_0 != returnVal)
    {
        throw Util::ThreadSyscallException(__FILE__, __LINE__, GetLastError());
    }

    return (WAIT_OBJECT_0 == returnVal);
}

//V 
void UtilInternal::Semaphore::Post(int releaseCount) const
{
    int returnVal = ReleaseSemaphore(m_sem, releaseCount, 0);
    if (false == returnVal)
    {
        throw Util::ThreadSyscallException(__FILE__, __LINE__, GetLastError());
    }
}

#endif
