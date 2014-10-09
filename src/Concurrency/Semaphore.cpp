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

Threading::Semaphore::Semaphore(long initialCount)
{
    m_sem = CreateSemaphore(NULL, initialCount, 0x7fffffff, NULL);
    if (INVALID_HANDLE_VALUE == m_sem)
    {
        throw Threading::ThreadSyscallException(__FILE__, __LINE__, GetLastError());
    }
}

Threading::Semaphore::~Semaphore()
{
    CloseHandle(m_sem);
}

// P
void Threading::Semaphore::Wait() const
{
    DWORD returnVal = WaitForSingleObject(m_sem, INFINITE);
    if (WAIT_OBJECT_0 != returnVal)
    {
        throw Threading::ThreadSyscallException(__FILE__, __LINE__, GetLastError());
    }
}

// P
bool Threading::Semaphore::TimedWait(const Threading::Time& timeout) const
{
    Threading::Int64 usTimeout = timeout.ToMilliSeconds();

    if (usTimeout < 0 || usTimeout > 0x7fffffff)
    {
        throw Threading::InvalidTimeoutException(__FILE__, __LINE__, timeout);
    }

    DWORD returnVal = WaitForSingleObject(m_sem, static_cast<DWORD>(usTimeout));
    if (WAIT_TIMEOUT != returnVal && WAIT_OBJECT_0 != returnVal)
    {
        throw Threading::ThreadSyscallException(__FILE__, __LINE__, GetLastError());
    }

    return (WAIT_OBJECT_0 == returnVal);
}

//V 
void Threading::Semaphore::Post(int releaseCount) const
{
    int returnVal = ReleaseSemaphore(m_sem, releaseCount, 0);
    if (false == returnVal)
    {
        throw Threading::ThreadSyscallException(__FILE__, __LINE__, GetLastError());
    }
}

#endif
