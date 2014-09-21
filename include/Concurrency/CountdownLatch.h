// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_COUNT_DOWN_LATCH_H
#define CONCURRENCY_COUNT_DOWN_LATCH_H

#include <Concurrency/Config.h>

namespace Util
{
class CONCURRENCY_API CountdownLatch
{
public:
	CountdownLatch(int count);
	~CountdownLatch(void);

	// �� 0 == GetCount() ʱ��CountDown() ��ʹ���������m_count����ֵ��1��Ϊ��ֵ
	void CountDown() const;

	// Await() ��⵽m_count����ֵΪ��ֵʱ����ʹ�����߳������ȴ������Ǽ���ִ��
	void Await() const;

	int GetCount() const;

	bool Reset(int count);

private:
	
#if defined(_WIN32)
	HANDLE		m_event;
	mutable long	m_count;
#else
	int				m_count;
	mutable pthread_mutex_t		m_mutex;
	mutable pthread_cond_t		m_cond;

	inline void lock() const;
	inline void unlock() const;
#endif
};

}

#endif