// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_CONCURRENCY_H
#define CONCURRENCY_CONCURRENCY_H

//
// This file must include *all* other headers of Concurrency.
//
#include <Concurrency/AbstractMutex.h>
#include <Concurrency/Config.h>
#include <Concurrency/Cond.h>
#include <Concurrency/CountDownLatch.h>
#include <Concurrency/Lock.h>
#include <Concurrency/Monitor.h>
#include <Concurrency/Mutex.h>
#include <Concurrency/MutexPtrLock.h>
#include <Concurrency/RWRecMutex.h>
#include <Concurrency/RecMutex.h>
#include <Concurrency/StaticMutex.h>
#include <Concurrency/Singleton.h>
#include <Concurrency/SpinMutex.h>
#include <Concurrency/Thread.h>
#include <Concurrency/ThreadControl.h>
#include <Concurrency/ThreadException.h>
#include <Concurrency/ThreadPool.h>
#include <Concurrency/ThreadLocal.h>
#include <Concurrency/Timer.h>

#include <Concurrency//MapReduce/BufferQueue.h>
#include <Concurrency//MapReduce/ClosedError.h>
#include <Concurrency//MapReduce/ConcurrentPriorityQueue.h>
#include <Concurrency//MapReduce/IteratorQueue.h>
#include <Concurrency//MapReduce/MapReduce.h>
#include <Concurrency//MapReduce/MapReduceHelpers.h>
#include <Concurrency//MapReduce/MapReduceImpl.h>
#include <Concurrency//MapReduce/MutableThread.h>
#include <Concurrency//MapReduce/QueueBase.h>
#include <Concurrency//MapReduce/SerialExecutor.h>
#include <Concurrency//MapReduce/SimpleThreadPool.h>
#include <Concurrency//MapReduce/StdThread.h>
#include <Concurrency//MapReduce/StreamMutex.h>
//#include <Concurrency//MapReduce/SystemError.h>

#endif