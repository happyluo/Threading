// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

//
// We disable deprecation warning here, to allow clean compilation of
// of deprecated methods.
//
#ifdef _MSC_VER
#   pragma warning( disable : 4996 )
#endif

#include <Concurrency/StaticMutex.h>
#include <Concurrency/ThreadException.h>

#ifdef _WIN32
#   include <list>
#   include <algorithm>

using namespace std;

static CRITICAL_SECTION sCriticalSection;

//
// Although apparently not documented by Microsoft, static objects are
// initialized before DllMain/DLL_PROCESS_ATTACH and finalized after
// DllMain/DLL_PROCESS_DETACH ... However, note that after the DLL is
// detached the allocated StaticMutexes may still be accessed. See
// http://blogs.msdn.com/larryosterman/archive/2004/06/10/152794.aspx
// for some details. This means that there is no convenient place to
// cleanup the globally allocated static mutexes.
//

namespace Util
{

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

void Util::StaticMutex::Initialize() const
{
    //
    // Yes, a double-check locking. It should be safe since we use memory barriers
    // (through InterlockedCompareExchangePointer) in both reader and writer threads
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
        // m_mutex is written after the new initialized CRITICAL_SECTION/Util::Mutex
        //
        void* oldVal = InterlockedCompareExchangePointer(reinterpret_cast<void**>(&m_mutex), newMutex, 0);
        assert(oldVal == 0);

    }
    LeaveCriticalSection(&sCriticalSection);
}

bool 
Util::StaticMutex::Initialized() const
{
    //
    // Read mutex and then inserts a memory barrier to ensure we can't 
    // see tmp != 0 before we see the initialized object
    //
    void* tmp = m_mutex;
    return InterlockedCompareExchangePointer(reinterpret_cast<void**>(&tmp), 0, 0) != 0;
}

void
Util::StaticMutex::Lock() const
{
    if (!Initialized())
    {
        Initialize();
    }
    EnterCriticalSection(m_mutex);
    assert(m_mutex->RecursionCount == 1);
}

bool
Util::StaticMutex::TryLock() const
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
Util::StaticMutex::Unlock() const
{
    assert(m_mutex != 0);
    assert(m_mutex->RecursionCount == 1);
    LeaveCriticalSection(m_mutex);
}

void
Util::StaticMutex::Unlock(LockState&) const
{
    assert(m_mutex != 0);
    assert(m_mutex->RecursionCount == 1);
    LeaveCriticalSection(m_mutex);
}

void
Util::StaticMutex::Lock(LockState&) const
{
    if (!Initialized())
    {
        Initialize();
    }
    EnterCriticalSection(m_mutex);
}

#else

void
Util::StaticMutex::Lock() const
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
Util::StaticMutex::TryLock() const
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
Util::StaticMutex::Unlock() const
{
    int rc = pthread_mutex_unlock(&m_mutex);
    if (rc != 0)
    {
        throw ThreadSyscallException(__FILE__, __LINE__, rc);
    }
}

void
Util::StaticMutex::unlock(LockState& state) const
{
    state.mutex = &m_mutex;
}

void
Util::StaticMutex::lock(LockState&) const
{
}

#endif