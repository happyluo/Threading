// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_THREAD_POOL_H
#define CONCURRENCY_THREAD_POOL_H

#include <Config.h>
#include <Concurrency/Runnable.h>
#include <Concurrency/Thread.h>
#include <Concurrency/ThreadSafeQueue.h>
#include <Concurrency/ThreadSafeList.h>
#include <Concurrency/Monitor.h>
#include <Util/Atomic.h>
#include <Util/Properties.h>
#include <Logging/LoggerUtil.h>

THREADING_BEGIN

void ThreadJoiner(const ThreadPtr& thread);

//////////////////////////////////////////////////////////////////////////
/// class TaskBase
class THREADING_API TaskBase : public Threading::Runnable
    , virtual public Threading::Shared
    , public Threading::Monitor<Threading::Mutex>
{
public:
    TaskBase(const std::string& name) : m_name(name)
        , m_done(false)
    {
    }

    virtual ~TaskBase() {}

    const std::string& Name() const
    {
        return m_name;
    }

    void Reset()
    {
        Threading::Monitor<Threading::Mutex>::LockGuard lock(*this);
        m_done = false;
        m_waiters = 0;
    }

    void WaitDone() const
    {
        Threading::Monitor<Threading::Mutex>::LockGuard lock(*this);
        while (!m_done)
        {
            try 
            {
                ++m_waiters;
                Wait();
                --m_waiters;
            } 
            catch (...) 
            {
                --m_waiters;
                throw;
            }
        }
    }

    bool TimedWaitDone(const Time& timeout) const
    {
        Threading::Monitor<Threading::Mutex>::LockGuard lock(*this);

        if (m_done)
        {
            return true;
        }

        bool returnVal = false;

        try 
        {
            ++m_waiters;
            returnVal = TimedWait(timeout);
            --m_waiters;
        } 
        catch (...) 
        {
            --m_waiters;
            throw;
        }

        return returnVal;
    }

    void NotifyDone() const
    {
        Threading::Monitor<Threading::Mutex>::LockGuard lock(*this);
        m_done = true;

        if (0 != m_waiters)
        {
            //Notify();
            NotifyAll();
        }
    }

protected:
    const std::string m_name;

private:
    mutable bool    m_done;
    mutable int        m_waiters;
};

typedef SharedPtr<TaskBase> TaskPtr;

//////////////////////////////////////////////////////////////////////////
/// class CallbackTask
class CallbackTask : public TaskBase
{
public:
    CallbackTask(void (*fun)(void *), void *param = NULL, const std::string& name = "") : 
        TaskBase(name)
        , m_fun(fun)
        , m_param(param)
    {
    }

    virtual ~CallbackTask() 
    {
        m_fun = NULL;
        m_param = NULL;
    }

    virtual void Run()
    {
        return m_fun(m_param);
    }

protected:
    void (*m_fun)(void *);
    void *m_param;
};



//////////////////////////////////////////////////////////////////////////
/// class JoinThreads
class THREADING_API JoinThreads
{
public:
    explicit JoinThreads(std::set<ThreadPtr>& threads) : m_threads(threads)
    {
    }

    ~JoinThreads()
    {
        //size_t joinedsize = m_threads.size();
        std::set<ThreadPtr>::const_iterator iter = m_threads.begin(); 
        while (iter != m_threads.end())
        {
            ThreadJoiner(*iter++);
        }
    }

private:
    std::set<ThreadPtr>& m_threads;
};

//////////////////////////////////////////////////////////////////////////
/// class ThreadPool
class THREADING_API ThreadPool : public Threading::Shared, public Threading::Monitor<Threading::Mutex>
{
    friend class TaskThread;
public:
    ThreadPool(int threadnum = 2, const std::string& poolname = "", const std::string& logger_file = "");

    ThreadPool(const std::string& properties_file, const std::string& logger_file = "", const std::string& poolname = "");

    ~ThreadPool(void);

    void Reset();

    void SubmitTask(const TaskPtr& task);

    TaskPtr SubmitTask(void (*fun)(void *), void *param = NULL);

    void JoinAll();

    void SetThreadIdleTime(const Time& idle_time);

private:
    //Note that the order of declaration of the members is important:
    //   both the m_destroyed flag and the m_tasksqueue must be declared before the m_threads vector,
    //   which must in turn be declared before the m_joiner. This ensures that the members are
    //   destroyed in the right order; you can't destroy the queue safely until all the threads
    //   have stopped, for example.
    AtomicBool m_destroyed;
    ThreadSafeQueue<TaskBase> m_tasksqueue;
    std::set<ThreadPtr> m_threads;                   // All threads, running or not.
    JoinThreads m_joiner;

    const size_t m_size;        // Number of threads that are pre-created.
    const size_t m_sizemax;     // Maximum number of threads.
    const size_t m_sizewarn;    // If m_inuse reaches m_sizeWarn, a "low on threads" warning will be printed.
    AtomicInt m_inuse;          // Number of threads that are currently in use.

    const bool m_waitifnotask;      // If the task queue has no task to execute, let idle thread wait.
    const bool m_haspriority;
    const Int64 m_shrinkcycletime;   // sleep time(second) of shrink thread.
    const Int64 m_threadidletime;    // idle thread wait time(millisecond).
    const int m_priority;
    const size_t m_stacksize;

    PropertiesPtr m_properties;
    LoggerPtr m_logger;
    std::string m_poolname;
};

typedef Threading::SharedPtr<ThreadPool> ThreadPoolPtr;

//////////////////////////////////////////////////////////////////////////
/// class TaskThread
class TaskThread : virtual public Shared, virtual public Thread
{
    friend class IdleThreadShrinker;
public:
    TaskThread(ThreadPool& thread_pool) : 
        m_idle(false),
        m_destroyed(false),
        m_threadpool(thread_pool)
    {
        SetNoDelete(true);
        if (m_threadpool.m_haspriority)
        {
            Start(m_threadpool.m_stacksize, m_threadpool.m_priority);
        }
        else
        {
            Start(m_threadpool.m_stacksize);
        }
        SetNoDelete(false);
    }

protected:

    virtual void Run()
    {
        while (!m_destroyed && !m_threadpool.m_destroyed)
        {
            Threading::SharedPtr<TaskBase> task;
            if (m_threadpool.m_waitifnotask)
            {
                task = m_threadpool.m_tasksqueue.TimedPop(Threading::Time::MilliSeconds(m_threadpool.m_threadidletime));
            }
            else
            {
                task = m_threadpool.m_tasksqueue.TryPop();
            }

            if (task)
            {
                m_idle = false;

                try
                {
                    task->Run();
                    task->NotifyDone();        // notify the task has completed if the customer is waiting.

                    --m_threadpool.m_inuse;
                }
                catch (...)
                {
                    task->NotifyDone();        // notify the task has completed if the customer is waiting.

                    --m_threadpool.m_inuse;

                    Error out(m_threadpool.m_logger);
                    out << "task: " << typeid(task).name() <<" in thread pool `" 
                        << m_threadpool.m_poolname << "' is running error, "
                        << "please check the thask's Run() method\n";
                }
            }
            else
            {
                m_idle = true;
                GetThreadControl().Yield();
            }
        }
    }

private:
    bool m_idle;
    bool m_destroyed;
    ThreadPool& m_threadpool;
};

THREADING_END

#endif