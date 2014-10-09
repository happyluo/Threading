// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_SPIN_MUTEX_H
#define CONCURRENCY_SPIN_MUTEX_H

#include <Config.h>

THREADING_BEGIN

class THREADING_API SpinMutex
{
public:
    void Lock() UTIL_NOEXCEPT;
    void Unlock() UTIL_NOEXCEPT;

private:
    UTIL_CONSTEXPR SpinMutex(void*) UTIL_NOEXCEPT;
    SpinMutex(const SpinMutex&);
    SpinMutex& operator =(const SpinMutex&);

    void* m_pmutex;

    friend THREADING_API SpinMutex& GetSpinMutex(const void*);
};

THREADING_API SpinMutex& GetSpinMutex(const void*);

THREADING_END

#endif