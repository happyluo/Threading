// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************
#include <iostream>


#include <Concurrency/Latch.h>

#include <Concurrency/Mutex.h>
#include <Concurrency/Cond.h>

CONCURRENCY_BEGIN


Latch::Latch(int count)
: m_count(count)
{
#ifdef LANG_CPP11
	std::atomic_init(&m_waiting, 0);
#else
	m_waiting = 0;
#endif
}

Latch::~Latch()
{
	while (m_waiting > 0) 
	{
		// Don't destroy this object if threads have not yet exited Wait(). This can
		// occur when a thread calls count_down() followed by the destructor - the
		// waiting threads may be scheduled to wake up, but have not yet have exited.
		//
		// NOTE - on pthread systems, could add a yield call here
	}
}

void Latch::Wait()
{
	++m_waiting;
	{
		//unique_lock<mutex> lock(m_condvarMutex);
		Util::Mutex::LockGuard lock(m_condvarMutex);
		while(m_count > 0) 
		{
			m_condvar.Wait(lock);
		}
	}
	--m_waiting;
}

bool Latch::try_wait() 
{
	++m_waiting;
	bool success;
	{
		//unique_lock<mutex> lock(m_condvarMutex);
		Util::Mutex::LockGuard lock(m_condvarMutex);
		success = (m_count == 0);
	}
	--m_waiting;
	return success;
}

void Latch::count_down(int n)
{
	Util::Mutex::LockGuard lock(m_condvarMutex);
	if (m_count - n < 0) 
	{
		throw std::logic_error("internal count == 0");
	}
	m_count -= n;
	if (m_count == 0) 
	{
		m_condvar.Broadcast();
	}
}

void Latch::arrive()
{
	count_down(1);
}

void Latch::arrive_and_wait() 
{
	++m_waiting;
	{
		//unique_lock<mutex> lock(m_condvarMutex);
		Util::Mutex::LockGuard lock(m_condvarMutex);
		if (m_count == 0) 
		{
			throw std::logic_error("internal count == 0");
		}
		if (--m_count == 0) 
		{
			m_condvar.Broadcast();
		}
		else 
		{
			while(m_count > 0) 
			{
				m_condvar.Wait(lock);
			}
		}
	}
	--m_waiting;
}

#ifdef HAS_CXX11_RVREF
ScopedGuard Latch::arrive_guard()
{
	function<void ()> f = bind(&Latch::arrive, this);
	return ScopedGuard(f);
}

ScopedGuard Latch::wait_guard() 
{
	function<void ()> f = bind(&Latch::Wait, this);
	return ScopedGuard(f);
}

ScopedGuard Latch::arrive_and_wait_guard()
{
	function<void ()> f = bind(&Latch::arrive_and_wait, this);
	return ScopedGuard(f);
}
#endif

CONCURRENCY_END
