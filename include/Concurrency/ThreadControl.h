// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_THREAD_CONTROL_H
#define CONCURRENCY_THREAD_CONTROL_H

#include <Concurrency/Config.h>
#include <Util/Time.h>
#include <Build/UndefSysMacros.h>

#ifdef LANG_CPP11
#   include <memory>
#   include <thread>
#	include <chrono>
#endif

namespace Util
{
	// 
	//在使用线程时，为了避免产生不确定的行为，必须遵守一些规则：
	//• 不要汇合或分离不是你自己创建的线程。
	//• 对于你创建的每个线程，你必须严格地进行一次汇合或分离；如果没有这样做，就可能造成资源泄漏。
	//• 不要在多个线程中针对某个线程调用 Join。
	//• 在你创建的其他所有线程终止之前，不要离开 main。
	//• 在临界区里调用 yield 是一个常见错误。这样做常常是没有意义的，因为 yield 调用会寻找另外一个能运行的线程，
	//	但当该线程运行时，它很可能会尝试进入由调用 yield 的线程持有的临界区，继而再次休眠。在最好的情况下，
	//	这什么也没有达成；而在最坏的情况下，它会造	成许多多余的上下文切换，却一无所获。
	//	只有在以下情况下，才应该调用 yield：另外的线程至少应该有机会实际运行，并做一点有用的事情。

class CONCURRENCY_API ThreadControl
{
public:

	/// 缺省构造器返回一个指向当前调用的线程ThreadControl 对象。可以获得当前线程的句柄。
	/// 在此对象上部可以调用 Join() 和 Detach() 方法，因为线程不可以等待(Join)或分离(Detach)自身
	ThreadControl(void);
	//~ThreadControl(void);

#ifdef LANG_CPP11
	typedef std::thread::id ID;
	ThreadControl(const std::shared_ptr<std::thread>& thread);
#elif defined(_WIN32)
	typedef DWORD ID;
	ThreadControl(HANDLE thread, ID id);
#else
	typedef pthread_t ID;
	explicit ThreadControl(pthread_t thread);
#endif

	// compare the thread's ID
	// == and != are meaningful only before the thread is joined/detached,
	// or while the thread is still running.
	//
	bool operator ==(const ThreadControl& rhs) const;
	bool operator !=(const ThreadControl& rhs) const;

	/// 挂起发出调用的线程，直到 Join 所针对的线程终止为止。
	//	注意，只能在一个线程中调用另一个线程的 Join 方法，也就	是说，只有一个线程能够等待另一个线程终止。
	//	如果在多个线程中调用某个线程的 Join 方法，就会产生不确定的行为。	如果你针对先前已经汇合(Joined)的、
	//	或是分离的(detached)线程调	用 join，也会产生不确定的行为。
	void Join();

	/// 分离一个线程。一旦线程分离，便不能再与它汇合。
	//	如果针对已经分离的(detached)、或已汇合的(Joined)线程调用 detach，会产生不确定的行为。
	//	注意，如果分离了一个线程，必须确保这个线程在主线程(main 函数)终止之前终止。这意味着，
	//	它们的生命期必须比主线程的生命期短，因为分离的线程不能再汇合。
	void Detach(); 

	/// 获取线程ID(On Windows)，或底层的pthread_t (on POSIX platforms.)
	ID Id() const;

	/// 挂起线程，时间长度由 Time 参数指定
	static void Sleep(const Time& timeout);

	/// 使调用线程放弃 CPU，让其他线程运行。
	static void Yield();

private:

#ifdef LANG_CPP11
	std::shared_ptr<std::thread>		m_thread;
	ID			m_id;
#elif defined(_WIN32)
	HANDLE	m_thread;
	ID			m_id;
#else
	pthread_t	m_thread;

	//
	// Used to prevent joining/detaching a ThreadControl constructed
	// with the default constructor. Only needed to enforce our
	// portable join/detach behavior.
	//
	bool		m_detachable;
#endif
};

}

#endif