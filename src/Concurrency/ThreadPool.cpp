// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo.xiaowei (at) hotmail.com>
//
// **********************************************************************

#include <Concurrency/ThreadPool.h>

void Threading::ThreadJoiner(const ThreadPtr& thread)
{
    if (thread && thread->IsAlive())
    {
        thread->GetThreadControl().Join();
    }
}

Threading::ThreadPool::ThreadPool(int threadnum
                             , const std::string& poolname
                             , const std::string& logger_file) : 
    m_destroyed(false), 
    m_joiner(m_threads),
    m_size(threadnum),
    m_sizemax(threadnum),
    m_sizewarn(threadnum),
    m_inuse(0),
    m_waitifnotask(true),
    m_haspriority(false),
    m_shrinkcycletime(0),
    m_threadidletime(0),
    m_priority(0),
    m_stacksize(0), 
    m_poolname(poolname)
{
    m_logger = new Logger(m_poolname, logger_file);

    try
    {
        for (unsigned i = 0; i < m_size; ++i)
        {
            m_threads.insert(new TaskThread(*this));
        }
    }
    catch(const Threading::Exception& ex)
    {
        m_destroyed = true;     
        {
            Error out(m_logger);
            out << "cannot create thread for `" << m_poolname << "':\n" << ex;
        }
        throw;
    }
}

Threading::ThreadPool::ThreadPool(const std::string& properties_file, 
                             const std::string& logger_file, 
                             const std::string& poolname) :
    m_destroyed(false), 
    m_joiner(m_threads),
    m_size(0),
    m_sizemax(0),
    m_sizewarn(0),
    m_inuse(0),
    m_waitifnotask(false),
    m_haspriority(false),
    m_shrinkcycletime(0),
    m_threadidletime(0),
    m_priority(0),
    m_stacksize(0), 
    m_poolname(poolname)
{
    m_properties = CreateProperties(0);
    m_properties->Load(properties_file);
    
    m_logger = new Logger(m_poolname, logger_file);

#ifndef OS_WINRT
#   ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int nProcessors = sysInfo.dwNumberOfProcessors;
#   else
    int nProcessors = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
#   endif        
#endif

    int size = m_properties->GetPropertyAsIntWithDefault(m_poolname + ".Size", 1);
    if (size < 1)
    {
        Warning out(m_logger);
        out << m_poolname << ".Size < 1; Size adjusted to 1";
        size = 1;
    }

    int sizeMax = m_properties->GetPropertyAsIntWithDefault(m_poolname + ".SizeMax", size);
#ifndef OS_WINRT
    if (-1 == sizeMax)
    {
        sizeMax = nProcessors;
    }
#endif
    if (sizeMax < size)
    {
        Warning out(m_logger);
        out << m_poolname << ".SizeMax < " << m_poolname << ".Size; SizeMax adjusted to Size (" << size << ")";
        sizeMax = size;
    }

    int sizeWarn = m_properties->GetPropertyAsInt(m_poolname + ".SizeWarn");
    if (sizeWarn != 0 && sizeWarn < size)
    {
        Warning out(m_logger);
        out << m_poolname << ".SizeWarn < " << m_poolname << ".Size; adjusted SizeWarn to Size (" << size << ")";
        sizeWarn = size;
    }
    else if (sizeWarn > sizeMax)
    {
        Warning out(m_logger);
        out << m_poolname << ".SizeWarn > " << m_poolname << ".SizeMax; adjusted SizeWarn to SizeMax (" << sizeMax << ")";
        sizeWarn = sizeMax;
    }

    const_cast<size_t&>(m_size) = static_cast<size_t>(size);
    const_cast<size_t&>(m_sizemax) = static_cast<size_t>(sizeMax);
    const_cast<size_t&>(m_sizewarn) = static_cast<size_t>(sizeWarn);

    int stackSize = m_properties->GetPropertyAsInt(m_poolname + ".StackSize");
    if (stackSize < 0)
    {
        Warning out(m_logger);
        out << m_poolname << ".StackSize < 0; Size adjusted to OS default";
        stackSize = 0;
    }
    const_cast<size_t&>(m_stacksize) = static_cast<size_t>(stackSize);

    const_cast<bool&>(m_waitifnotask) = "true" == Threading::ToLower(m_properties->GetProperty(m_poolname + ".WaitIfNoTask"));
    if (m_waitifnotask)
    {
        int threadIdleTime = m_properties->GetPropertyAsIntWithDefault(m_poolname + ".ThreadIdleTime", 6000);
        if (threadIdleTime < 0)
        {
            Warning out(m_logger);
            out << m_poolname << ".ThreadIdleTime < 0; ThreadIdleTime adjusted to 0 milliseconds";
            threadIdleTime = 0;
        }
        const_cast<Int64&>(m_threadidletime) = threadIdleTime;
    }
        
    const_cast<bool&>(m_haspriority) = "" != m_properties->GetProperty(m_poolname + ".ThreadPriority");
    const_cast<int&>(m_priority) = m_properties->GetPropertyAsInt(m_poolname + ".ThreadPriority");
    if (!m_haspriority)
    {
        const_cast<bool&>(m_haspriority) = "" != m_properties->GetProperty("ThreadPriority");
        const_cast<int&>(m_priority) = m_properties->GetPropertyAsInt("ThreadPriority");
    }

    /// pre-create threads.
    try
    {
        for (size_t i = 0; i < m_size; ++i)
        {
            m_threads.insert(new TaskThread(*this));
        }
    }
    catch(const Threading::Exception& ex)
    {
        m_destroyed = true;     
        {
            Error out(m_logger);
            out << "cannot create thread for `" << m_poolname << "':\n" << ex;
        }
        throw;
    }
}

Threading::ThreadPool::~ThreadPool(void)
{
    LockGuard sync(*this);
    
    JoinAll();

    assert(0 == m_inuse);
}

void Threading::ThreadPool::Reset()
{
    m_destroyed = false;

    /// recreate threads.
    try
    {
        for (size_t i = 0; i < m_size; ++i)
        {
            m_threads.insert(new TaskThread(*this));
        }
    }
    catch(const Threading::Exception& ex)
    {
        m_destroyed = true;     
        {
            Error out(m_logger);
            out << "cannot create thread for `" << m_poolname << "':\n" << ex;
        }
        throw;
    }
}

void Threading::ThreadPool::SubmitTask(const TaskPtr& task)
{
    if (m_destroyed)
    {
        Error out(m_logger);
        out << "cannot submit task to the destroyed ThreadPool: `" << m_poolname << "'\n"
            << "please reset this pool, or create a new thread pool.";
        
        throw ThreadPoolDestroyedException(__FILE__, __LINE__);
    }

    if (!task)
    {
        return;
    }

    m_tasksqueue.Push(task);

    {
        LockGuard sync(*this);
        size_t inuse = ++m_inuse;
        if (inuse >= m_sizemax || inuse < m_size)
        {
            return;
        }

        if (inuse == m_sizewarn)
        {
            Warning out(m_logger);
            out << "thread pool `" << m_poolname << "' is running low on threads\n"
                << "Size=" << m_size << ", " << "SizeMax=" << m_sizemax << ", " << "SizeWarn=" << m_sizewarn;
        }

        assert(inuse <= m_threads.size());
        if (inuse <= m_sizemax && inuse == m_threads.size())
        {
            {
                Trace out(m_logger, "");
                out << "growing " << m_poolname << ": Size=" << m_threads.size() + 1;
            }

            try
            {
                m_threads.insert(new TaskThread(*this));
            }
            catch(const Threading::Exception& ex)
            {
                m_destroyed = true;     
                {
                    Error out(m_logger);
                    out << "cannot create thread for `" << m_poolname << "':\n" << ex;
                }
                throw;
            }
        }
    }
}

Threading::TaskPtr Threading::ThreadPool::SubmitTask(void (*fun)(void *), void *param)
{
    TaskPtr task(new CallbackTask(fun, param));

    SubmitTask(task);

    return task;
}

void Threading::ThreadPool::JoinAll()
{
    while (!m_tasksqueue.Empty()) {};

    m_destroyed = true;

    size_t joinedsize = m_threads.size();
    std::set<ThreadPtr>::const_iterator iter = m_threads.begin(); 
    while (iter != m_threads.end())
    {
        ThreadJoiner(*iter);
        m_threads.erase(iter++);
    }

    {
        Trace out(m_logger, "");
        out << "all the threads of `" << m_poolname << "' have joined. Joined threads Size=" << joinedsize;
    }
}

void Threading::ThreadPool::SetThreadIdleTime(const Time& idle_time)
{
    const_cast<bool&>(m_waitifnotask) = true;
    Threading::Int64 threadIdleTime = idle_time.ToMilliSeconds();
    if (threadIdleTime < 0)
    {
        Warning out(m_logger);
        out << m_poolname << ".ThreadIdleTime < 0; ThreadIdleTime adjusted to 6000 milliseconds";
        threadIdleTime = 6000;
    }
    const_cast<Int64&>(m_threadidletime) = threadIdleTime;
}