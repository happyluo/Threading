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

/// �������һ�����ڱ����ٽ�����ͬ�����ƣ�
///	�ͻ�����һ����ͬһʱ�����ٽ����ڣ�ֻ����һ���߳��ڻ�����ǣ���������������ٽ����ڹ����̣߳�
/// ��������һ���߳̾��ܽ����ٽ������ڶ����߳̿����뿪�����(�Ӷ����������ļ���)������
///	�ڼ�����ڹ����Լ�����������һ�������ԭ�����̶߳��ᱻ���ѣ������ڼ������ִ�С���������Ϊ
///	������չ��������Ŀ���̣߳������ڼ�����п����кü����̹߳���

/// ���� Mesa ���壬�����źŵ��̻߳�������У�	ֻ�е������źŵ��̹߳�������(Wait)�����뿪�����(����)ʱ��������̲߳��ܵ������С�
/// ע�⣬����֪ͨ(Notify)��������ʹ������߳��������С�ֻ�е�����֪ͨ���̵߳���Wait()����Unlock()���������ļ���ʱ��
/// ������̲߳Ż��������(Mesa ����)��
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

	/// ������������������ѱ�������߳���ס���������õ��߳̾ͻ����ֱ�����������Ϊֹ��
	/// �ڵ��÷���ʱ��������ѱ�����ס��
	void Lock() const;

	/// ������ס������������������ã�	���þ���ס����������� true��
	/// ���������ѱ�������߳���ס�����÷��� false��
	bool TryLock() const;

	/// ���������ļ����������������߳��ڵȴ���������(Ҳ����������Lock ������)��
	/// ����һ���̻߳ᱻ���ѣ�����ס�������
	void Unlock() const;

	/// ���𷢳����õ��̣߳�ͬʱ�ͷż�����ϵ����������߳̿��Ե��� Notify �� NotifyAll ������
	/// �� wait �����й�����̡߳��� wait ���÷���ʱ��������ر���ס����������̻߳�ָ�ִ�С�
	void Wait() const;

	/// ������������̣߳�ֱ��ָ����ʱ�����š������������̵߳��� Notify �� NotifyAll�����ڷ�����ʱ֮ǰ����
	/// ������̣߳�������÷��� true��������ر���ס��������ָ̻߳�ִ�С������������ʱ���������� false��
	bool TimedWait(const Time& timeout) const;

	/// ����Ŀǰ�� Wait �����й����һ���̡߳�����ڵ��� Notify ʱû���������̣߳�֪ͨ�ͻᶪʧ
	/// (Ҳ����˵�����û���߳��ܱ����ѣ��� Notify �ĵ��ò��ᱻ��ס)��
	/// 
	/// ע�⣬����֪ͨ(Notify)��������ʹ������߳��������С�ֻ�е�����֪ͨ���̵߳���Wait()����Unlock()���������ļ���ʱ��
	/// ������̲߳Ż��������(Mesa ����)��
	void Notify() const;

	/// ����Ŀǰ�� wait �����й���������̡߳��� Notify һ���������ʱû�й�����̣߳��� NotifyAll �ĵ��þͻᶪʧ��
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