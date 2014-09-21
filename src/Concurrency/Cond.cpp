// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Concurrency/Cond.h>

#ifndef _WIN32
#    include <sys/time.h>
#endif

#if defined(LANG_CPP11)

Util::Cond::Cond(void) 
try : 
# if defined(USING_COND_ANY)
	m_cond(std::condition_variable_any())
# else
	m_cond(std::condition_variable());
# endif
{
}  
catch(const std::system_error& ex)
{
	throw ThreadSyscallException(__FILE__, __LINE__, ex.code().value());
}

Util::Cond::~Cond(void)
{
	
}

/// 如果有线程在等待信号量，则激活任意一个线程，使其开始执行;如果无线程等待信号量，则不进行任何操作
void Util::Cond::Signal()
{
	m_cond.notify_one();
}

/// 激活所有等待此信号量的线程
void Util::Cond::Broadcast()
{
	m_cond.notify_all();
}

#elif defined(_WIN32)

#   ifdef HAS_WIN32_CONDVAR

Util::Cond::Cond()
{
	InitializeConditionVariable(&m_cond);
}

Util::Cond::~Cond()
{
}

void Util::Cond::Signal()
{
	WakeConditionVariable(&m_cond);
}

void Util::Cond::Broadcast()
{
	WakeAllConditionVariable(&m_cond);
}

#   else

Util::Cond::Cond(void) :
	m_condstate(Util::Cond::IDLE),
	m_blockednum(0),
	m_signaleddnum(0),
	m_gatesem(1)
{
}

Util::Cond::~Cond(void)
{
}

/// 如果有线程在等待信号量，则激活任意一个线程，使其开始执行;如果无线程等待信号量，则不进行任何操作
/// 注：当此函数被调用时，任何其他线程对此条件变量上的“发信号”(signaling(in wake))或“等待”(waiting to wait (in waitingToWait))操作的调用
/// 都会被信号量(m_gatesem)所阻塞，直至在信号量m_worksem上等待的正确数目(单个(SINGLED)或所有(BROADCAST))的线程从Wait返回，
/// 并释放信号量(m_gatesem)(through postWait)。
void Util::Cond::Signal()
{
	wake(false);
}

/// 激活所有等待此信号量的线程
void Util::Cond::Broadcast()
{
	wake(true);
}

void Util::Cond::wake(bool broadcast)
{
	// 锁定m_gatesem，若有线程在等待此条件变量，则信号量(m_gatesem)会一直锁定，
	// 直至等待线程从postWait将其释放
	m_gatesem.Wait();

	Util::Mutex::LockGuard syncSignal(m_signaledmutex);

	if (m_signaleddnum != 0)
	{
		m_blockednum -= m_signaleddnum;
		m_signaleddnum = 0;
	}

	//
	// If there are waiting threads then we enter a signal or
	// broadcast state.
	//
	if (0 < m_blockednum)		//> 有线程等待
	{
		// 解除一些线程的锁定(阻塞)
		assert(IDLE == m_condstate);
		m_condstate = broadcast ? BROADCAST : SIGNAL;

		// 将一个等待线程唤醒。此操作调用后等待线程将被唤醒，并在postWait操作中继续在m_worksem上执行Post()
		// 唤醒其他等待线程(m_condstate == BROADCAST)，或在m_gatesem上执行Post()以允许其他signaling或waiting to wait线程继续执行
		//
		// Posting the m_worksem wakes a single waiting thread. After this
		// occurs the waiting thread will wake and then either post on
		// the m_worksem to wake the next waiting thread, or post on the
		// gate to permit more signaling to proceed.
		//
		// Release before posting to avoid potential immediate
		// context switch due to the mutex being locked.
		//
		syncSignal.Release();
		m_worksem.Post();
	}
	else		//> 无线程等待
	{
		// 此处 syncSignal 手动提前在m_gatesem之前释放，而不是利用析构自动释放，
		// 是为了避免由于m_signaldmutex锁定而导致的潜在的频繁的上下文切换(由于m_gatesem是在
		// syncSignal之前获取锁定的，若释放时m_gatesem还在syncSignal之前，将会导致其他线程
		// 获取m_gatesem后，任然获取不到syncSignal而阻塞，从而导致线程切换，降低性能)
		//
		// Otherwise no blocked waiters, release the m_gatesem.
		//
		// Release before posting to avoid potential immediate
		// context switch due to the mutex being locked.
		//
		syncSignal.Release();
		m_gatesem.Post();		// allow waitingToWait to execute.
	}

}

void Util::Cond::doWait() const
{
	try
	{
		m_worksem.Wait();
		postWait(false);
	}
	catch (...)
	{
		postWait(true);
		throw;
	}
}

bool Util::Cond::timedDowait(const Time& timeout) const
{
	try
	{
		bool returnVal = m_worksem.TimedWait(timeout);
		postWait(!returnVal);
		return returnVal;
	}
	catch (...)
	{
		postWait(true);
		throw;
	}
}

/// 将等待线程加入阻塞队列
void Util::Cond::waitingToWait() const
{
	//
    // m_gatesem is used to protect m_blockednum. Furthermore, this prevents
    // further threads from entering the wait state while a
    // signal/broadcast is being processed.
    //
	m_gatesem.Wait();
	++m_blockednum;
	m_gatesem.Post();
}


/// 等待线程进入 postWait 时，如果存在更多阻塞线程需被激活(BROADCAST), 
/// 信号量m_worksem的V(Post)操作会再次被调用，以唤起其他等待线程。如果无其他线程需被激活，
/// 则信号量m_gatesem会被释放，从而允许signaling或waiting to wait线程继续执行
void Util::Cond::postWait(bool timed_out_or_failed) const
{
	Util::Mutex::LockGuard syncSignal(m_signaledmutex);

	//
	// One more thread has unblocked.
	//
	++m_signaleddnum;

	//
	// If m_condstate is IDLE then this must be a timeout, otherwise its a
	// spurious wakeup which is incorrect.
	//
	if (IDLE == m_condstate)			//> 等待线程未收到Signal() 或 Broadcast()
	{
		assert(timed_out_or_failed);	//> 未收到Signal() 或 Broadcast() 一定是由于超时 或 出错导致
		return;
	}

	//
	// m_condstate == SIGNAL / BROADCAST
	// 
	if (timed_out_or_failed) 
	{
		// 如果是最后一个阻塞的线程 且存在一个即将发生的signal/broadcast
		//
        // If the thread was the last blocked thread and there's a
        // pending signal/broadcast, reset the signal/broadcast to
        // prevent spurious wakeup.
        //
		if (m_signaleddnum == m_blockednum)	
		{
			m_condstate = IDLE;

			// 执行此操作是为了避免条件变量的虚假唤醒(spurious wakeup)
			// 当等待超时时，将接收不到同一时刻其他线程释放信号量m_worksem(Post<V>)时发出的通知，
			// 导致信号量m_worksem未被消费(consumed)，而出现spurious wakeup.
			//
			// Threads timing out present a particular issue because they may have
			// woken without a corresponding notification and its easy to leave
			// the m_worksem in a state where a spurious wakeup will occur --
			// consider a notify and a timed wake occuring at the same time. In
			// this case, if we are not careful the m_worksem will have been posted,
			// but the waking thread may not consume the semaphore.
			//
			// Consume the queue post to prevent spurious wakeup. Note
			// that although the m_signaledmutex mutex could be released
			// prior to this Wait() call, doing so gains nothing since
			// this Wait() MUST return immediately (if it does not
			// there is a major bug and the entire application will
			// deadlock).
			//
			m_worksem.Wait();

			syncSignal.Release(); 
			m_gatesem.Post();
		}
		// 对于非最后一个超时线程不进行任何操作，直接将其唤醒执行
	}
	else
	{
		if (SIGNAL == m_condstate || m_signaleddnum == m_blockednum)	//> signal 或到达最后一个阻塞线程
		{
			m_condstate = IDLE;	// 重置条件变量状态

			// 允许其他signaling或waiting to wait线程获取m_gatesem信号量继续执行
			syncSignal.Release();
			m_gatesem.Post();		
		}
		else		//> 广播(braadcast)并还有多个线程阻塞，等待唤醒
		{
			// 唤醒其他等待线程(m_condstate == BROADCAST)
			syncSignal.Release();
			m_worksem.Post();		// 释放信号量，允许其他线程被唤醒
		}
	}
}

#    endif // HAS_WIN32_CONDVAR

#else

Util::Cond::Cond(void) 
{
	pthread_condattr_t condattr;

	int returnVal = pthread_condattr_init(&condattr);
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}

#if ! defined(__hpux) && ! defined(__APPLE__)
	returnVal = pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC); 
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}
#endif

	returnVal = pthread_cond_init(&m_cond, &condattr);
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}

	returnVal = pthread_condattr_destroy(&condattr);
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}
}

Util::Cond::~Cond(void)
{
#ifndef NDEBUG
	int returnVal = pthread_cond_destroy(&m_cond);
	assert(0 == returnVal);
	//if (0 != returnVal)
	//{
	//	throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	//}
#else
    pthread_cond_destroy(&m_cond);
#endif
}

/// 如果有线程在等待信号量，则激活任意一个线程，使其开始执行;如果无线程等待信号量，则不进行任何操作
void Util::Cond::Signal()
{
	int returnVal = pthread_cond_signal(&m_cond);
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}
}

/// 激活所有等待此信号量的线程
void Util::Cond::Broadcast()
{
	int returnVal = pthread_cond_broadcast(&m_cond);
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}
}

#endif