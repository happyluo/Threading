// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_LATCH_H
#define CONCURRENCY_LATCH_H

#include <Concurrency/Config.h>
#include <Concurrency/Cond.h>
#include <Concurrency/Mutex.h>
#include <Util/Atomic.h>
#include <Util/ScopedGuard.h>

CONCURRENCY_BEGIN

// A Latch allows one or more threads to block until an operation is
// completed. A Latch is initialized with a count value. Calls to count_down()
// will decrement this count. Calls to Wait() will block until the count
// reaches zero. All calls to count_down() happen before any call to Wait()
// returns.
class Latch : public noncopyable
{
public:

	// Creates a new Latch with the given count.
	explicit Latch(int count);

	~Latch();

	void arrive();

	void arrive_and_wait();

	void count_down(int n);

	void Wait();

	bool try_wait();

#ifdef HAS_CXX11_RVREF
	// Creates a ScopedGuard that will invoke arrive, Wait, or
	// arrive_and_wait on this Latch when it goes out of scope.
	ScopedGuard arrive_guard();
	ScopedGuard wait_guard();
	ScopedGuard arrive_and_wait_guard();
#endif

private:
	// The counter for this Latch.
	int m_count;

	// Counts the number of threads that are currently waiting
#ifdef LANG_CPP11
	std::atomic_int m_waiting;
#else
	AtomicInt m_waiting;
#endif

	// The condition that blocks until the count reaches 0
	Cond m_condvar;
	Util::Mutex m_condvarMutex;

#ifdef LANG_CPP11
	// Disallow copy and assign
	Latch(const Latch&) =delete;
	Latch& operator=(const Latch&) =delete;
#endif
};

CONCURRENCY_END

#endif  // CONCURRENCY_LATCH_H
