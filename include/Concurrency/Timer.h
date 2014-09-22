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
#include <Concurrency/Thread.h>
#include <Concurrency/Monitor.h>
#include <Util/Shared.h>

namespace Util
{

//////////////////////////////////////////////////////////////////////////
/// class  TimerTask
///
/// 任何想要使用Timer进行调度的任务都要从此类派生，并实现 RunTimerTask 接口。
/// 运行是所需的参数可通过构造函数或 Set 方法在任务被调度之前进行设置。
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
	/// 待执行的任务
	TimerTaskPtr	m_task;	
	/// 任务下次被执行之前需等待的时间间隔
	/// 此值对需重复执行的任务(Repeate tasks)有效，对只执行一次的任务，此值为：0(Time()))
	Time				m_delay;
	/// 任务下次被调度执行的时刻(绝对时间)
	Time				m_scheduledtime;		// = Time::Now(Time::Monotonic) + m_delay;

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

	// 销毁Timer对象，如果调用者是Timer自身的线程则分离(detach)执行线程；
	// 如果调用者是其他线程，则Join Timer的执行线程，直至其结束
	// 当Timer 不再需要时，需要用户手动调用此接口销毁Timer 对象。
	void Destroy();

	// 在给定的等待时间(waittime)之后开始执行调度执行 task  对应的任务
	// task 只调度一次
	void Schedule(const TimerTaskPtr& task, const Time& delaytime);

	// 在每次执行之间等待waittime 后开始执行调度执行 task  对应的任务
	// task 被重复调度
	void ScheduleRepeated(const TimerTaskPtr& task, const Time& delaytime, const Time& basetime = Time::Now(Time::Monotonic));

	// 取消给定任务。如果任务未被执行或是被循环执行的任务，则返回true；
	// 如果此函数被重复调用，或任务已经执行(单次任务once-only)，或是指定了不会被调度到的任务，则返回 false
	// 如果待取消的任务正在执行，则函数立即返回，并允许任务执行完毕。
	bool Cancel(const TimerTaskPtr& task);

private:

	/// 主线程函数，实现任务调度和等待
	virtual void Run();

	/// 任务调度和等待函数
	/// 调用此函数之前必须锁定 Monitor 
	bool doSchedule(ScheduleTask& scheduleTask);

	/// Destroy 是否被执行
	bool m_destroyed;

	/// 任务恢复运行的时间点，在此之时间到达之前，其他等待任务可以执行
	Time	m_taskWakeUpTime;

	/// 用来实现被调度任务的延时等待，直至其超时
	Monitor<Util::Mutex>	m_monitor;

	/// 正在被调度的任务队列(不用存储原始任务的map,是为了实现公平性, 此列表中的任务是按照调度时间排序的)。
	/// 任务一旦被调度，则从此列表中删除,对于需周期执行的任务,下次调度之前更新其调度时间，再加入列表
	std::set<ScheduleTask>	m_scheduleTasks;

	/// 需要被调度的任务列表，并控制任务的添加和删除。
	/// 对于单次(once-only)执行的任务,一旦被调度，即从此列表中删除
	/// 对于需周期执行的任务，除非用户主动调用 Cancle 或 Destroy, 否则将始终保存在此队列中，直至Timer对象销毁
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