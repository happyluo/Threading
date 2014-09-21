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
// Concurrency primitive that allows many readers & one writer access
// to a data structure. Writers have priority over readers. The
// structure is not strictly fair in that there is no absolute queue
// of waiting writers - that is managed by a condition variable.
//
// Both Reader & Writer mutexes can be recursively locked. Calling
// upgrade() or timedUpgrade() while holding a read lock promotes
// the reader to a writer lock.
//
// 本实现使用条件变量来管理Reader 和 Writer.
//
// Reader & Writer mutexes 可以被循环加锁，在读者(Reader)占有锁时，通过调用Upgrade() or TimedUpgrade() 
// 可以将调用此接口的持有锁的读者的优先级提升，同时挂起自身，并等待当前其他持有互斥体的读取者执行完毕，
// 直到互斥体变得可用于写为止。其后此upgrader 将具有最高的优先级，其将先于所有Writer 获取锁，先执行。
//
/// 此处实现的读写锁，并非是真正的锁，其内部实现时，是使用条件变量来控制 读线程 和 写线程 的执行或阻塞，
/// 从而实现它们对数据的互斥访问。
///
/// 实现时，内部使用的互斥量(Util::Mutex) 是用来实现对RWRecMutex结构本身内部的数据成员的互斥访问的。

class CONCURRENCY_API RWRecMutex : public noncopyable
{
public:
	RWRecMutex(void);
	~RWRecMutex(void);

	// 尝试获取读锁。如果目前有写入者持有互斥体，调用者就会挂起，直到互斥体变得可用于读取为止。
	// 如果可以获取互斥体，或者目前只有读取者持有互斥体，这个调用就会锁住互斥体，并立即返回。
	// 注意：当此函数返回时，其并未对任何数据加锁，只是在检测到当前是写者有效(m_count < 0，锁被写者占用)
	// 或是有writer 和upgrader在等待获取锁时，使读者reader 线程阻塞
	void ReadLock() const;

	// 尝试获取读锁。如果锁目前由写入者持有，这个函数就会返回 false。否则，它获取锁，并返回 true。
	bool TryReadLock() const;

	// 试获取读锁锁。如果锁目前由写入者持有，函数就会等待指定的时长，直到发生超时。
	// 如果在超时之前获取了锁，函数就会返回 true。否则，一旦发生超时，这个函数就返回 false。
	bool TimedReadLock(const Time& timeout) const;


	// 获取写锁。如果目前有读取者或写入者持有互斥体，调用者就会挂起，直到互斥体可用于写为止。
	// 如果可以获取互斥体，这个调用就获取锁，并立即返回。
	void WriteLock() const;

	// 获取写锁。如果锁目前由读取者或写入者持有，这个函数返回 false。否则，它获取锁，返回 true。
	bool TryWriteLock() const;

	// 试获取写锁。如果锁目前由读取者或写入者持有，函数就等待指定的时长，直到发生超时。
	// 如果在超时之前获取了锁，函数就会返回 true。否则，一旦发生超时，这个函数就返回 false。
	bool TimedWriteLock(const Time& timeout) const;

	// 解除互斥体的加锁 （不管目前持有锁的是读取者还是写入者）。
	void Unlock() const;

	// 使一个读锁升级成写锁。如果目前有其他读取者持有互斥体，调用者就会挂起，直到互斥体变得可用于写为止。
	// 如果可以升级互斥体，调用者就会获取锁，并立即返回。
	// 【注意：】 
	//	1.  调用此函数之前，必须先持有其读锁(not hold a Read Lock);
	//	2. 同一时刻只允许一个读取者(reader) 提升自己持有的锁，如果有多个线程调用Upgrade, 则除第一个线程之外的
	//		所有线程都将收到DeadlockException异常.(all but the first thread receive a DeadlockException.)
	//	3. Upgrade是非递归的。不要在同一个线程中多次调用它。
	void Upgrade() const;

	// 尝试把读锁升级成写锁。如果目前锁由其他读取者持有，函数就会等待指定的时长，直到发生超时。
	// 如果在超时之前获取了锁，这个函数就会返回 true。否则，一旦发生超时或其他线程读线程正在等待提升自己持有的锁，
	// 函数就返回 false。
	// 【注意：】 
	//	1.  调用此函数之前，必须先持有其读锁(not hold a Read Lock);
	bool TimedUpgrade(const Time& timeout) const;

	// 将当前写锁(write lock)降级为读者(read lock)Downgrade a write lock to a read lock.
	// 【注意：】 
	// 调用Downgrade (or Unlock) 的次数必须和调用 WriteLock / Upgrade / successfully called TimedUpgrade 
	// 的次数相同. 执行此操作后其他读取者获取锁，进行读操作
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