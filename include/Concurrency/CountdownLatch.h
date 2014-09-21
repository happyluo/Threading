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

	// 当 0 == GetCount() 时，CountDown() 会使闭锁对象的m_count计数值减1变为负值
	void CountDown() const;

	// Await() 检测到m_count计数值为负值时，不使调用线程阻塞等待，而是继续执行
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