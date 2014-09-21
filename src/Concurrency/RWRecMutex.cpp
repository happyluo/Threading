// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Concurrency/RWRecMutex.h>

Util::RWRecMutex::RWRecMutex(void) :
	m_count(0),
	m_waitingWriterNum(0),
	m_upgrading(false)
{
}

Util::RWRecMutex::~RWRecMutex(void)
{
}

// 注意：当此函数返回时，其并未对任何数据加锁，只是在检测到当前是写者有效(m_count < 0，锁被写者占用)
// 或是有writer 和upgrader在等待获取锁时，使读者reader 线程阻塞
void Util::RWRecMutex::ReadLock() const
{
	Util::Mutex::LockGuard sync(m_mutex);

	// Wait while a writer holds the lock or while writers or an upgrader
	// are waiting to get the lock.
	while (m_count < 0 || m_waitingWriterNum != 0)
	{
		m_readers.Wait(sync);
	}

	// 如果当前锁是被 reader 占有，并且没有writer 和upgrader等待锁，则直接将计数值加1，并进行读操作
	++m_count;
}

bool Util::RWRecMutex::TryReadLock() const
{
	Util::Mutex::LockGuard sync(m_mutex);

	// Would block if a writer holds the lock or if writers or an upgrader
	// are waiting to get the lock.
	if (m_count < 0 || m_waitingWriterNum != 0)
	{
		return false;
	}

	++m_count;
	return true;
}

bool Util::RWRecMutex::TimedReadLock(const Time& timeout) const
{
	Util::Mutex::LockGuard sync(m_mutex);

	Time end = Time::Now(Time::Monotonic) + timeout;
	while (m_count < 0 || m_waitingWriterNum != 0)
	{
		Time remainder = end - Time::Now(Time::Monotonic);
		if (remainder > Time())
		{
			if (false == m_readers.TimedWait(sync, remainder))
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	++m_count;
	return true;
}

void Util::RWRecMutex::WriteLock() const
{
	Util::Mutex::LockGuard sync(m_mutex);

	// 如果锁已经被当前写者占有(当前写线程循环加锁)
	if (m_count < 0 && m_writerThreadId == ThreadControl())
	{
		--m_count;
		return;
	}

	// Wait for the lock to become available and increment the number
	// of waiting writers.
	while (0 != m_count)
	{
		++m_waitingWriterNum;
		try
		{
			m_writers.Wait(sync);
		}
		catch (...)
		{
			--m_waitingWriterNum;
			throw;
		}
		--m_waitingWriterNum;
	}

	// Got the lock, indicate it's held by a writer.
	m_count = -1;
	m_writerThreadId = ThreadControl();
	return;
}

bool Util::RWRecMutex::TryWriteLock() const
{
	Util::Mutex::LockGuard sync(m_mutex);

	if (m_count < 0 && m_writerThreadId == ThreadControl())
	{
		--m_count;
		return true;
	}

	if (0 != m_count)
	{
		return false;
	}

	--m_count;
	m_writerThreadId = ThreadControl();
	return true;
} 

bool Util::RWRecMutex::TimedWriteLock(const Time& timeout) const
{
	Util::Mutex::LockGuard sync(m_mutex);

	if (m_count < 0 && m_writerThreadId == ThreadControl())
	{
		--m_count;
		return true;
	}

	Time end = Time::Now(Time::Monotonic) + timeout;
	while (0 != m_count)
	{
		Time remainder = end - Time::Now(Time::Monotonic);
		if (remainder > Time())
		{
			++m_waitingWriterNum;
			try
			{
				if (false == m_writers.TimedWait(sync, timeout))
				{
					--m_waitingWriterNum;
					return false;
				}
			}
			catch (...)
			{
				--m_waitingWriterNum;
				throw;
			}
			
			--m_waitingWriterNum;
		}
		else
		{
			return false;
		}
	}

	--m_count;
	m_writerThreadId = ThreadControl();
	return true;
}

void Util::RWRecMutex::Unlock() const
{
	bool writerWaiting = false;
	bool readerWaiting = false;

	{
		Util::Mutex::LockGuard sync(m_mutex);
		assert(0 != m_count);

		// 如果 m_count < 0, 则调用此函数的线程是一个writer，此时其占有锁，通过++m_cont将其释放
		if (m_count < 0)			//> 写锁(writer lock)
		{
			++m_count;

			if (m_count < 0)		//> 写循环锁
			{
				return;
			}
		}
		else						//> 读锁(reader lock)
		{
			// Reader called unlock
			--m_count;
		}

		// Writers are waiting (writerWaiting) if m_waitingWriterNum > 0.  In that
		// case, it's OK to let another writer into the region once there
		// are no more readers (m_count == 0). 
		writerWaiting = (0 == m_count && m_waitingWriterNum > 0);
		//Otherwise, no writers are waiting but readers may be waiting (readerWaiting).
		readerWaiting = (0 == m_waitingWriterNum);
	} // Unlock mutex.

	if (writerWaiting)
	{
		if (m_upgrading)
		{
			m_upgradeReader.Signal();		// upgrader 具有最高的优先级
		}
		else
		{
			m_writers.Signal();
		}
	}
	else if (readerWaiting)
	{
		m_readers.Broadcast();
	}
}

// 调用此函数之前，必须先持有其读锁(not hold a read lock)
// ReadLock or successful TryReadLock / TimedReadLock or Downgrade called on writer thread.
void Util::RWRecMutex::Upgrade() const
{
	Util::Mutex::LockGuard sync(m_mutex);

	if (true == m_upgrading)
	{
		throw DeadlockException(__FILE__, __LINE__);
	}

	// 在进入此函数之前线程已经获取了读锁,即调用 ReadLock / TryReadLock / 
	// TimedReadLock / 写(writer)线程调用了Downgrade 并成功返回
	assert(m_count > 0);
	--m_count;		// 由于线程持有的读锁将被提升为写锁，所以要将m_count 减1

	// m_count 不为0，则说明有其他的读取者正在找有锁，等待所有读取者执行完毕放锁
	m_upgrading = true;
	while (0 != m_count)
	{
		++m_waitingWriterNum;
		
		try
		{
			m_upgradeReader.Wait(sync);
		}
		catch (...)
		{
			// 异常，恢复原始状态
			m_upgrading = false;
			--m_waitingWriterNum;
			++m_count;
			throw;
		}

		--m_waitingWriterNum;
	}

	// 成功获取锁，提升为写锁
	m_count = -1;
	m_writerThreadId = ThreadControl();
	m_upgrading = false;
	return;
}

// 调用此函数之前，必须先持有其读锁(not hold a read lock)
// ReadLock or successful TryReadLock / TimedReadLock
bool Util::RWRecMutex::TimedUpgrade(const Time& timeout) const
{
	Util::Mutex::LockGuard sync(m_mutex);

	if (true == m_upgrading)
	{
		return false;
	}

	// 在进入此函数之前线程已经获取了读锁,即调用 ReadLock / TryReadLock / 
	// TimedReadLock / 写(writer)线程调用了Downgrade 并成功返回
	assert(m_count > 0);
	--m_count;		// 由于线程持有的读锁将被提升为写锁，所以要将m_count 减1

	m_upgrading = true;
	Time end = Time::Now(Time::Monotonic) + timeout;
	while (0 != m_count)
	{
		Time remainder = end - Time::Now(Time::Monotonic);
		if (remainder > Time())
		{
			++m_waitingWriterNum;
			try
			{
				if (false == m_upgradeReader.TimedWait(sync, remainder))
				{
					++m_count;
					m_upgrading = false;
					--m_waitingWriterNum;
					return false;
				}
			}
			catch (...)
			{
				++m_count;
				m_upgrading = false;
				--m_waitingWriterNum;
				throw;
			}
		}
		else
		{
			++m_count;
			m_upgrading = false;
			return false;
		}

		--m_waitingWriterNum;
	}

	m_count = -1;
	m_upgrading = false;
	m_writerThreadId = ThreadControl();

	return true;
}

// 此操作不释放锁，用户仍需调用Unlock 实现锁的释放
// 此函数配合WriteLock / Upgrade / TimedUpgrade 使用，实现写者和读者之间的切换，而无需调用Unlock, 提高效率
void Util::RWRecMutex::Downgrade() const
{
	Util::Mutex::LockGuard sync(m_mutex);

	// 当前线程持有写锁，将其降为读锁. 从而允许其他读取者获取锁，进行读操作
	if (-1 == m_count)		
	{
		m_count = 1;
	}
}