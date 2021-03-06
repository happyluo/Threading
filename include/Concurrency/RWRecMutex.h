// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************
#ifndef CONCURRENCY_RW_REC_MUTEX_H
#define CONCURRENCY_RW_REC_MUTEX_H

#include <Util/Time.h>
#include <Config.h>
#include <Concurrency/ThreadControl.h>
#include <Concurrency/Cond.h>
#include <Concurrency/Mutex.h>

THREADING_BEGIN

class THREADING_API RWRecMutex : public noncopyable
{
public:
    RWRecMutex(void);
    ~RWRecMutex(void);

    void ReadLock() const;

    bool TryReadLock() const;

    bool TimedReadLock(const Time& timeout) const;


    void WriteLock() const;

    bool TryWriteLock() const;

    bool TimedWriteLock(const Time& timeout) const;

    void Unlock() const;

    void Upgrade() const;

    bool TimedUpgrade(const Time& timeout) const;

    void Downgrade() const;

private:
    
    // noncopyable
    //RWRecMutex(const RWRecMutex&);
    //void operator=(const RWRecMutex&);
    mutable int    m_count;

    mutable ThreadControl m_writerThreadId;

    mutable unsigned int m_waitingWriterNum;

    mutable bool m_upgrading;

    Mutex m_mutex;

    mutable Cond m_readers;
    mutable Cond m_writers;
    mutable Cond m_upgradeReader;
};

THREADING_END

#endif