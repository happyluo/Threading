// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Concurrency/Thread.h>
#include <Concurrency/ThreadException.h>

#ifdef LANG_CPP11

Util::Thread::Thread() :
	m_started(false),
	m_running(false)
{
}

Util::Thread::Thread(const string& name) :
	m_name(name),
	m_started(false),
	m_running(false)
{
}

Util::Thread::~Thread()
{
}

static unsigned int
/*WINAPI */StartHook(void* arg)
{
	// Ensure that the thread doesn't go away until run() has
	// completed.
	//
	Util::ThreadPtr thread;

	try
	{
		Util::Thread* rawThread = static_cast<Util::Thread*>(arg);

		//
		// Ensure that the thread doesn't go away until run() has
		// completed.
		//
		thread = rawThread;

		//
		// Initialize the random number generator in each thread on
		// Windows (the rand() seed is thread specific).
		//
		unsigned int seed = static_cast<unsigned int>(Util::Time::Now().ToMicroSeconds());
		srand(seed ^ static_cast<unsigned int>(hash<thread::id>()(thread->GetThreadControl().Id())));

		//
		// See the comment in Util::Thread::Start() for details.
		//
		rawThread->DecRef();
		thread->Run();
	}
	catch(...)
	{
		if (!thread->Name().empty())
		{
			cerr << thread->Name() << " terminating" << endl;
		}
		std::terminate();
	}

	thread->_done();

	return 0;
}

//#include <process.h>

Util::ThreadControl
Util::Thread::Start(size_t)
{
	return Start(0, 0);
}

Util::ThreadControl
Util::Thread::Start(size_t, int)
{
	//
	// Keep this alive for the duration of Start
	//
	Util::ThreadPtr keepMe = this;

	Util::Mutex::LockGuard lock(m_statemutex);

	if (m_started)
	{
		throw ThreadStartedException(__FILE__, __LINE__);
	}

	//
	// It's necessary to increment the reference count since
	// pthread_create won't necessarily call the thread function until
	// later. If the user does (new MyThread)->Start() then the thread
	// object could be deleted before the thread object takes
	// ownership. It's also necessary to increment the reference count
	// prior to calling pthread_create since the thread itself calls
	// DecRef().
	//
	IncRef();
	m_thread.reset(new thread(StartHook, this));

	m_started = true;
	m_running = true;

	return ThreadControl(m_thread);
}

Util::ThreadControl
Util::Thread::GetThreadControl() const
{
	Util::Mutex::LockGuard lock(m_statemutex);
	if (!m_started)
	{
		throw ThreadNotStartedException(__FILE__, __LINE__);
	}
	return ThreadControl(m_thread);
}

#elif defined(_WIN32)

Util::Thread::Thread(void) : 
	m_started(false),
	m_running(false),
	m_thread(0),
	m_id(0)
{
}

Util::Thread::Thread(const std::string& name) :
	m_name(name),
	m_started(false),
	m_running(false),
	m_thread(0),
	m_id(0)
{
	
}

Util::Thread::~Thread(void)
{
}

static unsigned int
WINAPI StartHook(void* arg)
{
	// Ensure that the thread doesn't go away until run() has
	// completed.
	//
	Util::ThreadPtr thread; // 若不使用此智能指针，在后续执行rawThread->DecRef(); 会导致线程对象被删除，使执行Run()时程序异常

	try
	{
		Util::Thread* rawThread = static_cast<Util::Thread*>(arg);

		//
		// Ensure that the thread doesn't go away until run() has
		// completed.
		//
		thread = rawThread;

#ifdef _WIN32
		//
		// Initialize the random number generator in each thread on
		// Windows (the rand() seed is thread specific).
		//
		unsigned int seed = static_cast<unsigned int>(Util::Time::Now().ToMicroSeconds());
		srand(seed ^ thread->GetThreadControl().Id());
#endif

		//
		// See the comment in Util::Thread::Start() for details.
		//
		rawThread->DecRef();
		thread->Run();
	}
	catch(...)
	{
		if (!thread->Name().empty())
		{
			std::cerr << thread->Name() << " terminating" << std::endl;
		}
#if defined(_MSC_VER) && (_MSC_VER < 1300)
		terminate();
#else
		std::terminate();
#endif
	}

	thread->_done();

	return 0;
}

Util::ThreadControl Util::Thread::Start(size_t stack_size)
{
	return Start(stack_size, THREAD_PRIORITY_NORMAL);
}

Util::ThreadControl Util::Thread::Start(size_t stack_size, int priority)
{
	// 确保在Start执行期间，当前对象不会被删除
	ThreadPtr keepMe(this);
	//ThreadPtr keepMe = this;

	Util::Mutex::LockGuard sync(m_statemutex);
	//Util::Mutex::LockGuard sync = m_statemutex;

	if (m_started)
	{
		throw ThreadStartedException(__FILE__, __LINE__);
	}

	// 此处必须增加当前对象的引用计数值，因为线程创建后，线程函数可能不会立即执行，可能会延迟到Start()函数返回之后才执行。
	// 若用户在Start()返回后，线程获取所有权得以执行之前delete 了该线程对象，会导致异常。
	// 而且此操作应该在线程创建之前执行，因为线程函数 StartHook()在执行Run()之前会执行DecRef() 操作。
	IncRef(); 

	unsigned int threatd_id;
	m_thread = reinterpret_cast<HANDLE>(_beginthreadex(0, 
		static_cast<unsigned int>(stack_size), StartHook, this, CREATE_SUSPENDED, &threatd_id));

	m_id = threatd_id;
	assert(m_thread != (HANDLE)-1L);
	if (0 == m_thread ||
		false == SetThreadPriority(m_thread, priority) ||
		-1 == ResumeThread(m_thread))
	{
		DecRef();
		throw ThreadSyscallException(__FILE__, __LINE__, GetLastError());
	}

	m_started = true;
	m_running = true;

	return ThreadControl(m_thread, m_id);
}

Util::ThreadControl Util::Thread::GetThreadControl() const
{
	Util::Mutex::LockGuard sync(m_statemutex);

	if (!m_started)
	{
		throw ThreadNotStartedException(__FILE__, __LINE__);
	}

	return ThreadControl(m_thread, m_id);
}

#else

Util::Thread::Thread(void) :
	m_started(false),
	m_running(false)
{
}

Util::Thread::Thread(const std::string& name) :
	m_name(name),
	m_started(false),
	m_running(false)
{

}

Util::Thread::~Thread(void)
{
}

extern "C"
{
static void* StartHook(void* arg)
{
	/// 若不使用此智能指针，在后续执行rawThread->DecRef(); 会导致线程对象被删除，使执行Run()时程序异常
	Util::ThreadPtr thread;

	try
	{
		Util::Thread* rawThread = static_cast<Util::Thread*>(arg);

		thread = rawThread;

		//
		// See the comment in Util::Thread::Start() for details.
		//
		rawThread->DecRef();
		thread->Run();
	}
	catch (...)
	{
		if (!thread->Name().empty())
		{
			std::cerr << thread->Name() << " terminating" << std::endl;
		}
		std::terminate();
	}

	thread->_done();

	return 0;
}

}

Util::Thread::ThreadControl Start(size_t stack_size)
{
	return Start(stack_size, false, 0);
}

Util::ThreadControl Util::Thread::Start(size_t stack_size, int priority)
{
	return Start(stack_size, true, priority);
}

Util::ThreadControl  Util::Thread::Start(size_t stack_size, bool realtime_scheduling, int priority)
{
    //
    // Keep this alive for the duration of start
    //
	Util::ThreadPtr keepMe = this;

	Util::Mutex::LockGuard sync(m_statemutex);

	if (m_started)
	{
		throw ThreadStartedException(__FILE__, __LINE__);
	}

	// 此处必须增加当前对象的引用计数值，因为线程创建后，线程函数可能不会立即执行，可能会延迟到Start()函数返回之后才执行。
	// 若用户在Start()返回后，线程获取所有权得以执行之前delete 了该线程对象，会导致异常。
	// 而且此操作应该在线程创建之前执行，因为线程函数 StartHook()在执行Run()之前会执行DecRef() 操作。
	IncRef(); 

	pthread_attr_t thread_attr;
	int returnVal = pthread_attr_init(&thread_attr);
	if (0 != returnVal)
	{
		DecRef();
		phread_attr_destroy(&thread_attr);
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}

	if (stack_size > 0)
	{
		if (stack_size < PTHREAD_STACK_MIN)
		{
			stack_size = PTHREAD_STACK_MIN;
		}
#ifdef __APPLE__
		if (stack_size % 4096 > 0)
		{
			stack_size = stack_size / 4096 * 4096 + 4096;
		}
#endif

		returnVal = pthread_attr_setstacksize(&thread_attr, stack_size);
		if (0 != returnVal)
		{
			DecRef();
			phread_attr_destroy(&thread_attr);
			throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
		}

		if (realtime_scheduling)
		{
			returnVal = pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
			if (0 != returnVal)
			{
				DecRef();
				//phread_attr_destroy(&thread_attr);
				throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
			}

			sched_param param;
			param.sched_priority = priority;
			returnVal = pthread_attr_setschedparam(&thread_attr, &param);
			if (0 != returnVal)
			{
				DecRef();
				pthread_attr_destroy(&thread_attr);
				throw ThreadSyscallException(__FILE__, __LINE__, rc);
			}
			pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);
		}

		returnVal = pthread_create(&m_thread, &thread_attr, StartHook, this);
		pthread_attr_destroy(&thread_attr);
		if (0 != returnVal)
		{
			DecRef();
			throw ThreadSyscallException(__FILE__, __LINE__, rc);
		}

		m_started = true;
		m_running = true;
		return ThreadControl(m_thread);
	}
}

Util::ThreadControl Util::Thread::GetThreadControl() const
{
	Util::Mutex::LockGuard sync(m_statemutex);

	if (!m_started)
	{
		throw ThreadNotStartedException(__FILE__, __LINE__);
	}

	return ThreadControl(m_thread);
}

#endif

bool Util::Thread::operator ==(const Thread& rhs) const
{
	return this == &rhs;
}

bool Util::Thread::operator !=(const Thread& rhs) const
{
	return this != &rhs;
}

bool Util::Thread::operator <(const Thread& rhs) const
{
	return this < &rhs;
}

//
// Check whether a thread is still alive.
//
bool Util::Thread::IsAlive() const
{
	Util::Mutex::LockGuard sync(m_statemutex);
	return m_running;
}

//
// Get the thread name
//
const std::string& Util::Thread::Name() const
{
	return m_name;
}

unsigned Util::Thread::HardwareConcurrency() UTIL_NOEXCEPT
{
#if defined(CTL_HW) && defined(HW_NCPU)
	unsigned num;
	int mib[2] = {CTL_HW, HW_NCPU};
	std::size_t len = sizeof(num);
	sysctl(mib, 2, &num, &len, 0, 0);
	return num;
#elif defined(_SC_NPROCESSORS_ONLN)
	long result = sysconf(_SC_NPROCESSORS_ONLN);

	// sysconf returns -1 if the name is invalid, the option does not exist or
	// does not have a definite limit.
	// if sysconf returns some other negative number, we have no idea
	// what is going on. Default to something safe.
	if (result < 0)
	{
		return 0;
	}
	return static_cast<unsigned>(result);
#elif defined(_WIN32)
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	return sysInfo.dwNumberOfProcessors;
#else  // defined(CTL_HW) && defined(HW_NCPU)
	// TODO: grovel through /proc or check cpuid on x86 and similar
	// instructions on other architectures.
#   if defined(_MSC_VER) && ! defined(__clang__)
		CONCURRENCY_WARNING("HardwareConcurrency not yet implemented")
#   else
#       warning HardwareConcurrency not yet implemented
#   endif

	return 0;  // Means not computable [thread.thread.static]
#endif  // defined(CTL_HW) && defined(HW_NCPU)
}

//
// 内部使用函数，勿调用
//
void Util::Thread::_done()
{
	Util::Mutex::LockGuard sync(m_statemutex);
	m_running = false;
}