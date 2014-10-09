// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Concurrency/SpinMutex.h>
#include <Concurrency/StaticMutex.h>
#include <Concurrency/Thread.h>
#include <Config.h>

THREADING_BEGIN

static const std::size_t spinMutexCount = 16;
static Threading::StaticMutex mutexBackImp[spinMutexCount] =
{
    STATIC_MUTEX_INITIALIZER, STATIC_MUTEX_INITIALIZER, STATIC_MUTEX_INITIALIZER, STATIC_MUTEX_INITIALIZER,
    STATIC_MUTEX_INITIALIZER, STATIC_MUTEX_INITIALIZER, STATIC_MUTEX_INITIALIZER, STATIC_MUTEX_INITIALIZER,
    STATIC_MUTEX_INITIALIZER, STATIC_MUTEX_INITIALIZER, STATIC_MUTEX_INITIALIZER, STATIC_MUTEX_INITIALIZER,
    STATIC_MUTEX_INITIALIZER, STATIC_MUTEX_INITIALIZER, STATIC_MUTEX_INITIALIZER, STATIC_MUTEX_INITIALIZER
};

static Threading::StaticMutex* mutexBack = reinterpret_cast<Threading::StaticMutex*>(mutexBackImp);

UTIL_CONSTEXPR SpinMutex::SpinMutex(void* pmutex) UTIL_NOEXCEPT
    : m_pmutex(pmutex)
{
}

void
SpinMutex::Lock() UTIL_NOEXCEPT
{
    Threading::StaticMutex& lock = *static_cast<Threading::StaticMutex*>(m_pmutex);
    unsigned count = 0;
    while (!lock.TryLock())
    {
        if (++count > 16)
        {
            lock.Lock();
            break;
        }

        Threading::ThreadControl::Yield();
    }
}

void
SpinMutex::Unlock() UTIL_NOEXCEPT
{
    static_cast<Threading::StaticMutex*>(m_pmutex)->Unlock();
}

SpinMutex&
GetSpinMutex(const void* pmutex)
{
    static SpinMutex muts[spinMutexCount] =
    {
        &mutexBack[ 0], &mutexBack[ 1], &mutexBack[ 2], &mutexBack[ 3],
        &mutexBack[ 4], &mutexBack[ 5], &mutexBack[ 6], &mutexBack[ 7],
        &mutexBack[ 8], &mutexBack[ 9], &mutexBack[10], &mutexBack[11],
        &mutexBack[12], &mutexBack[13], &mutexBack[14], &mutexBack[15]
    };

    return muts[std::hash<const void*>()(pmutex) & (spinMutexCount - 1)];
}

THREADING_END