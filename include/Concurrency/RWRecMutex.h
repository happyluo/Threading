// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_RW_REC_MUTEX_H
#define CONCURRENCY_RW_REC_MUTEX_H

#include <Concurrency/Config.h>
#include <Util/Time.h>
#include <Concurrency/ThreadControl.h>
#include <Concurrency/Cond.h>

namespace Util
{
//
// 本实现使用条件变量来管理Reader 和 Writer.
//
// Reader & Writer mutexes 可以被循环加锁，在读者(Reader)占有锁时，通过调用Upgrade() or TimedUpgrade() 
// 可以将调用此接口的持有锁的读者的优先级提升，同时挂起自身，并等待当前其他持有互斥体的读取者执行完毕，
// 直到互斥体变得可用于写为止。其后此upgrader 将具有最高的优先级，其将先于所有Writer 获取锁，先执行。
//

class CONCURRENCY_API RWRecMutex : public noncopyable
{
public:
	RWRecMutex(void);
	~RWRecMutex(void);

	void ReadLock() const;

	bool TryReadLock() const;

	bool TimedReadLock(const Time& timeout) const;


	void WriteLock() const;

	bool TryWriteLock() const;

	bool TimedWriteLock(const Time& timeout) const;

	void Unlock() const;

	void Upgrade() const;

	bool TimedUpgrade(const Time& timeout) const;

	void Downgrade() const;

private:
	
	// noncopyable
	//RWRecMutex(const RWRecMutex&);
	//void operator=(const RWRecMutex&);

	// m_count > 0: 表示当前读者readers占有锁，数值表示活动的读者readers的数目；
	// m_count < 0: 表示当前写者writer占有锁，数值表示此写者循环加锁的次数( writeLock()调用的次数)；
	// m_count = 0: 初始状态，所未被占用，处于可用状态
	mutable int	m_count;

	// 如果当前锁被写者writer占有，m_writerThreadId 对应的是活动写者的线程ID
	mutable ThreadControl m_writerThreadId;

	// 等待锁的写者writers和upgrader(readers) 的数目.
	mutable unsigned int m_waitingWriterNum;

	// 是否有读者readers要求提升自己的优先级，以先于写者获取锁.
	// true if an upgrader wants the lock.
	mutable bool m_upgrading;

	// 实现对RWRecMutex结构本身内部的数据成员的互斥访问的.
	Util::Mutex m_mutex;

	// Condition variables for waiting readers, writers, and upgrader.
	mutable Cond m_readers;
	mutable Cond m_writers;

	// 试图提升锁(upgrade lock)的线程具有最高的优先级，新的读者(readers)和写者(writers)
	// 只有在upgrade 线程执行完之后才能获取锁
	mutable Cond m_upgradeReader;
};

}

#endif