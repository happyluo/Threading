// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_EXCEPTION_H
#define UTIL_EXCEPTION_H

#include <string>
#include <exception>
#include <sstream>
#include <Config.h>
#include <Util/Exception.h>

THREADING_BEGIN

class THREADING_API Exception : public std::exception
{
public:
    Exception();
    Exception(const char* file, int line);

    virtual ~Exception() throw();

    virtual std::string Name() const;

    virtual void Print(std::ostream& out) const;

    virtual const char* what() const throw();

    virtual Exception* Clone() const;

    virtual void Throw() const;

    const char* File() const;

    int Line() const;

private:

    const char            *m_fileName;
    int                    m_line;
    static const char    *ms_pcName;
    mutable std::string    m_strWhat;        // Initialized lazily in what().
};

THREADING_API std::ostream& operator << (std::ostream &out, const Threading::Exception &ex);

/// �չ���ָ���쳣��
class THREADING_API NullSharedPtrException : public Exception
{
public:
    NullSharedPtrException(const char* file, int line);

    virtual ~NullSharedPtrException() throw();

    virtual std::string Name() const;

    virtual Exception* Clone() const;

    virtual void Throw() const;
    
private:
    
    static const char *ms_pcName;
};
    
class THREADING_API IllegalArgumentException : public Exception
{
public:
    IllegalArgumentException(const char* file, int line);

    IllegalArgumentException(const char* file, int line, const std::string& reason);

    virtual ~IllegalArgumentException() throw();

    virtual std::string Name() const;

    virtual void Print(std::ostream& out) const;

    virtual Exception* Clone() const;

    virtual void Throw() const;

    const std::string& Reason() const;

private:

    std::string            m_reason;

    static const char    *ms_pcName;
};

/// ϵͳ�����쳣��
class THREADING_API SyscallException : public Exception
{
public:
    SyscallException(const char* file, int line, int syscallError);

    virtual ~SyscallException() throw();

    virtual std::string Name() const;

    virtual void Print(std::ostream& out) const;

    virtual Exception* Clone() const;

    virtual void Throw() const;

    int Error() const;

private:

    const int            m_errorCode;
    static const char    *ms_pcName;
};

//////////////////////////////////////////////////////////////////////////
/// FileException
class THREADING_API FileException : public SyscallException
{
public:
    FileException(const char* file, int line, int syscallError, const std::string& path);

    virtual ~FileException() throw();

    virtual std::string Name() const;

    virtual void Print(std::ostream& out) const;

    virtual FileException* Clone() const;

    virtual void Throw() const;

    const std::string& Path() const;

private:

    static const char *ms_pcName;
    std::string m_path;
};


//////////////////////////////////////////////////////////////////////////
/// FileLockException
class THREADING_API FileLockException : public Exception
{
public:
    FileLockException(const char* file, int line, int syscallError, const std::string& path);

    virtual ~FileLockException() throw();

    virtual std::string Name() const;

    virtual void Print(std::ostream& out) const;

    virtual FileLockException* Clone() const;

    virtual void Throw() const;

    const std::string& Path() const;

    int Error() const;

private:

    const int m_errorCode;
    static const char *ms_pcName;
    std::string m_path;
};

//////////////////////////////////////////////////////////////////////////
/// InitializationException
class THREADING_API InitializationException : public Exception
{
    friend class Properties;

public:

    InitializationException(const char* file, int line);
    InitializationException(const char* file, int line, const std::string& reason);
    virtual ~InitializationException() throw();

    virtual std::string Name() const;
    virtual void Print(std::ostream& out) const;
    virtual InitializationException* Clone() const;
    virtual void Throw() const;

    const std::string& Reason() const;

private:

    std::string m_reason;

    static const char* ms_pcName;    
};

//////////////////////////////////////////////////////////////////////////
/// VersionMismatchException
class THREADING_API VersionMismatchException : public Exception
{
public:

    VersionMismatchException(const char* file, int line);
    virtual ~VersionMismatchException() throw();
    virtual std::string Name() const;
    virtual VersionMismatchException* Clone() const;
    virtual void Throw() const;

private:

    static const char *ms_pcName;
};

//////////////////////////////////////////////////////////////////////////
/// FatalException
class THREADING_API FatalException : public Exception
{
public:
    FatalException(const char* file, int line, const std::string& message);
    virtual ~FatalException() throw();

    virtual std::string Name() const;
    virtual void Print(std::ostream& out) const;
    virtual FatalException* Clone() const;
    virtual void Throw() const;

    const std::string& Reason() const;

private:
    std::string m_reason;

    static const char* ms_pcName;    
};

THREADING_END

#endif
