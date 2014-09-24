// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_ATOMIC_H
#define UTIL_ATOMIC_H

#include <Util/Config.h>
#ifdef LANG_CPP11
#    include <atomic>
#else
#    include <Concurrency/Mutex.h>

namespace Util
{

class UTIL_API AtomicBool : public Util::Mutex
{
public:
    AtomicBool(bool value = false) : m_value(value)
    {
    }

    bool operator =(bool value)
    {
        Util::Mutex::LockGuard sync(*this);
        m_value = value;
        return m_value;
    }

    operator bool() const
    {
        Util::Mutex::LockGuard sync(*this);
        return m_value;
    }

private:
    volatile bool m_value;
};

class UTIL_API AtomicInt : public Util::Mutex
{
public:
    AtomicInt(int value = 0) : m_value(value)
    {
    }

    int operator =(int value)
    {
        Util::Mutex::LockGuard sync(*this);
        m_value = value;
        return m_value;
    }

    int operator ++()        
    {
        Util::Mutex::LockGuard sync(*this);
        ++m_value;
        return m_value;
    }

    int operator --()            
    {
        Util::Mutex::LockGuard sync(*this);
        --m_value;
        return m_value;
    }

    AtomicInt operator ++(int)        
    {
        Util::Mutex::LockGuard sync(*this);
        int ret = m_value;
        ++m_value;
        return ret;
    }

    AtomicInt operator --(int)
    {
        Util::Mutex::LockGuard sync(*this);
        int ret = m_value;
        --m_value;
        return ret;
    }

    int operator +=(int rhs)
    {
        Util::Mutex::LockGuard syncl(*this);
        m_value += rhs;
        return m_value;
    }

    int operator -=(int rhs)
    {
        Util::Mutex::LockGuard syncl(*this);
        m_value -= rhs;
        return m_value;
    }
    
    int operator *=(int rhs)
    {
        Util::Mutex::LockGuard syncl(*this);
        m_value *= rhs;
        return *this;
    }

    int operator /=(int rhs)
    {
        Util::Mutex::LockGuard syncl(*this);
        m_value /= rhs;
        return m_value;
    }

    int operator %=(int rhs)
    {
        Util::Mutex::LockGuard syncl(*this);
        m_value %= rhs;
        return m_value;
    }

    operator int() const
    {
        Util::Mutex::LockGuard sync(*this);
        return m_value;
    }

private:
    volatile int m_value;
};

}

#endif


#endif