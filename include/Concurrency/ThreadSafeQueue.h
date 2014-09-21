// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_THREAD_SAFE_QUEUE_H
#define CONCURRENCY_THREAD_SAFE_QUEUE_H

//#define  USING_MONITOR

# ifdef USING_MONITOR
#	include <list>
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
//
// 此实现用成员变量 m_waitingReaders 来记住挂起的读取者的数目。构造器把这个变量初始化为零， Pop 的
// 实现在调用 Wait 之前和之后使这个变量增大和减小。请注意，这些语句处在 try-catch 块中；这样，即使
// Wait 抛出异常，在等待的读取者的计数也仍然会保持准确。最后，只有在有读取者在等待的情况下， Push 才
// 会调用 Notify。
//
// 这种实现的优点是，它使发生在监控器互斥体之上的竞争降到了最低限度：写入者每次都唤醒一个读取者，
// 所以不会发生多个读取者同时尝试锁住互斥体的情况。而且，监控器的 Notify 只有在解除了互斥体的加锁
// 之后，才会向等待中的线程发出信号。这意味着，当线程从 Wait 中醒来、重新尝试获取互斥体时，互斥体
// 很可能处在未加锁状态。这会使随后的操作更高效，因为获取未加锁的互斥体通常会非常高效，而强迫线程
// 在锁住的互斥体上休眠很昂贵(因为必须进行线程上下文切换)。
// 

template<class T> 
class ThreadSafeQueue : public Util::Monitor<Util::Mutex>
{
public:
	ThreadSafeQueue() : m_waitingReaders(0) {}

	SharedPtr<T> TryPop()
	{
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		if (0 == m_queue.size()) 
		{
			return SharedPtr<T>();
		}
		SharedPtr<T> result = m_queue.front();
		m_queue.pop_front();
		return result;
	}

	bool TryPop(T& value)
	{
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		if (0 == m_queue.size()) 
		{
			return false;
		}
		//value = std::move(*m_queue.front());
		value = *m_queue.front();
		m_queue.pop_front();
		return;
	}

	SharedPtr<T> Pop()		// Wait And Pop
	{
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		while (0 == m_queue.size()) 
		{
			try 
			{
				++m_waitingReaders;
				Wait();
				--m_waitingReaders;
			} 
			catch (...) 
			{
				--m_waitingReaders;
				throw;
			}
		}
		SharedPtr<T> result = m_queue.front();
		m_queue.pop_front();
		return result;
	}

	void Pop(T& value)		// Wait And Pop
	{
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		while (0 == m_queue.size()) 
		{
			try 
			{
				++m_waitingReaders;
				Wait();
				--m_waitingReaders;
			} 
			catch (...) 
			{
				--m_waitingReaders;
				throw;
			}
		}

		//value = std::move(*m_queue.front());
		value = *m_queue.front();
		m_queue.pop_front();
		return;
	}

	SharedPtr<T> TimedPop(const Time& timeout)			// Timed Wait And Pop
	{
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		//while (0 == m_queue.size()) 
		if (0 == m_queue.size())
		{
			try 
			{
				++m_waitingReaders;
				if (!TimedWait(timeout)) 
				{
					// time out
					--m_waitingReaders;
					return SharedPtr<T>();
				}
				--m_waitingReaders;
			} 
			catch (...) 
			{
				--m_waitingReaders;
				throw;
			}
		}

		if (0 == m_queue.size())
		{
			return false;
		}

		SharedPtr<T> result = m_queue.front();
		m_queue.pop_front();
		return result;
	}

	bool TimedPop(T& value, const Time& timeout)		// Timed Wait And Pop
	{
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		//while (0 == m_queue.size()) 
		if (0 == m_queue.size()) 
		{
			try 
			{
				++m_waitingReaders;
				if (!TimedWait(timeout)) 
				{
					// time out
					--m_waitingReaders;
					return false;
				}
				--m_waitingReaders;
			} 
			catch (...) 
			{
				--m_waitingReaders;
				throw;
			}
		}

		if (0 == m_queue.size())
		{
			return false;
		}

		//value = std::move(*m_queue.front());
		value = *m_queue.front();
		m_queue.pop_front();
		return true;
	}

	void Push(const T& new_value)		
	{
		SharedPtr<T> new_data(new T(new_value));
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		m_queue.push_back(new_data);
		if (m_waitingReaders)
		{
			Notify();
		}
	}

	void Push(const SharedPtr<T>& new_value)
	{
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		m_queue.push_back(new_value);
		if (m_waitingReaders)
		{
			Notify();
		}
	}

	bool Empty()
	{
		Util::Monitor<Util::Mutex>::LockGuard lock(*this);
		return 0 == m_queue.size();
	}


private:
	std::list<SharedPtr<T> > m_queue;
	short m_waitingReaders;
};

#	else

template<class T> 
class ThreadSafeQueue : public noncopyable
{
	
public:
	ThreadSafeQueue() : 
		m_head(new Node), 
		m_tail(m_head.Get()),
		m_waitingReaders(0)
	{
	}
	//ThreadSafeQueue(const ThreadSafeQueue& other)=delete;
	//ThreadSafeQueue& operator=(const ThreadSafeQueue& other)=delete;

	SharedPtr<T> TryPop();
	bool TryPop(T& value);
	SharedPtr<T> Pop();		// Wait And Pop
	void Pop(T& value);		// Wait And Pop
	SharedPtr<T> TimedPop(const Time& timeout);			// Timed Wait And Pop
	bool TimedPop(T& value, const Time& timeout);		// Timed Wait And Pop
	void Push(const T& new_value);
	void Push(const SharedPtr<T>& new_value);
	bool Empty();
	template<typename Function>
	void for_each(Function fun);
	size_t Erase(const T& new_value);
	size_t Erase(const SharedPtr<T>& new_value);

private:
	struct Node
	{
		Util::Mutex			m_mutex;
		SharedPtr<T>	m_data;
		UniquePtr<Node>	m_next;
	};
	
	Node* getTail();
	UniquePtr<Node> popHead();
	UniquePtr<Node> tryPopHead();
	UniquePtr<Node> tryPopHead(T& value);
	//Util::Mutex::LockGuard waitForData();
	UniquePtr<Node> waitPopHead();
	UniquePtr<Node> waitPopHead(T& value);
	UniquePtr<Node> timedWaitPopHead(const Time& timeout);
	UniquePtr<Node> timedWaitPopHead(T& value, const Time& timeout);

	Util::Mutex m_headmutex;
	//Util::Mutex m_tailmutex;
	UniquePtr<Node> m_head;
	Node* m_tail;
	Cond m_datacond;

	short m_waitingReaders;
};

template<typename T>
typename ThreadSafeQueue<T>::Node* ThreadSafeQueue<T>::getTail()
{
	Util::Mutex::LockGuard tail_lock(m_tail->m_mutex);
	return m_tail;
}

template<typename T>
UniquePtr<typename ThreadSafeQueue<T>::Node> ThreadSafeQueue<T>::popHead()
{
	//Util::Mutex::LockGuard head_lock(m_headmutex);
	//if (m_head.Get() == getTail())
	//{
	//	return nullptr;
	//}
	//UniquePtr<Node> const old_head = std::move(m_head);
	//m_head = std::move(old_head->m_next);
	//old_head->m_next = 0; 
	//return old_head;

	UniquePtr<Node> /*const*/ old_head = m_head;
	m_head = old_head->m_next; 
	return old_head;
}

template<typename T>
UniquePtr<typename ThreadSafeQueue<T>::Node> ThreadSafeQueue<T>::tryPopHead()
{
	Util::Mutex::LockGuard head_lock(m_headmutex);
	if (m_head.Get() == getTail())
	{
		return UniquePtr<Node>();
	}
	return popHead();
}

template<typename T>
UniquePtr<typename ThreadSafeQueue<T>::Node> ThreadSafeQueue<T>::tryPopHead(T& value)
{
	Util::Mutex::LockGuard head_lock(m_headmutex);
	if (m_head.Get() == getTail())
	{
		return UniquePtr<Node>();
	}

	//value = std::move(*m_head->m_data);
	value = *m_head->m_data;
	return popHead();
}

// template<typename T>
// Util::Mutex::LockGuard ThreadSafeQueue<T>::waitForData()
// {
// 	Util::Mutex::LockGuard head_lock(m_headmutex);
//	//++m_waitingReaders;
// 	//m_datacond.Wait(head_lock,[&]{return m_head != getTail();});
//	//--m_waitingReaders;
// 	while (m_head.Get() == getTail())
// 	{
// 		m_datacond.Wait(head_lock);
// 	}
// 	
// 	return std::move(head_lock);
// }

template<typename T>
UniquePtr<typename ThreadSafeQueue<T>::Node> ThreadSafeQueue<T>::waitPopHead()
{
	//Util::Mutex::LockGuard head_lock(waitForData());
 	Util::Mutex::LockGuard head_lock(m_headmutex);
 	while (m_head.Get() == getTail())
 	{
		try 
		{
			++m_waitingReaders;
			m_datacond.Wait(head_lock);
			--m_waitingReaders;
		} 
		catch (...) 
		{
			--m_waitingReaders;
			throw;
		}
 	}
	return popHead();
}

template<typename T>
UniquePtr<typename ThreadSafeQueue<T>::Node> ThreadSafeQueue<T>::waitPopHead(T& value)
{
	//Util::Mutex::LockGuard head_lock(waitForData());
	Util::Mutex::LockGuard head_lock(m_headmutex);
	while (m_head.Get() == getTail())
	{
		try 
		{
			++m_waitingReaders;
			m_datacond.Wait(head_lock);
			--m_waitingReaders;
		} 
		catch (...) 
		{
			--m_waitingReaders;
			throw;
		}
	}
	//value = std::move(*m_head->m_data);
	value = *m_head->m_data;
	return popHead();
}

template<typename T>
UniquePtr<typename ThreadSafeQueue<T>::Node> ThreadSafeQueue<T>::timedWaitPopHead(const Time& timeout)
{
	Util::Mutex::LockGuard head_lock(m_headmutex);
	while (m_head.Get() == getTail())
	//if (m_head.Get() == getTail())
	{
		try 
		{
			++m_waitingReaders;
			if (!m_datacond.TimedWait(head_lock, timeout)) 
			{
				// time out
				--m_waitingReaders;
				return UniquePtr<Node>();
			}
			--m_waitingReaders;
		} 
		catch (...) 
		{
			--m_waitingReaders;
			throw;
		}
	}

	if (m_head.Get() == getTail())
	{
		return UniquePtr<Node>();
	}

	return popHead();
}

template<typename T>
UniquePtr<typename ThreadSafeQueue<T>::Node> ThreadSafeQueue<T>::timedWaitPopHead(T& value, const Time& timeout)
{
	Util::Mutex::LockGuard head_lock(m_headmutex);
	while (m_head.Get() == getTail())
	//if (m_head.Get() == getTail())
	{
		try 
		{
			++m_waitingReaders;
			if (!m_datacond.TimedWait(head_lock, timeout)) 
			{
				// time out
				--m_waitingReaders;
				return UniquePtr<Node>();
			}
			--m_waitingReaders;
		} 
		catch (...) 
		{
			--m_waitingReaders;
			throw;
		}
	}

	if (m_head.Get() == getTail())
	{
		return UniquePtr<Node>();
	}

	//value = std::move(*m_head->m_data);
	value = *m_head->m_data;
	return popHead();
}

template<typename T>
SharedPtr<T> ThreadSafeQueue<T>::TryPop()
{
	UniquePtr<Node> const old_head = tryPopHead();
	return old_head ? old_head->m_data : SharedPtr<T>();
}

template<typename T>
bool ThreadSafeQueue<T>::TryPop(T& value)
{
	UniquePtr<Node> const old_head = tryPopHead(value);
	return old_head;
}

template<typename T>
SharedPtr<T> ThreadSafeQueue<T>::Pop()
{
	UniquePtr<Node> const old_head = waitPopHead();
	return old_head->m_data;
}

template<typename T>
void ThreadSafeQueue<T>::Pop(T& value)
{
	UniquePtr<Node> const old_head = waitPopHead(value);
}

template<typename T>
SharedPtr<T> ThreadSafeQueue<T>::TimedPop(const Time& timeout)			// Timed Wait And Pop
{
	UniquePtr<Node> const old_head = timedWaitPopHead(timeout);
	return old_head ? old_head->m_data : SharedPtr<T>();
}

template<typename T>
bool ThreadSafeQueue<T>::TimedPop(T& value, const Time& timeout)		// Timed Wait And Pop
{
	return timedWaitPopHead(value, timeout);
}

template<typename T>
void ThreadSafeQueue<T>::Push(const T& new_value)
{
	SharedPtr<T> new_data(new T(new_value));
	//SharedPtr<T> new_data = std::make_shared<T>(std::move(new_value)));
	UniquePtr<Node> p(new Node);
	{
		Util::Mutex::LockGuard tail_lock(m_tail->m_mutex);
		m_tail->m_data = new_data;
		Node* const new_tail = p.Get();
		//m_tail->m_next = std::move(p);
		m_tail->m_next = p;
		m_tail = new_tail;
	}

	if (m_waitingReaders)
	{
		m_datacond.Signal();
	}
}

template<typename T>
void ThreadSafeQueue<T>::Push(const SharedPtr<T>& new_value)
{
	UniquePtr<Node> p(new Node);
	{
		Util::Mutex::LockGuard tail_lock(m_tail->m_mutex);
		m_tail->m_data = new_value;
		Node* const new_tail = p.Get();
		//m_tail->m_next = std::move(p);
		m_tail->m_next = p;
		m_tail = new_tail;
	}

	if (m_waitingReaders)
	{
		m_datacond.Signal();
	}
}

template<typename T>
bool ThreadSafeQueue<T>::Empty()
{
	Util::Mutex::LockGuard head_lock(m_headmutex);
	return (m_head.Get() == getTail());
}

template<typename T>
template<typename Function>
void ThreadSafeQueue<T>::for_each(Function fun)
{
	Util::Mutex* sync = &m_headmutex;
	sync->Lock();

	Node* current = m_head.Get();
	while (current != getTail())
	{
		fun(*next->m_data);
		Node* const next = current->m_next.Get();
		Util::Mutex* next_sync = &next->m_mutex;
		next_sync->Lock();
		sync->Unlock();
		current = next;
		//sync = std::move(next_sync);
		sync = next_sync;
	}

	sync->Unlock();
}

//template<typename T>
//size_t ThreadSafeQueue<T>::Erase(const T& new_value)		
//{
//	SharedPtr<T> new_data(new T(new_value));
//	Util::Monitor<Util::Mutex>::LockGuard lock(*this);
//	return m_set.erase(new_data);
//}
//
//template<typename T>
//size_t ThreadSafeQueue<T>::Erase(const SharedPtr<T>& new_value)		
//{
//	Util::Monitor<Util::Mutex>::LockGuard lock(*this);
//	return m_set.erase(new_value);
//}

#	endif

}

#endif