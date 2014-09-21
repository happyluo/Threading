// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_UNIQUE_PTR_H
#define UTIL_UNIQUE_PTR_H

#include <Util/Config.h>

namespace Util
{

//
// This is temporary and very partial placeholder for std::unique_ptr,
// which is not yet widely available.
//


template<typename T>
class UniquePtr
{
public:

    explicit UniquePtr(T* ptr = 0) : m_ptr(ptr)
    {
    }

    UniquePtr(UniquePtr& o) : m_ptr(o.Release())
    {
    }

    UniquePtr& operator =(UniquePtr& o)
    {
        Reset(o.Release());
        return *this;
    }

    ~UniquePtr()
    {
        if (m_ptr != 0)
        {
            delete m_ptr;
        }
    }

    T* Release()
    {
        T* r = m_ptr;
        m_ptr = 0;
        return r;
    }

    void Reset(T* ptr = 0)
    {
        assert(ptr == 0 || ptr != m_ptr);
        
        if (m_ptr != 0)
        {
            delete m_ptr;
        }
        m_ptr = ptr;
    }

    T& operator*() const
    {
        return *m_ptr;
    }
    
    T* operator->() const
    {
        return m_ptr;
    }


    T* Get() const
    {
        return m_ptr;
    }

	operator bool() const
	{
		return m_ptr ? true : false;
	}

    void Swap(UniquePtr& a)
    {
        T* tmp = a.m_ptr;
        a.m_ptr = m_ptr;
        m_ptr = tmp;
    }

private:

    T* m_ptr;
};

} // End of namespace Util

#endif
