// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************


#include <ostream>
#include <iomanip>
#include <cstdlib>
#include <Util/Exception.h>
#include <Concurrency/MutexPtrLock.h>
#include <Concurrency/Mutex.h>
#include <Util/StringUtil.h>


using namespace std;

namespace Threading
{
    bool DECLSPEC_EXPORT nullHandleAbort = false;
}

namespace
{

Threading::Mutex* globalMutex = 0;


class Init
{
public:

    Init()
    {
        globalMutex = new Threading::Mutex;
    }

    ~Init()
    {
        delete globalMutex;
        globalMutex = 0;
    }
};

Init init;

}

Threading::Exception::Exception() :
    m_fileName(0),
    m_line(0)
{
}

Threading::Exception::Exception(const char* file, int line) :
    m_fileName(file),
    m_line(line)
{
}

Threading::Exception::~Exception() throw()
{
}

const char* Threading::Exception::ms_pcName = "Threading::Exception";

string
Threading::Exception::Name() const
{
    return ms_pcName;
}

void
Threading::Exception::Print(ostream &out) const
{
    if (m_fileName && m_line > 0)
    {
        out << m_fileName << ':' << m_line << ": ";
    }
    out << Name();
}

const char*
Threading::Exception::what() const throw()
{
    try
    {
        Threading::MutexPtrLock<Threading::Mutex> lock(globalMutex);
        {
            if (m_strWhat.empty())
            {
                stringstream strStream;
                Print(strStream);
                m_strWhat = strStream.str();        // Lazy initialization.
            }
        }

        return m_strWhat.c_str();
    }
    catch(...)
    {
    }
    return "";
}

Threading::Exception*
Threading::Exception::Clone() const
{
    return new Exception(*this);
}

void
Threading::Exception::Throw() const
{
    throw *this;
}

const char*
Threading::Exception::File() const
{
    return m_fileName;
}

int
Threading::Exception::Line() const
{
    return m_line;
}

std::ostream&
Threading::operator<<(std::ostream &out, const Threading::Exception &ex)
{
    ex.Print(out);
    return out;
}

//////////////////////////////////////////////////////////////////////////
/// NullSharedPtrException
Threading::NullSharedPtrException::NullSharedPtrException(const char *file, int line) :
    Exception(file, line)
{
    if(Threading::nullHandleAbort)
    {
        abort();
    }
}

Threading::NullSharedPtrException::~NullSharedPtrException() throw()
{
}

const char* Threading::NullSharedPtrException::ms_pcName = "Threading::NullSharedPtrException";

string
Threading::NullSharedPtrException::Name() const
{
    return ms_pcName;
}

Threading::Exception*
Threading::NullSharedPtrException::Clone() const
{
    return new NullSharedPtrException(*this);
}

void
Threading::NullSharedPtrException::Throw() const
{
    throw *this;
}

//////////////////////////////////////////////////////////////////////////
/// IllegalArgumentException
Threading::IllegalArgumentException::IllegalArgumentException(const char *file, int line) :
    Exception(file, line)
{
}

Threading::IllegalArgumentException::IllegalArgumentException(
    const char *file, int line, const std::string& reason) :
    Exception(file, line), m_reason(reason)
{
}

Threading::IllegalArgumentException::~IllegalArgumentException() throw()
{
}

const char* Threading::IllegalArgumentException::ms_pcName = "Threading::IllegalArgumentException";

std::string
Threading::IllegalArgumentException::Name() const
{
    return ms_pcName;
}

void 
Threading::IllegalArgumentException::Print(std::ostream& out) const
{
    Exception::Print(out);
    out << ": " << m_reason;
}

Threading::Exception*
Threading::IllegalArgumentException::Clone() const
{
    return new IllegalArgumentException(*this);
}

void
Threading::IllegalArgumentException::Throw() const
{
    throw *this;
}

const std::string&
Threading::IllegalArgumentException::Reason() const
{
    return m_reason;
}

//////////////////////////////////////////////////////////////////////////
/// SyscallException
Threading::SyscallException::SyscallException(const char *file, int line, int syscallError) :
    Exception(file, line), m_errorCode(syscallError)
{
}

Threading::SyscallException::~SyscallException() throw()
{
}

const char* Threading::SyscallException::ms_pcName = "Threading::SyscallException";

std::string
Threading::SyscallException::Name() const
{
    return ms_pcName;
}

void 
Threading::SyscallException::Print(std::ostream& out) const
{
    Exception::Print(out);
    if (0 != m_errorCode)
    {
        out << "\nsystem call exception: " << Threading::ErrorToString(m_errorCode);
    }
}

Threading::Exception*
Threading::SyscallException::Clone() const
{
    return new SyscallException(*this);
}

void
Threading::SyscallException::Throw() const
{
    throw *this;
}

int 
Threading::SyscallException::Error() const
{
    return m_errorCode;
}

//////////////////////////////////////////////////////////////////////////
/// FileException
Threading::FileException::FileException(const char *file, int line, int err, const string& path) :
    SyscallException(file, line, err), m_path(path)
{
}

Threading::FileException::~FileException() throw()
{
}

const char* Threading::FileException::ms_pcName = "Threading::FileException";

std::string
Threading::FileException::Name() const
{
    return ms_pcName;
}

void 
Threading::FileException::Print(std::ostream& out) const
{
    SyscallException::Print(out);
    out << ":\ncould not open file: `" << m_path << "'"; 
}

Threading::FileException*
Threading::FileException::Clone() const
{
    return new FileException(*this);
}

void
Threading::FileException::Throw() const
{
    throw *this;
}

const std::string& 
Threading::FileException::Path() const
{
    return m_path;
}

//////////////////////////////////////////////////////////////////////////
/// FileLockException
Threading::FileLockException::FileLockException(const char *file, int line, int err, const string& path) :
    Exception(file, line), m_errorCode(err), m_path(path)
{
}

Threading::FileLockException::~FileLockException() throw()
{
}

const char* Threading::FileLockException::ms_pcName = "Threading::FileLockException";

std::string
Threading::FileLockException::Name() const
{
    return ms_pcName;
}

void 
Threading::FileLockException::Print(std::ostream& out) const
{
    Exception::Print(out);
    out << ":\ncould not lock file: `" << m_path << "'"; 
    if (0 != m_errorCode)
    {
        out << "\nsystem call exception: " << Threading::ErrorToString(m_errorCode);
    }
}

Threading::FileLockException*
Threading::FileLockException::Clone() const
{
    return new FileLockException(*this);
}

void
Threading::FileLockException::Throw() const
{
    throw *this;
}

const std::string& 
Threading::FileLockException::Path() const
{
    return m_path;
}

int 
Threading::FileLockException::Error() const
{
    return m_errorCode;
}

//////////////////////////////////////////////////////////////////////////
/// InitializationException
Threading::InitializationException::InitializationException(const char *file, int line) :
Exception(file, line)
{
}

Threading::InitializationException::InitializationException(
    const char *file, int line, const std::string& reason) :
Exception(file, line), m_reason(reason)
{
}

Threading::InitializationException::~InitializationException() throw()
{
}

const char* Threading::InitializationException::ms_pcName = "Threading::InitializationException";

string
Threading::InitializationException::Name() const
{
    return ms_pcName;
}

void
Threading::InitializationException::Print(ostream& out) const
{
    Exception::Print(out);
    out << ": " << m_reason;
}

Threading::InitializationException*
Threading::InitializationException::Clone() const
{
    return new InitializationException(*this);
}

void
Threading::InitializationException::Throw() const
{
    throw *this;
}

const std::string&
Threading::InitializationException::Reason() const
{
    return m_reason;
}

//////////////////////////////////////////////////////////////////////////
/// VersionMismatchException
Threading::VersionMismatchException::VersionMismatchException(const char *file, int line) :
    Exception(file, line)
{
}

Threading::VersionMismatchException::~VersionMismatchException() throw()
{
}

const char* Threading::VersionMismatchException::ms_pcName = "Threading::VersionMismatchException";

string
Threading::VersionMismatchException::Name() const
{
    return ms_pcName;
}

Threading::VersionMismatchException*
Threading::VersionMismatchException::Clone() const
{
    return new VersionMismatchException(*this);
}

void
Threading::VersionMismatchException::Throw() const
{
    throw *this;
}

//////////////////////////////////////////////////////////////////////////
/// FatalException
Threading::FatalException::FatalException(
    const char *file, int line, const std::string& reason) :
    Exception(file, line), m_reason(reason)
{
}

Threading::FatalException::~FatalException() throw()
{
}

const char* Threading::FatalException::ms_pcName = "Threading::FatalException";

string
Threading::FatalException::Name() const
{
    return ms_pcName;
}

void
Threading::FatalException::Print(ostream& out) const
{
    Exception::Print(out);
    out << ": " << m_reason;
}

Threading::FatalException*
Threading::FatalException::Clone() const
{
    return new FatalException(*this);
}

void
Threading::FatalException::Throw() const
{
    throw *this;
}

const std::string&
Threading::FatalException::Reason() const
{
    return m_reason;
}

