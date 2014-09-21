// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_COND_H
#define CONCURRENCY_COND_H

#include <Concurrency/Config.h>
#include <Concurrency/ThreadException.h>
#include <Concurrency/Semaphore.h>

#if defined(LANG_CPP11)
#	include <condition_variable>
#	include <chrono>
#elif defined(_WIN32) && !defined(HAS_WIN32_CONDVAR)
#	include <Concurrency/Mutex.h>
#endif


namespace Util
{

//
// Forward declaration (for friend declarations).
//
template <class T> class Monitor;
class RecMutex;
class Mutex;

class Cond : public noncopyable
{
	friend class Monitor<Util::Mutex>;
	friend class Monitor<RecMutex>;

public:
	CONCURRENCY_API Cond(void);
	CONCURRENCY_API ~Cond(void);

	/// 如果有线程在等待此Cond，则激活任意一个线程，使其开始执行;如果无线程等待此Cond，则不进行任何操作
	/// 注：当此函数被调用时，任何其他线程对此Cond的受信(signaling(in wake))或等待(waiting to wait (in waitingToWait))操作的调用
	/// 都会被信号量(m_gatesem)所阻塞，直至在信号量m_worksem上等待的正确数目(单个(SINGLED)或所有(BROADCAST))的线程从Wait返回，
	/// 并释放信号量(m_gatesem)(through postWait)后。
	CONCURRENCY_API void Signal();

	/// 激活所有等待此Cond的线程
	CONCURRENCY_API void Broadcast();

	/// Wait 会自动释放mutex, 并使线程进入等待拥塞状态，等待条件变量的Single / Broadcast被触发。
	/// 在线程被激活之前会重新获取mutex 的锁定
	template <typename L> 
	inline void Wait(const L& lock) const
	{
		if (!lock.Acquired())
		{
			throw ThreadLockedException(__FILE__, __LINE__);
		}

		waitImpl(lock.m_mutex);
	}

	template <typename L, typename Predicate>
	inline void Wait(const L& lck, Predicate pred) const
	{
		while (!pred())
		{
			Wait(lck);
		}
	}

	/// Wait 会自动释放mutex, 并等待Cond至多timeout长的时间，若在timeout内Cond受信(Single被触发)，
	/// 线程重新获取mutex 的锁定并激活执行，并返回true；若超时，则返回false.
	template <typename L>
	inline bool TimedWait(const L& lock, const Time& timeout) const
	{
		if (!lock.Acquired())
		{
			throw ThreadLockedException(__FILE__, __LINE__);
		}
		
		return timedWaitImpl(lock.m_mutex, timeout);
	}

private:
	template <typename M>
	inline void waitImpl(const M& mutex) const;
	
	template <typename M>
	inline bool  timedWaitImpl(const M& mutex, const Time& timeout) const;

#if defined(LANG_CPP11)			// cpp11
#	if defined(USING_COND_ANY)
	mutable std::condition_variable_any m_cond;
#	else
	mutable std::condition_variable m_cond;
#	endif
#elif defined(_WIN32)			// win32
#  ifdef HAS_WIN32_CONDVAR			// WinRT & Visual Studio 2012
	mutable CONDITION_VARIABLE m_cond;   
#	else							// Windows XP or Windows Server 2003.
	CONCURRENCY_API void wake(bool broadcast);
	CONCURRENCY_API void doWait() const;
	CONCURRENCY_API bool timedDowait(const Time& timeout) const;
	/// 将等待线程加入阻塞队列
	CONCURRENCY_API void waitingToWait() const;
	
	/// 等待线程进入 postWait 时，如果存在更多阻塞线程需被激活(BROADCAST), 
	/// 信号量m_worksem的V(Post)操作会再次被调用，以唤起其他等待线程。如果无其他线程需被激活，
	/// 则信号量m_gatesem会被释放，从而允许signaling或waiting to wait线程继续执行
	CONCURRENCY_API void postWait(bool timed_out_or_failed) const;

	enum State
	{
		IDLE, SIGNAL, BROADCAST
	};

	mutable State	m_condstate;

	/// 记录阻塞的线程的数目
	mutable int	m_blockednum;

	/// 记录受信的线程的数目
	mutable int	m_signaleddnum;

	/// 对 m_signaleddnum 进行同步的互斥量，同一时刻只有一个等待线程受信。
	/// 在广播(Broadcast)情况下是多个等待线程随机连续受信
	Util::Mutex			m_signaledmutex;

	/// 对m_blockednum进行同步的信号量，允许多个线程进入等待状态
	/// 当有线程正在进入受信状态时，此信号量会将其他想要进入signal 或 wait 状态的线程阻塞，
	/// 直至所有受信线程(signaled threads)恢复工作，并释放m_gatesem信号量
	UtilInternal::Semaphore		m_gatesem;
	 
	/// 用于控制线程阻塞或受信的信号量，阻塞的线程用此信号量等待条件变量变为受信(be signaled)，即m_worksem.Post()被调用
	UtilInternal::Semaphore		m_worksem;

#	endif

#else							// Linux like sys.
	mutable pthread_cond_t m_cond;
#endif

};

#if defined(LANG_CPP11)

template <typename M>
inline void Cond::waitImpl(const M& mutex) const
{
	typedef typename M::LockState LockState;
	LockState state;
	mutex.unlock(state);

	try
	{
# if defined(USING_COND_ANY)
		m_cond.wait(*state.m_pmuxte);
# else
		std::unique_lock<std::mutex> ul(*state.m_pmuxte, std::adopt_lock);
		m_cond.wait(ul);
		ul.release();
# endif
		
		mutex.lock(state);
	}
	catch(const std::system_error& ex)
	{
		mutex.lock(state);
		throw ThreadSyscallException(__FILE__, __LINE__, ex.code().value());
	}
}

template <typename M>
inline bool  Cond::timedWaitImpl(const M& mutex, const Time& timeout) const
{
	if (timeout < Time::MicroSeconds(0))
	{
		throw InvalidTimeoutException(__FILE__, __LINE__, timeout);
	}

	typedef typename M::LockState LockState;
	LockState state;
	mutex.unlock(state);

	try
	{
# if defined(USING_COND_ANY)
		std::cv_status result = 
			m_cond.wait_for (*state.m_pmuxte, std::chrono::microseconds(timeout.ToMicroSeconds()));
# else
		std::unique_lock<std::mutex> ul(*state.m_pmuxte, std::adopt_lock);
		std::cv_status result = 
			m_cond.wait_for (ul, std::chrono::microseconds(timeout.ToMicroSeconds()));
		ul.release();
# endif
		
		mutex.lock(state);

		return std::cv_status::no_timeout == result;
	}
	catch(const std::system_error& ex)
	{
		mutex.lock(state);
		throw ThreadSyscallException(__FILE__, __LINE__, ex.code().value());
	}
}

#elif defined(_WIN32)

#   ifdef HAS_WIN32_CONDVAR

template <typename M> inline void
Cond::waitImpl(const M& mutex) const
{
	typedef typename M::LockState LockState;

	LockState state;
	mutex.unlock(state);
	BOOL ok = SleepConditionVariableCS(&m_cond, state.m_pmuxte, INFINITE);  
	mutex.lock(state);

	if (!ok)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, GetLastError());
	}
}

template <typename M> inline bool
Cond::timedWaitImpl(const M& mutex, const Time& timeout) const
{
	Util::Int64 msTimeout = timeout.ToMilliSeconds();
	if (msTimeout < 0 || msTimeout > 0x7FFFFFFF)
	{
		throw Util::InvalidTimeoutException(__FILE__, __LINE__, timeout);
	} 

	typedef typename M::LockState LockState;

	LockState state;
	mutex.unlock(state);
	BOOL ok = SleepConditionVariableCS(&m_cond, state.m_pmuxte, static_cast<DWORD>(msTimeout));  
	mutex.lock(state);

	if (!ok)
	{
		DWORD err = GetLastError();

		if (err != ERROR_TIMEOUT)
		{
			throw ThreadSyscallException(__FILE__, __LINE__, err);
		}
		return false;
	}
	return true;
}

#   else

template <typename M>
inline void Cond::waitImpl(const M& mutex) const
{
	waitingToWait();

	typedef typename M::LockState LockState;
	LockState state;
	mutex.unlock(state);		// 备份mutex的锁定状态，并重置mutex的锁状态，使其对其他线程可用

	try
	{
		doWait();				// 或取条件变量之后，从备份状态数据中恢复mutex的锁状态
		mutex.lock(state);
	}
	catch(...)
	{
		mutex.lock(state);
		throw;
	}
}

template <typename M>
inline bool  Cond::timedWaitImpl(const M& mutex, const Time& timeout) const
{
	if (timeout < Time::MicroSeconds(0))
	{
		throw InvalidTimeoutException(__FILE__, __LINE__, timeout);
	}

	waitingToWait();

	typedef typename M::LockState LockState;
	LockState state;
	mutex.unlock(state);	// 备份mutex的锁定状态，并重置mutex的锁状态，使其对其他线程可用

	try
	{
		bool result = timedDowait(timeout);
		mutex.lock(state);	// 或取条件变量之后，从备份状态数据中恢复mutex的锁状态
		return result;
	}
	catch (...)
	{
		mutex.lock(state);
		throw;
	}
}

#   endif

#else

template <typename M>
inline void Cond::waitImpl(const M& mutex) const
{
	typedef typename M::LockState LockState;
	LockState state;
	mutex.unlock(state);		
	int returnVal = pthread_cond_wait(&m_cond, state.m_pmuxte);
	mutex.lock(state);

	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}
}

template <typename M>
inline bool  Cond::timedWaitImpl(const M& mutex, const Time& timeout) const
{
	if (timeout < Time::MicroSeconds(0))
	{
		throw InvalidTimeoutException(__FILE__, __LINE__, timeout);
	}

	typedef typename M::LockState LockState;
	LockState state;
	mutex.unlock(state);	

#   ifdef __APPLE__
	//
	// The monotonic time is based on mach_absolute_time and pthread
	// condition variables require time from gettimeofday  so we get
	// the realtime time.
	//
	timeval tv = Time::Now(Time::Realtime) + timeout;
#   else
	timeval tv = Time::Now(Time::Monotonic) + timeout;
#   endif

	timespec ts;
	ts.tv_sec = tv.tv_sec;
	ts.tv_nsec = tv.tv_usec * 1000;

	int returnVal = pthread_cond_timedwait(&m_cond, state.m_pmutex, &ts);
	mutex.lock(state);

	if (0 != returnVal)
	{
		if (ETIMEDOUT != returnVal)
		{
			throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
		}
		
		return false;
	}

	return true;
}

#endif

}

#endif
