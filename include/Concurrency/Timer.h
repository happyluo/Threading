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
/// �κ���Ҫʹ��Timer���е��ȵ�����Ҫ�Ӵ�����������ʵ�� RunTimerTask �ӿڡ�
/// ����������Ĳ�����ͨ�����캯���� Set ���������񱻵���֮ǰ�������á�
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
	/// ��ִ�е�����
	TimerTaskPtr	m_task;	
	/// �����´α�ִ��֮ǰ��ȴ���ʱ����
	/// ��ֵ�����ظ�ִ�е�����(Repeate tasks)��Ч����ִֻ��һ�ε����񣬴�ֵΪ��0(Time()))
	Time				m_delay;
	/// �����´α�����ִ�е�ʱ��(����ʱ��)
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

	// ����Timer���������������Timer������߳������(detach)ִ���̣߳�
	// ����������������̣߳���Join Timer��ִ���̣߳�ֱ�������
	// ��Timer ������Ҫʱ����Ҫ�û��ֶ����ô˽ӿ�����Timer ����
	void Destroy();

	// �ڸ����ĵȴ�ʱ��(waittime)֮��ʼִ�е���ִ�� task  ��Ӧ������
	// task ֻ����һ��
	void Schedule(const TimerTaskPtr& task, const Time& delaytime);

	// ��ÿ��ִ��֮��ȴ�waittime ��ʼִ�е���ִ�� task  ��Ӧ������
	// task ���ظ�����
	void ScheduleRepeated(const TimerTaskPtr& task, const Time& delaytime, const Time& basetime = Time::Now(Time::Monotonic));

	// ȡ�����������������δ��ִ�л��Ǳ�ѭ��ִ�е������򷵻�true��
	// ����˺������ظ����ã��������Ѿ�ִ��(��������once-only)������ָ���˲��ᱻ���ȵ��������򷵻� false
	// �����ȡ������������ִ�У������������أ�����������ִ����ϡ�
	bool Cancel(const TimerTaskPtr& task);

private:

	/// ���̺߳�����ʵ��������Ⱥ͵ȴ�
	virtual void Run();

	/// ������Ⱥ͵ȴ�����
	/// ���ô˺���֮ǰ�������� Monitor 
	bool doSchedule(ScheduleTask& scheduleTask);

	/// Destroy �Ƿ�ִ��
	bool m_destroyed;

	/// ����ָ����е�ʱ��㣬�ڴ�֮ʱ�䵽��֮ǰ�������ȴ��������ִ��
	Time	m_taskWakeUpTime;

	/// ����ʵ�ֱ������������ʱ�ȴ���ֱ���䳬ʱ
	Monitor<Util::Mutex>	m_monitor;

	/// ���ڱ����ȵ��������(���ô洢ԭʼ�����map,��Ϊ��ʵ�ֹ�ƽ��, ���б��е������ǰ��յ���ʱ�������)��
	/// ����һ�������ȣ���Ӵ��б���ɾ��,����������ִ�е�����,�´ε���֮ǰ���������ʱ�䣬�ټ����б�
	std::set<ScheduleTask>	m_scheduleTasks;

	/// ��Ҫ�����ȵ������б��������������Ӻ�ɾ����
	/// ���ڵ���(once-only)ִ�е�����,һ�������ȣ����Ӵ��б���ɾ��
	/// ����������ִ�е����񣬳����û��������� Cancle �� Destroy, ����ʼ�ձ����ڴ˶����У�ֱ��Timer��������
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