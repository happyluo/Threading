// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Concurrency/Cond.h>

#ifndef _WIN32
#    include <sys/time.h>
#endif

#if defined(LANG_CPP11)

Util::Cond::Cond(void) 
try : 
# if defined(USING_COND_ANY)
	m_cond(std::condition_variable_any())
# else
	m_cond(std::condition_variable());
# endif
{
}  
catch(const std::system_error& ex)
{
	throw ThreadSyscallException(__FILE__, __LINE__, ex.code().value());
}

Util::Cond::~Cond(void)
{
	
}

/// ������߳��ڵȴ��ź������򼤻�����һ���̣߳�ʹ�俪ʼִ��;������̵߳ȴ��ź������򲻽����κβ���
void Util::Cond::Signal()
{
	m_cond.notify_one();
}

/// �������еȴ����ź������߳�
void Util::Cond::Broadcast()
{
	m_cond.notify_all();
}

#elif defined(_WIN32)

#   ifdef HAS_WIN32_CONDVAR

Util::Cond::Cond()
{
	InitializeConditionVariable(&m_cond);
}

Util::Cond::~Cond()
{
}

void Util::Cond::Signal()
{
	WakeConditionVariable(&m_cond);
}

void Util::Cond::Broadcast()
{
	WakeAllConditionVariable(&m_cond);
}

#   else

Util::Cond::Cond(void) :
	m_condstate(Util::Cond::IDLE),
	m_blockednum(0),
	m_signaleddnum(0),
	m_gatesem(1)
{
}

Util::Cond::~Cond(void)
{
}

/// ������߳��ڵȴ��ź������򼤻�����һ���̣߳�ʹ�俪ʼִ��;������̵߳ȴ��ź������򲻽����κβ���
/// ע�����˺���������ʱ���κ������̶߳Դ����������ϵġ����źš�(signaling(in wake))�򡰵ȴ���(waiting to wait (in waitingToWait))�����ĵ���
/// ���ᱻ�ź���(m_gatesem)��������ֱ�����ź���m_worksem�ϵȴ�����ȷ��Ŀ(����(SINGLED)������(BROADCAST))���̴߳�Wait���أ�
/// ���ͷ��ź���(m_gatesem)(through postWait)��
void Util::Cond::Signal()
{
	wake(false);
}

/// �������еȴ����ź������߳�
void Util::Cond::Broadcast()
{
	wake(true);
}

void Util::Cond::wake(bool broadcast)
{
	// ����m_gatesem�������߳��ڵȴ����������������ź���(m_gatesem)��һֱ������
	// ֱ���ȴ��̴߳�postWait�����ͷ�
	m_gatesem.Wait();

	Util::Mutex::LockGuard syncSignal(m_signaledmutex);

	if (m_signaleddnum != 0)
	{
		m_blockednum -= m_signaleddnum;
		m_signaleddnum = 0;
	}

	//
	// If there are waiting threads then we enter a signal or
	// broadcast state.
	//
	if (0 < m_blockednum)		//> ���̵߳ȴ�
	{
		// ���һЩ�̵߳�����(����)
		assert(IDLE == m_condstate);
		m_condstate = broadcast ? BROADCAST : SIGNAL;

		// ��һ���ȴ��̻߳��ѡ��˲������ú�ȴ��߳̽������ѣ�����postWait�����м�����m_worksem��ִ��Post()
		// ���������ȴ��߳�(m_condstate == BROADCAST)������m_gatesem��ִ��Post()����������signaling��waiting to wait�̼߳���ִ��
		//
		// Posting the m_worksem wakes a single waiting thread. After this
		// occurs the waiting thread will wake and then either post on
		// the m_worksem to wake the next waiting thread, or post on the
		// gate to permit more signaling to proceed.
		//
		// Release before posting to avoid potential immediate
		// context switch due to the mutex being locked.
		//
		syncSignal.Release();
		m_worksem.Post();
	}
	else		//> ���̵߳ȴ�
	{
		// �˴� syncSignal �ֶ���ǰ��m_gatesem֮ǰ�ͷţ����������������Զ��ͷţ�
		// ��Ϊ�˱�������m_signaldmutex���������µ�Ǳ�ڵ�Ƶ�����������л�(����m_gatesem����
		// syncSignal֮ǰ��ȡ�����ģ����ͷ�ʱm_gatesem����syncSignal֮ǰ�����ᵼ�������߳�
		// ��ȡm_gatesem����Ȼ��ȡ����syncSignal���������Ӷ������߳��л�����������)
		//
		// Otherwise no blocked waiters, release the m_gatesem.
		//
		// Release before posting to avoid potential immediate
		// context switch due to the mutex being locked.
		//
		syncSignal.Release();
		m_gatesem.Post();		// allow waitingToWait to execute.
	}

}

void Util::Cond::doWait() const
{
	try
	{
		m_worksem.Wait();
		postWait(false);
	}
	catch (...)
	{
		postWait(true);
		throw;
	}
}

bool Util::Cond::timedDowait(const Time& timeout) const
{
	try
	{
		bool returnVal = m_worksem.TimedWait(timeout);
		postWait(!returnVal);
		return returnVal;
	}
	catch (...)
	{
		postWait(true);
		throw;
	}
}

/// ���ȴ��̼߳�����������
void Util::Cond::waitingToWait() const
{
	//
    // m_gatesem is used to protect m_blockednum. Furthermore, this prevents
    // further threads from entering the wait state while a
    // signal/broadcast is being processed.
    //
	m_gatesem.Wait();
	++m_blockednum;
	m_gatesem.Post();
}


/// �ȴ��߳̽��� postWait ʱ��������ڸ��������߳��豻����(BROADCAST), 
/// �ź���m_worksem��V(Post)�������ٴα����ã��Ի��������ȴ��̡߳�����������߳��豻���
/// ���ź���m_gatesem�ᱻ�ͷţ��Ӷ�����signaling��waiting to wait�̼߳���ִ��
void Util::Cond::postWait(bool timed_out_or_failed) const
{
	Util::Mutex::LockGuard syncSignal(m_signaledmutex);

	//
	// One more thread has unblocked.
	//
	++m_signaleddnum;

	//
	// If m_condstate is IDLE then this must be a timeout, otherwise its a
	// spurious wakeup which is incorrect.
	//
	if (IDLE == m_condstate)			//> �ȴ��߳�δ�յ�Signal() �� Broadcast()
	{
		assert(timed_out_or_failed);	//> δ�յ�Signal() �� Broadcast() һ�������ڳ�ʱ �� ������
		return;
	}

	//
	// m_condstate == SIGNAL / BROADCAST
	// 
	if (timed_out_or_failed) 
	{
		// ��������һ���������߳� �Ҵ���һ������������signal/broadcast
		//
        // If the thread was the last blocked thread and there's a
        // pending signal/broadcast, reset the signal/broadcast to
        // prevent spurious wakeup.
        //
		if (m_signaleddnum == m_blockednum)	
		{
			m_condstate = IDLE;

			// ִ�д˲�����Ϊ�˱���������������ٻ���(spurious wakeup)
			// ���ȴ���ʱʱ�������ղ���ͬһʱ�������߳��ͷ��ź���m_worksem(Post<V>)ʱ������֪ͨ��
			// �����ź���m_worksemδ������(consumed)��������spurious wakeup.
			//
			// Threads timing out present a particular issue because they may have
			// woken without a corresponding notification and its easy to leave
			// the m_worksem in a state where a spurious wakeup will occur --
			// consider a notify and a timed wake occuring at the same time. In
			// this case, if we are not careful the m_worksem will have been posted,
			// but the waking thread may not consume the semaphore.
			//
			// Consume the queue post to prevent spurious wakeup. Note
			// that although the m_signaledmutex mutex could be released
			// prior to this Wait() call, doing so gains nothing since
			// this Wait() MUST return immediately (if it does not
			// there is a major bug and the entire application will
			// deadlock).
			//
			m_worksem.Wait();

			syncSignal.Release(); 
			m_gatesem.Post();
		}
		// ���ڷ����һ����ʱ�̲߳������κβ�����ֱ�ӽ��份��ִ��
	}
	else
	{
		if (SIGNAL == m_condstate || m_signaleddnum == m_blockednum)	//> signal �򵽴����һ�������߳�
		{
			m_condstate = IDLE;	// ������������״̬

			// ��������signaling��waiting to wait�̻߳�ȡm_gatesem�ź�������ִ��
			syncSignal.Release();
			m_gatesem.Post();		
		}
		else		//> �㲥(braadcast)�����ж���߳��������ȴ�����
		{
			// ���������ȴ��߳�(m_condstate == BROADCAST)
			syncSignal.Release();
			m_worksem.Post();		// �ͷ��ź��������������̱߳�����
		}
	}
}

#    endif // HAS_WIN32_CONDVAR

#else

Util::Cond::Cond(void) 
{
	pthread_condattr_t condattr;

	int returnVal = pthread_condattr_init(&condattr);
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}

#if ! defined(__hpux) && ! defined(__APPLE__)
	returnVal = pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC); 
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}
#endif

	returnVal = pthread_cond_init(&m_cond, &condattr);
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}

	returnVal = pthread_condattr_destroy(&condattr);
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}
}

Util::Cond::~Cond(void)
{
#ifndef NDEBUG
	int returnVal = pthread_cond_destroy(&m_cond);
	assert(0 == returnVal);
	//if (0 != returnVal)
	//{
	//	throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	//}
#else
    pthread_cond_destroy(&m_cond);
#endif
}

/// ������߳��ڵȴ��ź������򼤻�����һ���̣߳�ʹ�俪ʼִ��;������̵߳ȴ��ź������򲻽����κβ���
void Util::Cond::Signal()
{
	int returnVal = pthread_cond_signal(&m_cond);
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}
}

/// �������еȴ����ź������߳�
void Util::Cond::Broadcast()
{
	int returnVal = pthread_cond_broadcast(&m_cond);
	if (0 != returnVal)
	{
		throw ThreadSyscallException(__FILE__, __LINE__, returnVal);
	}
}

#endif