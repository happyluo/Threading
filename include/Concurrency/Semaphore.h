// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_SEMAPHORE_H
#define CONCURRENCY_SEMAPHORE_H

#include <Util/Time.h>

#if defined(_WIN32)

THREADING_BEGIN

class Semaphore
{
public:
    Semaphore(long initialCount = 0);
    ~Semaphore();

    void Wait() const;            // P
    bool TimedWait(const Threading::Time&) const;    // P

    void Post(int releaseCount = 1) const;    // V

private:
    mutable HANDLE    m_sem;
};

THREADING_END

#endif

#endif