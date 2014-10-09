// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Concurrency/RWRecMutex.h>

Threading::RWRecMutex::RWRecMutex(void) :
    m_count(0),
    m_waitingWriterNum(0),
    m_upgrading(false)
{
}

Threading::RWRecMutex::~RWRecMutex(void)
{
}

void Threading::RWRecMutex::ReadLock() const
{
    Threading::Mutex::LockGuard sync(m_mutex);

    while (m_count < 0 || m_waitingWriterNum != 0)
    {
        m_readers.Wait(sync);
    }

    ++m_count;
}

bool Threading::RWRecMutex::TryReadLock() const
{
    Threading::Mutex::LockGuard sync(m_mutex);

    if (m_count < 0 || m_waitingWriterNum != 0)
    {
        return false;
    }

    ++m_count;
    return true;
}

bool Threading::RWRecMutex::TimedReadLock(const Time& timeout) const
{
    Threading::Mutex::LockGuard sync(m_mutex);

    Time end = Time::Now(Time::Monotonic) + timeout;
    while (m_count < 0 || m_waitingWriterNum != 0)
    {
        Time remainder = end - Time::Now(Time::Monotonic);
        if (remainder > Time())
        {
            if (false == m_readers.TimedWait(sync, remainder))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    ++m_count;
    return true;
}

void Threading::RWRecMutex::WriteLock() const
{
    Threading::Mutex::LockGuard sync(m_mutex);

    if (m_count < 0 && m_writerThreadId == ThreadControl())
    {
        --m_count;
        return;
    }

    while (0 != m_count)
    {
        ++m_waitingWriterNum;
        try
        {
            m_writers.Wait(sync);
        }
        catch (...)
        {
            --m_waitingWriterNum;
            throw;
        }
        --m_waitingWriterNum;
    }

    m_count = -1;
    m_writerThreadId = ThreadControl();
    return;
}

bool Threading::RWRecMutex::TryWriteLock() const
{
    Threading::Mutex::LockGuard sync(m_mutex);

    if (m_count < 0 && m_writerThreadId == ThreadControl())
    {
        --m_count;
        return true;
    }

    if (0 != m_count)
    {
        return false;
    }

    --m_count;
    m_writerThreadId = ThreadControl();
    return true;
} 

bool Threading::RWRecMutex::TimedWriteLock(const Time& timeout) const
{
    Threading::Mutex::LockGuard sync(m_mutex);

    if (m_count < 0 && m_writerThreadId == ThreadControl())
    {
        --m_count;
        return true;
    }

    Time end = Time::Now(Time::Monotonic) + timeout;
    while (0 != m_count)
    {
        Time remainder = end - Time::Now(Time::Monotonic);
        if (remainder > Time())
        {
            ++m_waitingWriterNum;
            try
            {
                if (false == m_writers.TimedWait(sync, timeout))
                {
                    --m_waitingWriterNum;
                    return false;
                }
            }
            catch (...)
            {
                --m_waitingWriterNum;
                throw;
            }
            
            --m_waitingWriterNum;
        }
        else
        {
            return false;
        }
    }

    --m_count;
    m_writerThreadId = ThreadControl();
    return true;
}

void Threading::RWRecMutex::Unlock() const
{
    bool writerWaiting = false;
    bool readerWaiting = false;

    {
        Threading::Mutex::LockGuard sync(m_mutex);
        assert(0 != m_count);

        if (m_count < 0) 
        {
            ++m_count;

            if (m_count < 0)
            {
                return;
            }
        }
        else                      
        {
            --m_count;
        }

        writerWaiting = (0 == m_count && m_waitingWriterNum > 0);
        readerWaiting = (0 == m_waitingWriterNum);
    }

    if (writerWaiting)
    {
        if (m_upgrading)
        {
            m_upgradeReader.Signal();       
        }
        else
        {
            m_writers.Signal();
        }
    }
    else if (readerWaiting)
    {
        m_readers.Broadcast();
    }
}

void Threading::RWRecMutex::Upgrade() const
{
    Threading::Mutex::LockGuard sync(m_mutex);

    if (true == m_upgrading)
    {
        throw DeadlockException(__FILE__, __LINE__);
    }

    assert(m_count > 0);
    --m_count;      

    m_upgrading = true;
    while (0 != m_count)
    {
        ++m_waitingWriterNum;
        
        try
        {
            m_upgradeReader.Wait(sync);
        }
        catch (...)
        {
            m_upgrading = false;
            --m_waitingWriterNum;
            ++m_count;
            throw;
        }

        --m_waitingWriterNum;
    }

    m_count = -1;
    m_writerThreadId = ThreadControl();
    m_upgrading = false;
    return;
}


bool Threading::RWRecMutex::TimedUpgrade(const Time& timeout) const
{
    Threading::Mutex::LockGuard sync(m_mutex);

    if (true == m_upgrading)
    {
        return false;
    }

    assert(m_count > 0);
    --m_count;

    m_upgrading = true;
    Time end = Time::Now(Time::Monotonic) + timeout;
    while (0 != m_count)
    {
        Time remainder = end - Time::Now(Time::Monotonic);
        if (remainder > Time())
        {
            ++m_waitingWriterNum;
            try
            {
                if (false == m_upgradeReader.TimedWait(sync, remainder))
                {
                    ++m_count;
                    m_upgrading = false;
                    --m_waitingWriterNum;
                    return false;
                }
            }
            catch (...)
            {
                ++m_count;
                m_upgrading = false;
                --m_waitingWriterNum;
                throw;
            }
        }
        else
        {
            ++m_count;
            m_upgrading = false;
            return false;
        }

        --m_waitingWriterNum;
    }

    m_count = -1;
    m_upgrading = false;
    m_writerThreadId = ThreadControl();

    return true;
}

void Threading::RWRecMutex::Downgrade() const
{
    Threading::Mutex::LockGuard sync(m_mutex);

    if (-1 == m_count)        
    {
        m_count = 1;
    }
}