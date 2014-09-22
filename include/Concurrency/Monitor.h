// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_MONITOR_H
#define CONCURRENCY_MONITOR_H

#include <Concurrency/Config.h>
#include <Concurrency/Lock.h>
#include <Concurrency/Cond.h>

namespace Util
{

/// 监控器是一种用于保护临界区的同步机制：
///	和互斥体一样，同一时刻在临界区内，只能有一个线程在活动。但是，监控器允许你在临界区内挂起线程；
/// 这样，另一个线程就能进入临界区。第二个线程可以离开监控器(从而解除监控器的加锁)，或者
///	在监控器内挂起自己；不管是哪一种情况，原来的线程都会被唤醒，继续在监控器内执行。这样的行为
///	可以扩展到任意数目的线程，所以在监控器中可以有好几个线程挂起。

/// 按照 Mesa 语义，发出信号的线程会继续运行，	只有当发出信号的线程挂起自身(Wait)、或离开监控器(解锁)时，另外的线程才能得以运行。
/// 注意，发出通知(Notify)并不会致使另外的线程立即运行。只有当发出通知的线程调用Wait()、或Unlock()解除监控器的加锁时，
/// 另外的线程才会得以运行(Mesa 语义)。
/// 
template <typename M>
class Monitor : public noncopyable
{
public:
	//
	// Lock & TryLock typedefs.
	//
	//typedef LockT<Monitor<M> > LockGuard;
	//typedef TryLockT<Monitor<M> > TryLockGuard;
	typedef LockT<Monitor> LockGuard;
	typedef TryLockT<Monitor> TryLockGuard;

	Monitor(void);
	~Monitor(void);

	/// 锁监控器。如果监控器已被另外的线程锁住，发出调用的线程就会挂起，直到监控器可用为止。
	/// 在调用返回时，监控器已被它锁住。
	void Lock() const;

	/// 尝试锁住监控器。如果监控器可用，	调用就锁住监控器，返回 true。
	/// 如果监控器已被另外的线程锁住，调用返回 false。
	bool TryLock() const;

	/// 解除监控器的加锁。如果有另外的线程在等待进入监控器(也就是阻塞在Lock 调用中)，
	/// 其中一个线程会被唤醒，并锁住监控器。
	void Unlock() const;

	/// 挂起发出调用的线程，同时释放监控器上的锁。其他线程可以调用 Notify 或 NotifyAll 来唤醒
	/// 在 wait 调用中挂起的线程。当 wait 调用返回时，监控器重被锁住，而挂起的线程会恢复执行。
	void Wait() const;

	/// 挂起调用它的线程，直到指定的时间流逝。如果有另外的线程调用 Notify 或 NotifyAll，并在发生超时之前唤醒
	/// 挂起的线程，这个调用返回 true，监控器重被锁住，挂起的线程恢复执行。而如果发生超时，函数返回 false。
	bool TimedWait(const Time& timeout) const;

	/// 唤醒目前在 Wait 调用中挂起的一个线程。如果在调用 Notify 时没有这样的线程，通知就会丢失
	/// (也就是说，如果没有线程能被唤醒，对 Notify 的调用不会被记住)。
	/// 
	/// 注意，发出通知(Notify)并不会致使另外的线程立即运行。只有当发出通知的线程调用Wait()、或Unlock()解除监控器的加锁时，
	/// 另外的线程才会得以运行(Mesa 语义)。
	void Notify() const;

	/// 唤醒目前在 wait 调用中挂起的所有线程。和 Notify 一样，如果这时没有挂起的线程，对 NotifyAll 的调用就会丢失。
	void NotifyAll() const;

private:
	void notifyImpl() const;

	mutable Cond	m_cond;

	//	> [-1]: indicates a broadcast;
	//	> [ n](a positive number): indicates <n> calls to notify(). 
	mutable int		m_notifynum;

	M				m_mutex;

};

}

template <typename M>
inline Util::Monitor<M>::Monitor() : m_notifynum(0)
{
}

template <typename M>
inline Util::Monitor<M>::~Monitor()
{
	
}

template <typename M>
inline void Util::Monitor<M>::Lock() const
{
	m_mutex.Lock();
	if (m_mutex.WillUnlock())
	{
		// On the first mutex acquisition reset the number pending notifications.
		m_notifynum = 0;
	}
}

template <typename M>
inline bool Util::Monitor<M>::TryLock() const
{
	bool locked = m_mutex.TryLock();
	if (locked && m_mutex.WillUnlock())
	{
		// On the first mutex acquisition reset the number pending notifications.
		m_notifynum = 0;
	}

	return locked;
}

template <typename M>
inline void Util::Monitor<M>::Unlock() const
{
	if (m_mutex.WillUnlock())
	{
		 // Perform any pending notifications.
		notifyImpl();
	}
	m_mutex.Unlock();
}

template <typename M>
inline void Util::Monitor<M>::Wait() const
{
	notifyImpl();
	
	try
	{
		m_cond.waitImpl(m_mutex);
	}
	catch (...)
	{
		//m_notifynum = 0;
		throw;
	}

	//m_notifynum = 0;
	return;
}

template <typename M>
inline bool Util::Monitor<M>::TimedWait(const Time& timeout) const
{
	notifyImpl();

	bool returnVal = false;

	try
	{
		returnVal = m_cond.timedWaitImpl(m_mutex, timeout);
	}
	catch (...)
	{
		//m_notifynum = 0;
		throw;
	}

	//m_notifynum = 0;
	return returnVal;
}

template <typename M>
inline void Util::Monitor<M>::Notify() const
{
	if (-1 != m_notifynum)
	{
		++m_notifynum;
	}
}

template <typename M>
inline void Util::Monitor<M>::NotifyAll() const
{
	m_notifynum = -1;
}

template <typename M>
inline void Util::Monitor<M>::notifyImpl() const
{
	if (0 != m_notifynum)			//> Zero indicates no notifies.
	{
		if (-1 == m_notifynum)		//> -1 means notifyAll.
		{
			m_cond.Broadcast();
		}
		else if (m_notifynum > 0)	//> Otherwise notify n times.
		{
			while (m_notifynum-- > 0)
			{
				m_cond.Signal();
			}
		}

		m_notifynum = 0;
	}

	return;
}

#endif