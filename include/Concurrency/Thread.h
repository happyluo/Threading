// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_THREAD_H
#define CONCURRENCY_THREAD_H

#include <string>
#include <Util/Shared.h>
#include <Util/SharedPtr.h>
#include <Concurrency/Mutex.h>
#include <Concurrency/ThreadControl.h>

namespace Util
{

class CONCURRENCY_API Thread : virtual public Shared, public noncopyable
{
public:
	Thread(void);
	Thread(const std::string& name);
	virtual ~Thread(void);

	virtual void Run() = 0;

	ThreadControl Start(size_t stack_size = 0);
	ThreadControl Start(size_t stack_size, int priority);

	ThreadControl GetThreadControl() const;

	bool operator ==(const Thread&) const;
	bool operator !=(const Thread&) const;
	bool operator <(const Thread&) const;

	bool IsAlive() const;

	const std::string& Name() const;

	static unsigned HardwareConcurrency() UTIL_NOEXCEPT;

	void _done();

protected:
	const std::string m_name;
	Util::Mutex m_statemutex;
	bool m_started;
	bool m_running;

#ifdef LANG_CPP11
	std::shared_ptr<std::thread> m_thread;
#elif defined(_WIN32)
	HANDLE m_thread;
	DWORD  m_id;
#else
	pthread_t m_thread;
#endif

private:

#ifdef _WIN32
	// nothing
#else
	ThreadControl Start(size_t stack_size, bool realtime_scheduling, int priority);
#endif

	//Thread(const Thread&);              // Copying is forbidden
	//void operator=(const Thread&);      // Assignment is forbidden

};

typedef SharedPtr<Thread> ThreadPtr;

}

#endif