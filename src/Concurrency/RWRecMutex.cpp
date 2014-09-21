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

// ע�⣺���˺�������ʱ���䲢δ���κ����ݼ�����ֻ���ڼ�⵽��ǰ��д����Ч(m_count < 0������д��ռ��)
// ������writer ��upgrader�ڵȴ���ȡ��ʱ��ʹ����reader �߳�����
void Util::RWRecMutex::ReadLock() const
{
	Util::Mutex::LockGuard sync(m_mutex);

	// Wait while a writer holds the lock or while writers or an upgrader
	// are waiting to get the lock.
	while (m_count < 0 || m_waitingWriterNum != 0)
	{
		m_readers.Wait(sync);
	}

	// �����ǰ���Ǳ� reader ռ�У�����û��writer ��upgrader�ȴ�������ֱ�ӽ�����ֵ��1�������ж�����
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

	// ������Ѿ�����ǰд��ռ��(��ǰд�߳�ѭ������)
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

		// ��� m_count < 0, ����ô˺������߳���һ��writer����ʱ��ռ������ͨ��++m_cont�����ͷ�
		if (m_count < 0)			//> д��(writer lock)
		{
			++m_count;

			if (m_count < 0)		//> дѭ����
			{
				return;
			}
		}
		else						//> ����(reader lock)
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
			m_upgradeReader.Signal();		// upgrader ������ߵ����ȼ�
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

// ���ô˺���֮ǰ�������ȳ��������(not hold a read lock)
// ReadLock or successful TryReadLock / TimedReadLock or Downgrade called on writer thread.
void Util::RWRecMutex::Upgrade() const
{
	Util::Mutex::LockGuard sync(m_mutex);

	if (true == m_upgrading)
	{
		throw DeadlockException(__FILE__, __LINE__);
	}

	// �ڽ���˺���֮ǰ�߳��Ѿ���ȡ�˶���,������ ReadLock / TryReadLock / 
	// TimedReadLock / д(writer)�̵߳�����Downgrade ���ɹ�����
	assert(m_count > 0);
	--m_count;		// �����̳߳��еĶ�����������Ϊд��������Ҫ��m_count ��1

	// m_count ��Ϊ0����˵���������Ķ�ȡ���������������ȴ����ж�ȡ��ִ����Ϸ���
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
			// �쳣���ָ�ԭʼ״̬
			m_upgrading = false;
			--m_waitingWriterNum;
			++m_count;
			throw;
		}

		--m_waitingWriterNum;
	}

	// �ɹ���ȡ��������Ϊд��
	m_count = -1;
	m_writerThreadId = ThreadControl();
	m_upgrading = false;
	return;
}

// ���ô˺���֮ǰ�������ȳ��������(not hold a read lock)
// ReadLock or successful TryReadLock / TimedReadLock
bool Util::RWRecMutex::TimedUpgrade(const Time& timeout) const
{
	Util::Mutex::LockGuard sync(m_mutex);

	if (true == m_upgrading)
	{
		return false;
	}

	// �ڽ���˺���֮ǰ�߳��Ѿ���ȡ�˶���,������ ReadLock / TryReadLock / 
	// TimedReadLock / д(writer)�̵߳�����Downgrade ���ɹ�����
	assert(m_count > 0);
	--m_count;		// �����̳߳��еĶ�����������Ϊд��������Ҫ��m_count ��1

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

// �˲������ͷ������û��������Unlock ʵ�������ͷ�
// �˺������WriteLock / Upgrade / TimedUpgrade ʹ�ã�ʵ��д�ߺͶ���֮����л������������Unlock, ���Ч��
void Util::RWRecMutex::Downgrade() const
{
	Util::Mutex::LockGuard sync(m_mutex);

	// ��ǰ�̳߳���д�������併Ϊ����. �Ӷ�����������ȡ�߻�ȡ�������ж�����
	if (-1 == m_count)		
	{
		m_count = 1;
	}
}