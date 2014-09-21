// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_RW_LOCK_H
#define CONCURRENCY_RW_LOCK_H

#include <Util/Time.h>

namespace Util
{

//////////////////////////////////////////////////////////////////////////
/// class RLockT
template <typename M>
class RLockT
{
public:
	RLockT(const M& mutex) : m_mutex(mutex)
	{
		m_mutex.ReadLock();
		m_acquired = true;
	}

	~RLockT()
	{
		if (m_acquired)
		{
			m_mutex.Unlock();
		}
	}

	void Acquire() const
	{
		if (m_acquired)
		{
			throw ThreadLockedException(__FILE__, __LINE__);
		}
		m_mutex.ReadLock();
		m_acquired = true;
	}

	bool TryAcquire() const
	{
		if (m_acquired)
		{
			throw ThreadLockedException(__FILE__, __LINE__);
		}
		m_acquired = m_mutex.TryReadLock();
		return m_acquired;
	}

	bool TimedAcquire(const Time& timeout) const
	{
		if (m_acquired)
		{
			throw ThreadLockedException(__FILE__, __LINE__);
		}
		m_acquired = m_mutex.TimedReadLock(timeout);
		return m_acquired;
	}

	void Release() const
	{
		if (!m_acquired)
		{
			throw ThreadLockedException(__FILE__, __LINE__);
		}

		m_mutex.Unlock();
		m_acquired = false;
	}

	bool Acquired() const
	{
		return m_acquired;
	}

	void Upgrade() const
	{
		m_mutex.Upgrade();
	}

	bool TimedUpgrade(const Time& timeout) const
	{
		return m_mutex.TimedUpgrade(timeout);
	}

	void Downgrade() const
	{
		m_mutex.Downgrade();
	}

protected:
	// TryRLockT's constructors
	RLockT(const M& mutex, bool) : m_mutex(mutex)
	{
		m_acquired = m_mutex.TryReadLock();
	}

	RLockT(const M& mutex, const Time& timeout) : m_mutex(mutex)
	{
		m_acquired = m_mutex.TimedReadLock(timeout);
	}

private:
	// Not implemented; prevents accidental use.
	//
	RLockT(const RLockT&);
	RLockT& operator =(const RLockT&);

	const M&	m_mutex;
	mutable bool m_acquired;
};

//////////////////////////////////////////////////////////////////////////
/// class TryRLockT
template <typename M>
class TryRLockT : public RLockT<M>
{
public:
	TryRLockT(const M& mutex) :
		RLockT<M>(mutex, true)
	{
	}

	TryRLockT(const M& mutex, const Time& timeout) :
		RLockT(mutex, timeout)
	{
	}

};

//////////////////////////////////////////////////////////////////////////
/// class WLockT
template <typename M>
class WLockT
{
public:
	WLockT(const M& mutex) : m_mutex(mutex)
	{
		m_mutex.WriteLock();
		m_acquired = true;
	}

	~WLockT()
	{
		if (m_acquired)
		{
			m_mutex.Unlock();
		}
	}

	void Acquire() const
	{
		if (m_acquired)
		{
			throw ThreadLockedException(__FILE__, __LINE__);
		}
		m_mutex.WriteLock();
		m_acquired = true;
	}
	
	bool TryAcquire() const
	{
		if (m_acquired)
		{
			throw ThreadLockedException(__FILE__, __LINE__);
		}
		m_acquired = m_mutex.TryWriteLock();
		return m_acquired;
	}

	bool TimedAcquire(const Time& timeout) const
	{
		if (m_acquired)
		{
			throw ThreadLockedException(__FILE__, __LINE__);
		}
		m_acquired = m_mutex.TimedWriteLock(timeout);
		return m_acquired;
	}

	void Release() const
	{
		if (!m_acquired)
		{
			throw ThreadLockedException(__FILE__, __LINE__);
		}
		m_mutex.Unlock();
		m_acquired = false;
	}

	bool Acquired() const
	{
		return m_acquired;
	}

protected:
	WLockT(const M& mutex, bool) : m_mutex(mutex)
	{
		m_acquired = m_mutex.TryWriteLock();
	}
	
	WLockT(const M& mutex, const Time& timeout) : m_mutex(mutex)
	{
		m_acquired = m_mutex.TimedWriteLock(timeout);
	}

private:
	// Not implemented; prevents accidental use.
	//
	WLockT(const WLockT&);
	WLockT& operator=(const WLockT&);

	const M&	m_mutex;
	mutable bool m_acquired;
};

//////////////////////////////////////////////////////////////////////////
/// class TryWLockT
template <typename M>
class TryWLockT : public WLockT<M>
{
public:
	TryWLockT(const M& mutex) : WLockT(mutex, true)
	{
	}

	TryWLockT(const M& mutex, const Time& timeout) : 
		WLockT(mutex, timeout)
	{
	}

};

}


#endif