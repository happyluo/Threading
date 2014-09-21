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

//template<class Y>
//LoggerOutputBase& 
//operator<<(LoggerOutputBase& os, ::IceInternal::ProxyHandle<Y> p)
//{
//    return os << (p ? p->ice_toString() : "");
//}

UTIL_API LoggerOutputBase& operator <<(LoggerOutputBase&, std::ios_base& (*)(std::ios_base&));
UTIL_API LoggerOutputBase& operator <<(LoggerOutputBase&, std::ostream& (*)(std::ostream&));		// for std::endl like io function
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

//#ifdef LANG_CPP11

typedef LoggerOutput<Logger, LoggerPtr, &Logger::Print> Print;
typedef LoggerOutput<Logger, LoggerPtr, &Logger::Warning> Warning;
typedef LoggerOutput<Logger, LoggerPtr, &Logger::Error> Error;

//#else

//
//class UTIL_API Print : public LoggerOutputBase
//{
//public:
//
//	Print(const LoggerPtr&);
//	~Print();
//
//	void Flush();
//
//private:
//
//	LoggerPtr m_logger;
//};
//
//class UTIL_API Warning : public LoggerOutputBase
//{
//public:
//
//	Warning(const LoggerPtr&);
//	~Warning();
//
//	void Flush();
//
//private:
//
//	LoggerPtr m_logger;
//};
//
//class UTIL_API Error : public LoggerOutputBase
//{
//public:
//
//	Error(const LoggerPtr&);
//	~Error();
//
//	void Flush();
//
//private:
//
//	LoggerPtr m_logger;
//};
//
//#endif

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

//
// A special plug-in that installs a logger during a communicator's initialization.
// Both initialize and destroy are no-op. See Ice::InitializationData.
//
//class UTIL_API LoggerPlugin : public Ice::Plugin
//{
//public:
//
//    LoggerPlugin(const CommunicatorPtr& communicator, const LoggerPtr&);
//
//    virtual void initialize();
//
//    virtual void destroy();
//};

}

#endif
