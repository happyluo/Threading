// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_RUNNABLE_H
#define CONCURRENCY_RUNNABLE_H

#include <Concurrency/Config.h>
#include <Util/Shared.h>

CONCURRENCY_BEGIN

class Runnable : virtual public Util::Shared
{
public:
	Runnable(void) {}
	virtual ~Runnable(void) {}

	virtual void Run() = 0;
};

CONCURRENCY_END

#endif