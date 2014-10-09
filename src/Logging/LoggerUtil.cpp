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

string
Threading::LoggerOutputBase::Str() const
{
    return m_strstream.str();
}

ostringstream&
Threading::LoggerOutputBase::Strstream()
{
    return m_strstream;
}

Threading::LoggerOutputBase&
Threading::operator <<(Threading::LoggerOutputBase& out, std::ios_base& (*val)(std::ios_base&))
{
    out.Strstream() << val;
    return out;
}

Threading::LoggerOutputBase& 
Threading::operator <<(Threading::LoggerOutputBase& out, std::ostream& (*val)(std::ostream&))
{
    out.Strstream() << val;
    return out;
}

Threading::LoggerOutputBase&
Threading::operator <<(Threading::LoggerOutputBase& out, const std::exception& ex)
{
    out.Strstream() << ex.what();
    return out;
}

Threading::Trace::Trace(const LoggerPtr& logger, const string& category) :
    m_logger(logger),
    m_category(category)
{
}

Threading::Trace::~Trace()
{
    Flush();
}

void
Threading::Trace::Flush()
{
    string s = Strstream().str();
    if (!s.empty())
    {
        m_logger->Trace(m_category, s);
    }
    Strstream().str("");
}

