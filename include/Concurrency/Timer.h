// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_TIMER_H
#define CONCURRENCY_TIMER_H

#include <set>
#include <map>
#include <Util/Time.h>
#include <Util/Shared.h>
#include <Concurrency/Thread.h>
#include <Concurrency/Monitor.h>

namespace Util
{

class CONCURRENCY_API TimerTask : virtual public Shared
{
public:
    virtual ~TimerTask() {}

    virtual void RunTimerTask() = 0;
};
typedef SharedPtr<TimerTask> TimerTaskPtr;

//////////////////////////////////////////////////////////////////////////
/// class  TimerTaskCompare
class TimerTaskCompare : public std::binary_function<TimerTaskPtr, TimerTaskPtr, bool>
{
public:

    bool operator()(const TimerTaskPtr& lhs, const TimerTaskPtr& rhs) const
    {
        return lhs.Get() < rhs.Get();
    }
};

//////////////////////////////////////////////////////////////////////////
/// struct ScheduledTask
struct ScheduleTask
{
    TimerTaskPtr    m_task;    
    Time            m_delay;
    Time            m_scheduledtime;  

    inline ScheduleTask();

    inline ScheduleTask(const TimerTaskPtr& task, const Time& scheduledtime, const Time& delay = Time());

    inline bool operator <(const ScheduleTask& rhs) const;
};


class CONCURRENCY_API Timer : virtual public Shared, virtual private Thread
{
public:

    Timer();

    Timer(int priority);

    virtual ~Timer();

    void Destroy();

    void Schedule(const TimerTaskPtr& task, const Time& delaytime);

    void ScheduleRepeated(const TimerTaskPtr& task, const Time& delaytime, const Time& basetime = Time::Now(Time::Monotonic));

    bool Cancel(const TimerTaskPtr& task);

private:

    virtual void Run();

    bool doSchedule(ScheduleTask& scheduleTask);

    bool m_destroyed;

    Time    m_taskWakeUpTime;

    Monitor<Util::Mutex>    m_monitor;

    std::set<ScheduleTask>    m_scheduleTasks;

    std::map<TimerTaskPtr, Time, TimerTaskCompare> m_alltasks;
};
typedef Util::SharedPtr<Timer> TimerPtr;

inline ScheduleTask::ScheduleTask() :
    m_task(0), m_scheduledtime(Time()), m_delay(Time())
{

}

inline ScheduleTask::ScheduleTask(const TimerTaskPtr& task, const Time& scheduledtime, const Time& delay) :
    m_task(task), m_scheduledtime(scheduledtime), m_delay(delay)
{
}

inline bool ScheduleTask::operator <(const ScheduleTask& rhs) const
{
    if (m_scheduledtime < rhs.m_scheduledtime)
    {
        return true;
    }
    else if (m_scheduledtime > rhs.m_scheduledtime)
    {
        return false;
    }

    return m_task.Get() < rhs.m_task.Get();
}

}


#endif