// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Logging/LoggerUtil.h>
#include <Logging/Logger.h>

using namespace std;

namespace UtilInternal
{

extern bool DECLSPEC_IMPORT printStackTraces;

}

string
Util::LoggerOutputBase::Str() const
{
    return m_strstream.str();
}

ostringstream&
Util::LoggerOutputBase::Strstream()
{
    return m_strstream;
}

Util::LoggerOutputBase&
Util::operator <<(Util::LoggerOutputBase& out, std::ios_base& (*val)(std::ios_base&))
{
    out.Strstream() << val;
    return out;
}

Util::LoggerOutputBase& 
Util::operator <<(Util::LoggerOutputBase& out, std::ostream& (*val)(std::ostream&))
{
	out.Strstream() << val;
	return out;
}

Util::LoggerOutputBase&
Util::operator <<(Util::LoggerOutputBase& out, const std::exception& ex)
{
    if (UtilInternal::printStackTraces)
    {
        const ::Util::Exception* exception = dynamic_cast<const ::Util::Exception*>(&ex);
        if (exception)
        {
            out.Strstream() << exception->what() << '\n' << exception->StackTrace();
            return out;
        }
    }
    out.Strstream() << ex.what();
    return out;
}

//#ifndef LANG_CPP11
//
//Util::Print::Print(const LoggerPtr& logger) :
//	m_logger(logger)
//{
//}
//
//Util::Print::~Print()
//{
//	Flush();
//}
//
//void
//Util::Print::Flush()
//{
//	string s = Strstream().str();
//	if(!s.empty())
//	{
//		m_logger->Print(s);
//	}
//	Strstream().str("");
//}
//
//Util::Warning::Warning(const LoggerPtr& logger) :
//	m_logger(logger)
//{
//}
//
//Util::Warning::~Warning()
//{
//	Flush();
//}
//
//void
//Util::Warning::Flush()
//{
//	string s = Strstream().str();
//	if(!s.empty())
//	{
//		m_logger->Warning(s);
//	}
//	Strstream().str("");
//}
//
//Util::Error::Error(const LoggerPtr& logger) :
//	m_logger(logger)
//{
//}
//
//Util::Error::~Error()
//{
//	Flush();
//}
//
//void
//Util::Error::Flush()
//{
//	string s = Strstream().str();
//	if(!s.empty())
//	{
//		m_logger->Error(s);
//	}
//	Strstream().str("");
//}
//
//#endif

Util::Trace::Trace(const LoggerPtr& logger, const string& category) :
    m_logger(logger),
    m_category(category)
{
}

Util::Trace::~Trace()
{
    Flush();
}

void
Util::Trace::Flush()
{
    string s = Strstream().str();
    if (!s.empty())
    {
		m_logger->Trace(m_category, s);

		//if (m_logger)
		//{
		//	m_logger->Trace(m_category, s);
		//}
		//else
		//{
		//	Logger("", "").Trace(m_category, s);
		//}
    }
    Strstream().str("");
}

