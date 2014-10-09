// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_RUNNABLE_H
#define CONCURRENCY_RUNNABLE_H

#include <Config.h>
#include <Util/Shared.h>

THREADING_BEGIN

class Runnable : virtual public Threading::Shared
{
public:
    Runnable(void) {}
    virtual ~Runnable(void) {}

    virtual void Run() = 0;
};

THREADING_END

#endif