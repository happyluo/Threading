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
    out.Strstream() << ex.what();
    return out;
}

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
    }
    Strstream().str("");
}

