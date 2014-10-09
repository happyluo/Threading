// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_ATOMIC_H
#define UTIL_ATOMIC_H

#include <Config.h>
#ifdef LANG_CPP11
#    include <atomic>
#else
#    include <Concurrency/Mutex.h>

THREADING_BEGIN

class THREADING_API AtomicBool : public Threading::Mutex
{
public:
    AtomicBool(bool value = false) : m_value(value)
    {
    }

    bool operator =(bool value)
    {
        Threading::Mutex::LockGuard sync(*this);
        m_value = value;
        return m_value;
    }

    operator bool() const
    {
        Threading::Mutex::LockGuard sync(*this);
        return m_value;
    }

private:
    volatile bool m_value;
};

class THREADING_API AtomicInt : public Threading::Mutex
{
public:
    AtomicInt(int value = 0) : m_value(value)
    {
    }

    int operator =(int value)
    {
        Threading::Mutex::LockGuard sync(*this);
        m_value = value;
        return m_value;
    }

    int operator ++()        
    {
        Threading::Mutex::LockGuard sync(*this);
        ++m_value;
        return m_value;
    }

    int operator --()            
    {
        Threading::Mutex::LockGuard sync(*this);
        --m_value;
        return m_value;
    }

    int operator ++(int)        
    {
        Threading::Mutex::LockGuard sync(*this);
        int ret = m_value;
        ++m_value;
        return ret;
    }

    int operator --(int)
    {
        Threading::Mutex::LockGuard sync(*this);
        int ret = m_value;
        --m_value;
        return ret;
    }

    int operator +=(int rhs)
    {
        Threading::Mutex::LockGuard syncl(*this);
        m_value += rhs;
        return m_value;
    }

    int operator -=(int rhs)
    {
        Threading::Mutex::LockGuard syncl(*this);
        m_value -= rhs;
        return m_value;
    }
    
    int operator *=(int rhs)
    {
        Threading::Mutex::LockGuard syncl(*this);
        m_value *= rhs;
        return *this;
    }

    int operator /=(int rhs)
    {
        Threading::Mutex::LockGuard syncl(*this);
        m_value /= rhs;
        return m_value;
    }

    int operator %=(int rhs)
    {
        Threading::Mutex::LockGuard syncl(*this);
        m_value %= rhs;
        return m_value;
    }

    operator int() const
    {
        Threading::Mutex::LockGuard sync(*this);
        return m_value;
    }

private:
    volatile int m_value;
};

THREADING_END

#endif


#endif