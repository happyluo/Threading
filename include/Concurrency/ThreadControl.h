// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_THREAD_CONTROL_H
#define CONCURRENCY_THREAD_CONTROL_H

#include <Config.h>
#include <Util/Time.h>
#include <Build/UndefSysMacros.h>

#ifdef LANG_CPP11
#   include <memory>
#   include <thread>
#   include <chrono>
#endif

THREADING_BEGIN

class THREADING_API ThreadControl
{
public:

    ThreadControl(void);
    //~ThreadControl(void);

#ifdef LANG_CPP11
    typedef std::thread::id ID;
    ThreadControl(const std::shared_ptr<std::thread>& thread);
#elif defined(_WIN32)
    typedef DWORD ID;
    ThreadControl(HANDLE thread, ID id);
#else
    typedef pthread_t ID;
    explicit ThreadControl(pthread_t thread);
#endif

    bool operator ==(const ThreadControl& rhs) const;
    bool operator !=(const ThreadControl& rhs) const;

    void Join();

    void Detach(); 

    ID Id() const;

    static void Sleep(const Time& timeout);

    static void Yield();

private:

#ifdef LANG_CPP11
    std::shared_ptr<std::thread>        m_thread;
    ID            m_id;
#elif defined(_WIN32)
    HANDLE    m_thread;
    ID            m_id;
#else
    pthread_t    m_thread;

    //
    // Used to prevent joining/detaching a ThreadControl constructed
    // with the default constructor. Only needed to enforce our
    // portable join/detach behavior.
    //
    bool        m_detachable;
#endif
};

THREADING_END

#endif
