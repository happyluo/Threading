// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_THREAD_SAFE_SET_H
#define CONCURRENCY_THREAD_SAFE_SET_H

#define  USING_MONITOR

# ifdef USING_MONITOR
#	include <set>
#	include <ConcurrencyMonitor.h>
# else
#	include <Concurrency/Mutex.h>
#	include <Concurrency/Cond.h>
#	include <Util/SharedPtr.h>
#	include <Util/UniquePtr.h>
# endif

namespace Util
{
#	ifdef USING_MONITOR

template<class T> 
class ThreadSafeSet : public Util::Monitor<Util::Mutex>
{
public:
	ThreadSafeSet() {}
	
	size_t Size() const
	{
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		return m_set.size();
	}

	size_t Erase(const T& new_value)		
	{
		SharedPtr<T> new_data(new T(new_value));
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		return m_set.erase(new_data);
	}

	size_t Erase(const SharedPtr<T>& new_value)		
	{
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		return m_set.erase(new_value);
	}

	bool Insert(const T& new_value)		
	{
		SharedPtr<T> new_data(new T(new_value));
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		return m_set.insert(new_data).second;
	}

	bool Insert(const SharedPtr<T>& new_value)
	{
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		return m_set.insert(new_value).second;
	}

	bool Empty()
	{
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		return 0 == m_set.size();
	}

private:
	std::set<SharedPtr<T> > m_set;
};

#endif

}

#endif