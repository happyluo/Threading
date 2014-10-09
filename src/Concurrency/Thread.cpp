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

Threading::Thread::Thread() :
    m_started(false),
    m_running(false)
{
}

Threading::Thread::Thread(const string& name) :
    m_name(name),
    m_started(false),
    m_running(false)
{
}

Threading::Thread::~Thread()
{
}

static unsigned int
/*WINAPI */StartHook(void* arg)
{
    // Ensure that the thread doesn't go away until run() has
    // completed.
    //
    Threading::ThreadPtr thread;

    try
    {
        Threading::Thread* rawThread = static_cast<Threading::Thread*>(arg);

        //
        // Ensure that the thread doesn't go away until run() has
        // completed.
        //
        thread = rawThread;

        //
        // Initialize the random number generator in each thread on
        // Windows (the rand() seed is thread specific).
        //
        unsigned int seed = static_cast<unsigned int>(Threading::Time::Now().ToMicroSeconds());
        srand(seed ^ static_cast<unsigned int>(hash<thread::id>()(thread->GetThreadControl().Id())));

        //
        // See the comment in Threading::Thread::Start() for details.
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

Threading::ThreadControl
Threading::Thread::Start(size_t)
{
    return Start(0, 0);
}

Threading::ThreadControl
Threading::Thread::Start(size_t, int)
{
    //
    // Keep this alive for the duration of Start
    //
    Threading::ThreadPtr keepMe = this;

    Threading::Mutex::LockGuard lock(m_statemutex);

    if (m_started)
    {
        throw ThreadStartedException(__FILE__, __LINE__);
    }

    IncRef();
    m_thread.reset(new thread(StartHook, this));

    m_started = true;
    m_running = true;

    return ThreadControl(m_thread);
}

Threading::ThreadControl
Threading::Thread::GetThreadControl() const
{
    Threading::Mutex::LockGuard lock(m_statemutex);
    if (!m_started)
    {
        throw ThreadNotStartedException(__FILE__, __LINE__);
    }
    return ThreadControl(m_thread);
}

#elif defined(_WIN32)

Threading::Thread::Thread(void) : 
    m_started(false),
    m_running(false),
    m_thread(0),
    m_id(0)
{
}

Threading::Thread::Thread(const std::string& name) :
    m_name(name),
    m_started(false),
    m_running(false),
    m_thread(0),
    m_id(0)
{
    
}

Threading::Thread::~Thread(void)
{
}

static unsigned int
WINAPI StartHook(void* arg)
{
    Threading::ThreadPtr thread; // 若不使用此智能指针，在后续执行rawThread->DecRef(); 会导致线程对象被删除，使执行Run()时程序异常

    try
    {
        Threading::Thread* rawThread = static_cast<Threading::Thread*>(arg);

        thread = rawThread;

#ifdef _WIN32
        unsigned int seed = static_cast<unsigned int>(Threading::Time::Now().ToMicroSeconds());
        srand(seed ^ thread->GetThreadControl().Id());
#endif

        //
        // See the comment in Threading::Thread::Start() for details.
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

Threading::ThreadControl Threading::Thread::Start(size_t stack_size)
{
    return Start(stack_size, THREAD_PRIORITY_NORMAL);
}

Threading::ThreadControl Threading::Thread::Start(size_t stack_size, int priority)
{
    ThreadPtr keepMe(this);

    Threading::Mutex::LockGuard sync(m_statemutex);

    if (m_started)
    {
        throw ThreadStartedException(__FILE__, __LINE__);
    }

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

Threading::ThreadControl Threading::Thread::GetThreadControl() const
{
    Threading::Mutex::LockGuard sync(m_statemutex);

    if (!m_started)
    {
        throw ThreadNotStartedException(__FILE__, __LINE__);
    }

    return ThreadControl(m_thread, m_id);
}

#else

Threading::Thread::Thread(void) :
    m_started(false),
    m_running(false)
{
}

Threading::Thread::Thread(const std::string& name) :
    m_name(name),
    m_started(false),
    m_running(false)
{

}

Threading::Thread::~Thread(void)
{
}

extern "C"
{
static void* StartHook(void* arg)
{
    Threading::ThreadPtr thread;

    try
    {
        Threading::Thread* rawThread = static_cast<Threading::Thread*>(arg);

        thread = rawThread;

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

Threading::ThreadControl Threading::Thread::Start(size_t stack_size)
{
    return Start(stack_size, false, 0);
}

Threading::ThreadControl Threading::Thread::Start(size_t stack_size, int priority)
{
    return Start(stack_size, true, priority);
}

Threading::ThreadControl  Threading::Thread::Start(size_t stack_size, bool realtime_scheduling, int priority)
{
    Threading::ThreadPtr keepMe = this;

    Threading::Mutex::LockGuard sync(m_statemutex);

    if (m_started)
    {
        throw ThreadStartedException(__FILE__, __LINE__);
    }

    IncRef(); 

    pthread_attr_t thread_attr;
    int returnVal = pthread_attr_init(&thread_attr);
    if (0 != returnVal)
    {
        DecRef();
        pthread_attr_destroy(&thread_attr);
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
            pthread_attr_destroy(&thread_attr);
            throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
        }
    }

    if (realtime_scheduling)
    {
        returnVal = pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
        if (0 != returnVal)
        {
            DecRef();
            //pthread_attr_destroy(&thread_attr);
            throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
        }

        sched_param param;
        param.sched_priority = priority;
        returnVal = pthread_attr_setschedparam(&thread_attr, &param);
        if (0 != returnVal)
        {
            DecRef();
            pthread_attr_destroy(&thread_attr);
            throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
        }
        pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);
    }

    returnVal = pthread_create(&m_thread, &thread_attr, StartHook, this);
    pthread_attr_destroy(&thread_attr);
    if (0 != returnVal)
    {
        DecRef();
        throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
    }

    m_started = true;
    m_running = true;
    return ThreadControl(m_thread);
}

Threading::ThreadControl Threading::Thread::GetThreadControl() const
{
    Threading::Mutex::LockGuard sync(m_statemutex);

    if (!m_started)
    {
        throw ThreadNotStartedException(__FILE__, __LINE__);
    }

    return ThreadControl(m_thread);
}

#endif

bool Threading::Thread::operator ==(const Thread& rhs) const
{
    return this == &rhs;
}

bool Threading::Thread::operator !=(const Thread& rhs) const
{
    return this != &rhs;
}

bool Threading::Thread::operator <(const Thread& rhs) const
{
    return this < &rhs;
}

//
// Check whether a thread is still alive.
//
bool Threading::Thread::IsAlive() const
{
    Threading::Mutex::LockGuard sync(m_statemutex);
    return m_running;
}

//
// Get the thread name
//
const std::string& Threading::Thread::Name() const
{
    return m_name;
}

unsigned Threading::Thread::HardwareConcurrency() UTIL_NOEXCEPT
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

void Threading::Thread::_done()
{
    Threading::Mutex::LockGuard sync(m_statemutex);
    m_running = false;
}