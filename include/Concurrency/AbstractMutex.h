// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_ABSTRACT_MUTEX_H
#define UTIL_ABSTRACT_MUTEX_H

#include <Config.h>
#include <Concurrency/Lock.h>


THREADING_BEGIN

class AbstractMutex
{
public:
    
    typedef LockT<AbstractMutex> LockGuard;
    typedef TryLockT<AbstractMutex> TryLockGuard;

    virtual ~AbstractMutex()
    {};

    virtual void Lock() const = 0;
    virtual void Unlock() const = 0;
    virtual bool TryLock() const = 0;
};

template <typename T>
class AbstractMutexI : public AbstractMutex, public T
{
public:

    typedef LockT<AbstractMutexI> LockGuard;
    typedef TryLockT<AbstractMutexI> TryLockGuard;

    virtual void Lock() const
    {
        T::Lock();
    }

    virtual void Unlock() const
    {
        T::Unlock();
    }

    virtual bool TryLock() const
    {
        return T::TryLock();
    }

    virtual ~AbstractMutexI()
    {}
};

template <typename T>
class AbstractMutexReadI : public AbstractMutex, public T
{
public:

    typedef LockT<AbstractMutexReadI> LockGuard;
    typedef TryLockT<AbstractMutexReadI> TryLockGuard;

    virtual void Lock() const
    {
        T::ReadLock();
    }

    virtual void Unlock() const
    {
        T::Unlock();
    }

    virtual bool TryLock() const
    {
        return T::TryReadLock();
    }

    virtual ~AbstractMutexReadI()
    {}
};

template <typename T>
class AbstractMutexWriteI : public AbstractMutex, public T
{
public:

    typedef LockT<AbstractMutexWriteI> LockGuard;
    typedef TryLockT<AbstractMutexWriteI> TryLockGuard;

    virtual void Lock() const
    {
        T::WriteLock();
    }

    virtual void Unlock() const
    {
        T::Unlock();
    }

    virtual bool TryLock() const
    {
        return T::TryWriteLock();
    }

    virtual ~AbstractMutexWriteI()
    {}
};

THREADING_END

#endif
