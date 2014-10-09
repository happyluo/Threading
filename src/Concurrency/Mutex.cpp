// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo.xiaowei (at) hotmail.com>
//
// **********************************************************************

#include <Concurrency/Mutex.h>
#include <Concurrency/ThreadException.h>

THREADING_BEGIN

#if defined(LANG_CPP11)

void Mutex::init(MutexProtocol)
{
}

Mutex::~Mutex(void)
{
    m_mutex.~mutex();
}

void Mutex::Lock()    const
{
    try
    {
        m_mutex.lock();
    }
    catch(const std::system_error& ex)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, ex.code().value());
    }
}

bool Mutex::TryLock() const
{
    try
    {
        return m_mutex.try_lock();
    }
    catch(const std::system_error& ex)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, ex.code().value());
    }
}

void Mutex::Unlock() const
{    
    try
    {
        std::unique_lock<std::mutex> ul(m_mutex, std::adopt_lock);
        //ul.unlock();
    }
    catch(const std::system_error& ex)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, ex.code().value());
    }
}

// LockState and the lock/unlock variations are for use by the
// Condition variable implementation.
void Mutex::unlock(LockState& state) const
{
    state.m_pmutex = &m_mutex;
}

void Mutex::lock(LockState&) const
{
}

#elif defined(_WIN32)

void Mutex::init(MutexProtocol)
{
#  ifdef HAS_WIN32_CONDVAR //OS_WINRT
    InitializeCriticalSectionEx(&m_mutex, 0, 0);
#  else
    InitializeCriticalSection(&m_mutex);
#  endif
}

Mutex::~Mutex(void)
{
    DeleteCriticalSection(&m_mutex);
}

void Mutex::Lock()    const
{
    EnterCriticalSection(&m_mutex);
    if (1 < m_mutex.RecursionCount)
    {
        throw ThreadLockedException(__FILE__, __LINE__);
    }
}

bool Mutex::TryLock() const
{
    if (!TryEnterCriticalSection(&m_mutex))
    {
        return false;
    }

    if (m_mutex.RecursionCount > 1)
    {
        LeaveCriticalSection(&m_mutex);
        throw ThreadLockedException(__FILE__, __LINE__);
    }

    return true;
}

void Mutex::Unlock() const
{
    assert(1 == m_mutex.RecursionCount);
    LeaveCriticalSection(&m_mutex);
}

// LockState and the lock/unlock variations are for use by the
// Condition variable implementation.
#  ifdef HAS_WIN32_CONDVAR
void Mutex::unlock(LockState& state) const
{
    state.m_pmutex = &m_mutex;
}

void Mutex::lock(LockState&) const
{
}

#  else

void Mutex::unlock(LockState&) const
{
    LeaveCriticalSection(&m_mutex);
}

void Mutex::lock(LockState&) const
{
    EnterCriticalSection(&m_mutex);
}
#    endif

#else

void Mutex::init(MutexProtocol protocol)
{
    pthread_mutexattr_t mutexAttr;
    int returnVal = pthread_mutexattr_init(&mutexAttr);
    //assert(0 == returnVal);
    if (0 != returnVal)
    {
        pthread_mutexattr_destroy(&mutexAttr);
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }

    //
    // Enable mutex error checking in debug builds
    //
#ifndef NDEBUG
#  if defined(__linux) && !defined(__USE_UNIX98)
    returnVal = pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_ERRORCHECK_NP);
#  else
    returnVal = pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_ERRORCHECK);
#  endif
    //assert(0 == returnVal);
    if (0 != returnVal)
    {
        pthread_mutexattr_destroy(&mutexAttr);
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }
#endif

    //
    // If system has support for priority inheritance we set the protocol
    // attribute of the mutex
    //
#if defined(_POSIX_THREAD_PRIO_INHERIT) && _POSIX_THREAD_PRIO_INHERIT > 0
    if (PrioInherit == protocol)
    {
        returnVal = pthread_mutexattr_setprotocol(&mutexAttr, PTHREAD_PRIO_INHERIT);
        if (returnVal != 0)
        {
            pthread_mutexattr_destroy(&mutexAttr);
            throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
        }
    }
#endif

    returnVal = pthread_mutex_init(&m_mutex, &mutexAttr);
    //assert(0 == returnVal;
    if (0 != returnVal)
    {
        pthread_mutexattr_destroy(&mutexAttr);
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }

    returnVal = pthread_mutexattr_destroy(&mutexAttr);
    //assert(0 == returnVal;
    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }
}

Mutex::~Mutex(void)
{
    int returnVal = pthread_mutex_destroy(&m_mutex);
    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }
}

void Mutex::Lock()    const
{
    int returnVal = pthread_mutex_lock(&m_mutex);
    if (0 != returnVal)
    {
        if (EDEADLK == returnVal)
        {
            throw ThreadLockedException(__FILE__, __LINE__);
        }
        else
        {
            throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
        }
    }
}

bool Mutex::TryLock() const
{
    int returnVal = pthread_mutex_trylock(&m_mutex);
    if (0 != returnVal && EBUSY != returnVal)
    {
        if (EDEADLK == returnVal)
        {
            throw ThreadLockedException(__FILE__, __LINE__);
        }
        else
        {
            throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
        }
    }

    return (0 == returnVal);
}

void Mutex::Unlock() const
{
    int returnVal = pthread_mutex_unlock(&m_mutex);
    if (0 != returnVal)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }
}

// LockState and the lock/unlock variations are for use by the
// Condition variable implementation.
void Mutex::unlock(LockState& state) const
{
    state.m_pmutex = &m_mutex;
}

void Mutex::lock(LockState&) const
{
}

#endif

bool Mutex::WillUnlock() const
{
    return true;
}

THREADING_END