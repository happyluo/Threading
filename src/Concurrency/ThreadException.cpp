// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Concurrency/ThreadException.h>


//////////////////////////////////////////////////////////////////////////
/// ThreadSyscallException
Threading::ThreadSyscallException::ThreadSyscallException(const char *file, int line, int syscallError) :
    SyscallException(file, line, syscallError)
{
}

Threading::ThreadSyscallException::~ThreadSyscallException() throw()
{
}

const char* Threading::ThreadSyscallException::ms_pcName = "Threading::ThreadSyscallException";

std::string
Threading::ThreadSyscallException::Name() const
{
    return ms_pcName;
}

Threading::Exception*
Threading::ThreadSyscallException::Clone() const
{
    return new ThreadSyscallException(*this);
}

void
Threading::ThreadSyscallException::Throw() const
{
    throw *this;
}

//////////////////////////////////////////////////////////////////////////
/// ThreadLockedException
Threading::ThreadLockedException::ThreadLockedException(const char *file, int line) :
    Exception(file, line)
{
}

Threading::ThreadLockedException::~ThreadLockedException() throw()
{
}

const char* Threading::ThreadLockedException::ms_pcName = "Threading::ThreadLockedException";

std::string
Threading::ThreadLockedException::Name() const
{
    return ms_pcName;
}

Threading::Exception*
Threading::ThreadLockedException::Clone() const
{
    return new ThreadLockedException(*this);
}

void
Threading::ThreadLockedException::Throw() const
{
    throw *this;
}

//////////////////////////////////////////////////////////////////////////
/// class ThreadStartedException
Threading::ThreadStartedException::ThreadStartedException(const char *file, int line) :
    Exception(file, line)
{
}

Threading::ThreadStartedException::~ThreadStartedException() throw()
{
}

const char* Threading::ThreadStartedException::ms_pcName = "Threading::ThreadStartedException";

std::string
Threading::ThreadStartedException::Name() const
{
    return ms_pcName;
}

Threading::Exception*
Threading::ThreadStartedException::Clone() const
{
    return new ThreadStartedException(*this);
}

void
Threading::ThreadStartedException::Throw() const
{
    throw *this;
}

//////////////////////////////////////////////////////////////////////////
/// class ThreadNotStartedException
Threading::ThreadNotStartedException::ThreadNotStartedException(const char *file, int line) :
    Exception(file, line)
{
}

Threading::ThreadNotStartedException::~ThreadNotStartedException() throw()
{
}

const char* Threading::ThreadNotStartedException::ms_pcName = "Threading::ThreadNotStartedException";

std::string
Threading::ThreadNotStartedException::Name() const
{
    return ms_pcName;
}

Threading::Exception*
Threading::ThreadNotStartedException::Clone() const
{
    return new ThreadNotStartedException(*this);
}

void
Threading::ThreadNotStartedException::Throw() const
{
    throw *this;
}

//////////////////////////////////////////////////////////////////////////
/// BadThreadControlException
Threading::BadThreadControlException::BadThreadControlException(const char *file, int line) :
    Exception(file, line)
{
}

Threading::BadThreadControlException::~BadThreadControlException() throw()
{
}

const char* Threading::BadThreadControlException::ms_pcName = "Threading::BadThreadControlException";

std::string
Threading::BadThreadControlException::Name() const
{
    return ms_pcName;
}

Threading::Exception*
Threading::BadThreadControlException::Clone() const
{
    return new BadThreadControlException(*this);
}

void
Threading::BadThreadControlException::Throw() const
{
    throw *this;
}

//////////////////////////////////////////////////////////////////////////
/// class InvalidTimeoutException
Threading::InvalidTimeoutException::InvalidTimeoutException(
    const char *file, int line, const Time& timeout) :
    Exception(file, line), m_uSec(timeout)
{
}

Threading::InvalidTimeoutException::~InvalidTimeoutException() throw()
{
}

const char* Threading::InvalidTimeoutException::ms_pcName = "Threading::InvalidTimeoutException";

std::string
Threading::InvalidTimeoutException::Name() const
{
    return ms_pcName;
}

void 
Threading::InvalidTimeoutException::Print(std::ostream& out) const
{
    Exception::Print(out);
    out << ":\ninvalid timeout: " << m_uSec << " seconds";
}

Threading::Exception*
Threading::InvalidTimeoutException::Clone() const
{
    return new InvalidTimeoutException(*this);
}

void
Threading::InvalidTimeoutException::Throw() const
{
    throw *this;
}

//////////////////////////////////////////////////////////////////////////
/// class DeadlockException
Threading::DeadlockException::DeadlockException(
    const char *file, int line) :
    Exception(file, line)
{
}

Threading::DeadlockException::~DeadlockException() throw()
{
}

const char* Threading::DeadlockException::ms_pcName = "Threading::DeadlockException";

std::string
Threading::DeadlockException::Name() const
{
    return ms_pcName;
}

Threading::Exception*
Threading::DeadlockException::Clone() const
{
    return new DeadlockException(*this);
}

void
Threading::DeadlockException::Throw() const
{
    throw *this;
}

//////////////////////////////////////////////////////////////////////////
/// class ThreadPoolDestroyedException
Threading::ThreadPoolDestroyedException::ThreadPoolDestroyedException(
    const char *file, int line) :
    Exception(file, line)
{
}

Threading::ThreadPoolDestroyedException::~ThreadPoolDestroyedException() throw()
{
}

const char* Threading::ThreadPoolDestroyedException::ms_pcName = "Threading::ThreadPoolDestroyedException";

std::string
Threading::ThreadPoolDestroyedException::Name() const
{
    return ms_pcName;
}

Threading::Exception*
Threading::ThreadPoolDestroyedException::Clone() const
{
    return new ThreadPoolDestroyedException(*this);
}

void
Threading::ThreadPoolDestroyedException::Throw() const
{
    throw *this;
}