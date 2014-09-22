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

	m_scheduleTasks.insert(ScheduleTask(task, scheduleTime/*, Time()*/));

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

	//Time now = Time::Now(Time::Monotonic);
	//Time scheduleTime = now + delaytime;
	//if (delaytime > Time() && scheduleTime < now)
	//{
	//	throw IllegalArgumentException(__FILE__, __LINE__, "invalid delay");
	//}

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

	// 调度任务列表(m_scheduleTasks)为空 或 挂起的任务尚在等待状态
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

	// 从m_scheduleTasks中删除元素是需用到调度时间(m_scheduledtime)和任务对象指针
	m_scheduleTasks.erase(ScheduleTask(task, iter->second, Time()));
	m_alltasks.erase(iter);

	return true;
}

/// 主线程函数，实现任务调度和等待
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
						//scheduleTask.m_scheduledtime = repeateBaseTime + scheduleTask.m_delay;
						iter->second = scheduleTask.m_scheduledtime;
						m_scheduleTasks.insert(scheduleTask);
					}
				}
		
				scheduleTask = ScheduleTask();

				if (m_scheduleTasks.empty())
				{
					m_taskWakeUpTime = Time();
					m_monitor.Wait();		// 调度任务列表中没有任务，等待用户添加任务或取消Timer
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
				//repeateBaseTime = Time::Now(Time::Monotonic);
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
	//Monitor<Util::Mutex>::LockGuard sync(m_monitor);
	//if (m_destroyed)
	//{
	//	throw IllegalArgumentException(__FILE__, __LINE__, "timer destroyed");
	//}

	scheduleTask = ScheduleTask();

	while (!m_scheduleTasks.empty() && !m_destroyed)
	{
		const ScheduleTask& firstTask = *m_scheduleTasks.begin();
		Time now = Time::Now(Time::Monotonic);

		/// 1. 检测延时时间是否到达，如果到达，则允许任务执行
		if (firstTask.m_scheduledtime <= now)
		{
			scheduleTask = firstTask;
			m_scheduleTasks.erase(m_scheduleTasks.begin());		// 从调度列表中删除已被调度的任务
			if (firstTask.m_delay == Time())			// 如果是单次(once-only)执行任务,一旦被调度，即从任务列表中删除
			{
				m_alltasks.erase(scheduleTask.m_task);
			}
			
			return true;	// 有任务等待超时，则允许其执行
		}

		///
		/// 任务等待时间未到，则执行等待操作
		///

		// 记录等待任务最晚被唤醒(等待超时)的时间，在此期间允许其他线程获取锁并执行
		m_taskWakeUpTime = firstTask.m_scheduledtime;		

		// 2. 等待，如果超时，或用户对m_monitor调用Notify操作，则执行新一轮调度，
		//    重新调度时，选择队列m_scheduleTasks中 m_scheduledtime < m_taskWakeUpTime 的任务先执行.
		//    因为m_scheduleTasks中的任务是按照调度时间排序的，每次调度都会选时间最小的任务执行
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