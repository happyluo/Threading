// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Concurrency/RecMutex.h>
#include <Concurrency/ThreadException.h>


#if defined(LANG_CPP11)

void Util::RecMutex::init(MutexProtocol)
{
}

Util::RecMutex::~RecMutex(void)
{
	m_mutex.~mutex();
}

void Util::RecMutex::Lock()	const
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

bool Util::RecMutex::TryLock() const
{
	return m_mutex.try_lock();
}

void Util::RecMutex::Unlock() const
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
void Util::RecMutex::unlock(LockState& state) const
{
	state.m_pmutex = &m_mutex;
}

// 恢复到Unlock(LockState& state)之前的状态
void Util::RecMutex::lock(LockState& state) const
{
}

bool Util::RecMutex::WillUnlock() const
{
	std::unique_lock<std::mutex> ul(m_mutex, std::adopt_lock);
	bool result = ul.owns_lock();
	ul.release();
	return result;
}

#else

Util::RecMutex::RecMutex() : m_count(0)
{
	init(PrioNone);
}

Util::RecMutex::RecMutex(MutexProtocol protocol) : m_count(0)
{
#ifdef _WIN32
	init(PrioNone);
#else
	init(protocol);
#endif
}

bool Util::RecMutex::WillUnlock() const
{
	return (1 == m_count);
}

#	if defined(_WIN32)

void Util::RecMutex::init(MutexProtocol)
{
#ifdef HAS_WIN32_CONDVAR	//OS_WINRT
	InitializeCriticalSectionEx(&m_mutex, 0, 0);
#else
	InitializeCriticalSection(&m_mutex);
#endif
}

Util::RecMutex::~RecMutex(void)
{
	assert(0 == m_count);
	DeleteCriticalSection(&m_mutex);
}

void Util::RecMutex::Lock()	const
{
	EnterCriticalSection(&m_mutex);
	if (++m_count > 1)
	{
		LeaveCriticalSection(&m_mutex);
	}
}

bool Util::RecMutex::TryLock() const
{
	if (!TryEnterCriticalSection(&m_mutex))
	{
		return false;
	}

	if (++m_count > 1)
	{
		LeaveCriticalSection(&m_mutex);
	}

	return true;
}

void Util::RecMutex::Unlock() const
{
	if (0 == --m_count)
	{
		LeaveCriticalSection(&m_mutex);
	}
}

// LockState and the lock/unlock variations are for use by the
// Condition variable implementation.
#		ifdef HAS_WIN32_CONDVAR
void Util::RecMutex::unlock(LockState& state) const
{
	state.m_pmutex = &m_mutex;
	state.m_count = m_count;
	m_count = 0;
}

void Util::RecMutex::lock(LockState& state) const
{
	m_count = state.m_count;
}

#		else

void Util::RecMutex::unlock(LockState& state) const
{
	state.m_count = m_count;
	m_count = 0;				// clear current mutex's lock count.
	LeaveCriticalSection(&m_mutex);		// release(unlock) current mutex.
}

// 恢复到Unlock(LockState& state)之前的状态
void Util::RecMutex::lock(LockState& state) const
{
	EnterCriticalSection(&m_mutex);		// 恢复锁状态
	m_count = state.m_count;				// 恢复循环加锁的次数
}
#		endif

#	else

//Util::RecMutex::RecMutex(void) : m_count(0)
//{
//	init(PrioNone);
//}
//
//Util::RecMutex::RecMutex(Util::MutexProtocol protocol) : m_count(0)
//{
//	init(protocol);
//}

void Util::RecMutex::init(Util::MutexProtocol protocol)
{
	pthread_mutexattr_t mutexAttr;
	int returnVal = pthread_mutexattr_init(&mutexAttr);
	//assert(0 == returnVal);
	if (0 != returnVal)
	{
		pthread_mutexattr_destroy(&mutexAttr);
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}

#if defined(__linux) && !defined(__USE_UNIX98)
	returnVal = pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE_NP);
#else
	returnVal = pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
#endif
	assert(0 == returnVal);
	if (0 != returnVal)
	{
		pthread_mutexattr_destroy(&mutexAttr);
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}

	//
	// If system has support for priority inheritance we set the protocol
	// attribute of the mutex
	//
#if defined(_POSIX_THREAD_PRIO_INHERIT) && _POSIX_THREAD_PRIO_INHERIT > 0
	if (PrioInherit == protocol)
	{
		returnVal = pthread_mutexattr_setprotocol(&mutexAttr, PTHREAD_PRIO_INHERIT);
		assert(0 == returnVal);
		if (returnVal != 0)
		{
			pthread_mutexattr_destroy(&mutexAttr);
			throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
		}
	}
#endif

	returnVal = pthread_mutex_init(&m_mutex, &mutexAttr);
	//assert(0 == returnVal);
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

Util::RecMutex::~RecMutex(void)
{
	assert(0 == m_count);
#ifndef NDEBUG
	int returnVal = pthread_mutex_destroy(&m_mutex);
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}
#else
    pthread_mutex_destroy(&m_mutex);
#endif
}

void Util::RecMutex::Lock()	const
{
	int returnVal = pthread_mutex_lock(&m_mutex);
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}

	if (++m_count > 1)
	{
		returnVal = pthread_mutex_unlock(&m_mutex);
		//assert(0 == returnVal);
		if (0 != returnVal)
		{
			throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
		}
	}
}

bool Util::RecMutex::TryLock() const
{
	int returnVal = pthread_mutex_trylock(&m_mutex);
	if (0 != returnVal)
	{
		if (EBUSY != returnVal)
		{
			throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
		}
	}
	else if (++m_count > 1)
	{
		returnVal = pthread_mutex_unlock(&m_mutex);
		//assert(0 == returnVal);
		if (0 != returnVal)
		{
			throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
		}
	}

	return (0 == returnVal);
}

void Util::RecMutex::Unlock() const
{
	if (0 == --m_count)
	{
#ifndef NDEBUG
		int returnVal = pthread_mutex_unlock(&m_mutex);
		//assert(0 == returnVal);
		if (0 != returnVal)
		{
			throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
		}
#else
        pthread_mutex_unlock(&m_mutex);
#endif
	}
}

// LockState and the lock/unlock variations are for use by the
// Condition variable implementation.
void Util::RecMutex::unlock(LockState& state) const
{
	state.m_count = m_count;
	m_count = 0;
	state.m_pmutex = &m_mutex;
}

void Util::RecMutex::lock(LockState& state) const
{
	m_count = state.m_count;
}

#	endif

#endif


