// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_LOGGER_UTIL_H
#define UTIL_LOGGER_UTIL_H

#include <Logging/Logger.h>

namespace Util
{

class UTIL_API LoggerOutputBase : private Util::noncopyable
{
public:

    std::string Str() const;
   
    std::ostringstream& Strstream(); // For internal use only. Don't use in your code.
    
private:

    std::ostringstream m_strstream;
};

template<typename T>
inline LoggerOutputBase&
operator <<(LoggerOutputBase& out, const T& val)
{
    out.Strstream() << val;
    return out;
}

UTIL_API LoggerOutputBase& operator <<(LoggerOutputBase&, std::ios_base& (*)(std::ios_base&));
UTIL_API LoggerOutputBase& operator <<(LoggerOutputBase&, std::ostream& (*)(std::ostream&));        // for std::endl like io function
UTIL_API LoggerOutputBase& operator <<(LoggerOutputBase&, const ::std::exception& ex);

template<class L, class LPtr, void (L::*output)(const std::string&)>
class LoggerOutput : public LoggerOutputBase
{
public:
    inline LoggerOutput(const LPtr& lptr) :
        m_logger(lptr)
    {}
    
    inline ~LoggerOutput()
    {
        Flush();
    }

    inline void Flush()
    {
        std::string s = Strstream().str();
        if (!s.empty())
        {
            L& ref = *m_logger;
            (ref.*output)(s);
        }
        Strstream().str("");
    }

private:
   
    LPtr m_logger;
};

typedef LoggerOutput<Logger, LoggerPtr, &Logger::Print> Print;
typedef LoggerOutput<Logger, LoggerPtr, &Logger::Warning> Warning;
typedef LoggerOutput<Logger, LoggerPtr, &Logger::Error> Error;

class UTIL_API Trace : public LoggerOutputBase
{
public:
    Trace(const LoggerPtr& logger, const std::string& category);
    ~Trace();
    void Flush();
   
private:
    
    LoggerPtr m_logger;
    std::string m_category;
};

}

#endif
