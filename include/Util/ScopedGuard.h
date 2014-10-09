// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_SCOPED_GUARD_H
#define UTIL_SCOPED_GUARD_H

#include <Config.h>

THREADING_BEGIN

class ScopedGuard : public noncopyable
{
public:
    explicit ScopedGuard(std::function<void ()> f) : m_func(f) {};
    explicit ScopedGuard(void f()) : m_func(f) {};

    template<typename T>
    explicit ScopedGuard(T t) : m_func(std::bind(&ScopedGuard::Call<T>, t)) {};

#ifdef LANG_CPP11    // HAS_CXX11_RVREF
    ScopedGuard(ScopedGuard&& other) : m_func(std::move(other.m_func))
    {
        other.Dismiss();
    }

    ScopedGuard& operator =(ScopedGuard&& other)
    {
        if (this != &other)
        {
            m_func();
            m_func = std::move(other.m_func);
            other.Dismiss();
        }
        return *this;
    }
#endif

    ~ScopedGuard()
    {
        m_func();
    }

    void Dismiss()
    {
        m_func = Nothing;
    }

private:
    static void Nothing() {};

    template<typename T>
    static void Call(T t) 
    {
        t(); 
    }

#ifdef LANG_CPP11
    ScopedGuard(const ScopedGuard&) =delete;
    ScopedGuard& operator=(const ScopedGuard& other) =delete;
#endif
    std::function<void ()> m_func;
};

THREADING_END


#endif  // UTIL_SCOPED_GUARD_H
