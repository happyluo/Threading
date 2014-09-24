// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Concurrency/Timer.h>
#include <Concurrency/ThreadException.h>

using namespace std;
using namespace Util;

Timer::Timer(void) : Thread("Util timer thread"), m_destroyed(false)
{
    SetNoDelete(true);
    Start();
    SetNoDelete(false);
}

Timer::Timer(int priority) : Thread("Util timer thread"), m_destroyed(false)
{
    SetNoDelete(true);
    Start(0, priority);
    SetNoDelete(false);
}

Timer::~Timer()
{
    Destroy();        
}

void Timer::Destroy()
{
    {
        Monitor<Util::Mutex>::LockGuard sync(m_monitor);
        if (m_destroyed)
        {
            return;
        }
        m_destroyed = true;
        m_monitor.Notify();
        m_alltasks.clear();
        m_scheduleTasks.clear();
    }

    if (GetThreadControl() == ThreadControl())
    {
        GetThreadControl().Detach();
    }
    else
    {
        GetThreadControl().Join();
    }
}

void Timer::Schedule(const TimerTaskPtr& task, const Time& delaytime)
{
    Monitor<Util::Mutex>::LockGuard sync(m_monitor);
    if (m_destroyed)
    {
        throw IllegalArgumentException(__FILE__, __LINE__, "timer destroyed");
    }

    Time now = Time::Now(Time::Monotonic);
    Time scheduleTime = now + delaytime;
    if (delaytime > Time() && scheduleTime < now)
    {
        throw IllegalArgumentException(__FILE__, __LINE__, "invalid delay");
    }

    bool inserted = m_alltasks.insert(std::make_pair(task, scheduleTime)).second;
    if (!inserted)
    {
        throw IllegalArgumentException(__FILE__, __LINE__, "task is already schedulded");
    }

    m_scheduleTasks.insert(ScheduleTask(task, scheduleTime));

    if (m_taskWakeUpTime == Time() || scheduleTime < m_taskWakeUpTime)
    {
        m_monitor.Notify();
    }
}

void Timer::ScheduleRepeated(const TimerTaskPtr& task, const Time& delaytime, const Time& basetime)
{
    Monitor<Util::Mutex>::LockGuard sync(m_monitor);
    if (m_destroyed)
    {
        throw IllegalArgumentException(__FILE__, __LINE__, "timer destroyed");
    }

    Time scheduleTime = basetime + delaytime;
    if (delaytime > Time() && scheduleTime < basetime)
    {
        throw IllegalArgumentException(__FILE__, __LINE__, "invalid delay");
    }

    bool inserted = m_alltasks.insert(std::make_pair(task, scheduleTime)).second;
    if (!inserted)
    {
        throw IllegalArgumentException(__FILE__, __LINE__, "task is already schedulded");
    }

    m_scheduleTasks.insert(ScheduleTask(task, scheduleTime, delaytime));

    if (m_taskWakeUpTime == Time() || scheduleTime < m_taskWakeUpTime)
    {
        m_monitor.Notify();
    }
}

bool Timer::Cancel(const TimerTaskPtr& task)
{
    Monitor<Util::Mutex>::LockGuard sync(m_monitor);
    if (m_destroyed)
    {
        return false;
    }

    std::map<TimerTaskPtr, Time, TimerTaskCompare>::iterator iter = m_alltasks.find(task);
    if (iter == m_alltasks.end())
    {
        return false;
    }

    m_scheduleTasks.erase(ScheduleTask(task, iter->second, Time()));
    m_alltasks.erase(iter);

    return true;
}

void Timer::Run()
{
    ScheduleTask scheduleTask;

    while (true)
    {
        //Time repeateBaseTime;
        {
            Monitor<Util::Mutex>::LockGuard sync(m_monitor);
            if (!m_destroyed)
            {
                if (Time() != scheduleTask.m_delay)
                {
                    std::map<TimerTaskPtr, Time, TimerTaskCompare>::iterator iter =
                        m_alltasks.find(scheduleTask.m_task);
                    if (iter != m_alltasks.end())
                    {
                        scheduleTask.m_scheduledtime = Time::Now(Time::Monotonic) + scheduleTask.m_delay;
                        iter->second = scheduleTask.m_scheduledtime;
                        m_scheduleTasks.insert(scheduleTask);
                    }
                }
        
                scheduleTask = ScheduleTask();

                if (m_scheduleTasks.empty())
                {
                    m_taskWakeUpTime = Time();
                    m_monitor.Wait();
                }
            }

            if (m_destroyed)
            {
                break;
            }

            if (!doSchedule(scheduleTask))
            {
                continue;
            }
        }

        if (m_destroyed)
        {
            break;
        }

        if (0 != scheduleTask.m_task)
        {
            try
            {
                scheduleTask.m_task->RunTimerTask();
            }
            catch(const Exception& e)
            {
                cerr << "Util::Timer::Run(): uncaught exception:\n" << e.what();
                cerr << endl;
            } 
            catch(const std::exception& e)
            {
                cerr << "Util::Timer::Run(): uncaught exception:\n" << e.what() << endl;
            } 
            catch(...)
            {
                cerr << "Util::Timer::Run(): uncaught exception" << endl;
            }
        }
    }    
}

bool Timer::doSchedule(ScheduleTask& scheduleTask)
{
    scheduleTask = ScheduleTask();

    while (!m_scheduleTasks.empty() && !m_destroyed)
    {
        const ScheduleTask& firstTask = *m_scheduleTasks.begin();
        Time now = Time::Now(Time::Monotonic);

        if (firstTask.m_scheduledtime <= now)
        {
            scheduleTask = firstTask;
            m_scheduleTasks.erase(m_scheduleTasks.begin());  
            if (firstTask.m_delay == Time()) 
            {
                m_alltasks.erase(scheduleTask.m_task);
            }
            
            return true; 
        }

        m_taskWakeUpTime = firstTask.m_scheduledtime;        

        try
        {
            m_monitor.TimedWait(firstTask.m_scheduledtime - now);    
        }
        catch (const InvalidTimeoutException&)
        {
            Time timeout = (firstTask.m_scheduledtime - now) / 2;
            while (timeout > Time())
            {
                try
                {
                    m_monitor.TimedWait(timeout);
                    break;    
                }
                catch (const InvalidTimeoutException&)
                {
                    timeout /= 2;
                }
            }
        }
    }

    return false;
}