// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifdef _MSC_VER
#   pragma warning( disable : 4996 )
#endif

#include <Concurrency/StaticMutex.h>
#include <Concurrency/ThreadException.h>


#ifdef _WIN32
#   include <list>
#   include <algorithm>

using namespace std;

namespace {
    
static CRITICAL_SECTION sCriticalSection;

class Init
{
public:

    Init();
};

static Init _init;

Init::Init()
{
    InitializeCriticalSection(&sCriticalSection);
}

}

void Threading::StaticMutex::Initialize() const
{
    //
    // Double-check locking. 
    //
    EnterCriticalSection(&sCriticalSection);

    //
    // The second check
    //
    if (m_mutex == 0)
    {
        CRITICAL_SECTION* newMutex = new CRITICAL_SECTION;
        InitializeCriticalSection(newMutex);

        //
        // m_mutex is written after the new initialized CRITICAL_SECTION/Threading::Mutex
        //
        void* oldVal = InterlockedCompareExchangePointer(reinterpret_cast<void**>(&m_mutex), newMutex, 0);
        assert(oldVal == 0);

    }
    LeaveCriticalSection(&sCriticalSection);
}

bool 
Threading::StaticMutex::Initialized() const
{
    void* tmp = m_mutex;
    return InterlockedCompareExchangePointer(reinterpret_cast<void**>(&tmp), 0, 0) != 0;
}

void
Threading::StaticMutex::Lock() const
{
    if (!Initialized())
    {
        Initialize();
    }
    EnterCriticalSection(m_mutex);
    assert(m_mutex->RecursionCount == 1);
}

bool
Threading::StaticMutex::TryLock() const
{
    if (!Initialized())
    {
        Initialize();
    }
    if (!TryEnterCriticalSection(m_mutex))
    {
        return false;
    }
    if (m_mutex->RecursionCount > 1)
    {
        LeaveCriticalSection(m_mutex);
        throw ThreadLockedException(__FILE__, __LINE__);
    }
    return true;
}

void
Threading::StaticMutex::Unlock() const
{
    assert(m_mutex != 0);
    assert(m_mutex->RecursionCount == 1);
    LeaveCriticalSection(m_mutex);
}

void
Threading::StaticMutex::Unlock(LockState&) const
{
    assert(m_mutex != 0);
    assert(m_mutex->RecursionCount == 1);
    LeaveCriticalSection(m_mutex);
}

void
Threading::StaticMutex::Lock(LockState&) const
{
    if (!Initialized())
    {
        Initialize();
    }
    EnterCriticalSection(m_mutex);
}

#else

void
Threading::StaticMutex::Lock() const
{
    int rc = pthread_mutex_lock(&m_mutex);
    if (rc != 0)
    {
        if (rc == EDEADLK)
        {
            throw ThreadLockedException(__FILE__, __LINE__);
        }
        else
        {
            throw ThreadSyscallException(__FILE__, __LINE__, rc);
        }
    }
}

bool
Threading::StaticMutex::TryLock() const
{
    int rc = pthread_mutex_trylock(&m_mutex);
    if (rc != 0 && rc != EBUSY)
    {
        if (rc == EDEADLK)
        {
            throw ThreadLockedException(__FILE__, __LINE__);
        }
        else
        {
            throw ThreadSyscallException(__FILE__, __LINE__, rc);
        }
    }
    return (rc == 0);
}

void
Threading::StaticMutex::Unlock() const
{
    int rc = pthread_mutex_unlock(&m_mutex);
    if (rc != 0)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, rc);
    }
}

void
Threading::StaticMutex::Unlock(LockState& state) const
{
    state.m_pmutex = &m_mutex;
}

void
Threading::StaticMutex::Lock(LockState&) const
{
}

#endif
