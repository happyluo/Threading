// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifdef _WIN32
#   include <sys/timeb.h>
#   include <time.h>
#else
#   include <sys/time.h>
#endif

#ifdef __APPLE__
#   include <mach/mach.h>
#   include <mach/mach_time.h>
#endif

#include <iostream>
#include <iomanip>
#include <Util/Time.h>
#include <Util/Exception.h>
#include <Build/UsefulMacros.h>

using namespace Threading;

#ifdef _WIN32

namespace
{

static double frequency = -1.0;

class InitializeFrequency
{
public:

    InitializeFrequency()
    {
        Int64 v;
        if (QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&v)))   
        {
            if (QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&v)))  
            {
                frequency = static_cast<double>(v);
            }
        }
    }
};
static InitializeFrequency frequencyInitializer;

}
#endif

#ifdef __APPLE__
namespace
{

double machMultiplier = 1.0;
class InitializeTime
{
public:

    InitializeTime()
    {
        mach_timebase_info_data_t initTimeBase = { 0, 0 };
        mach_timebase_info(&initTimeBase);
        machMultiplier = static_cast<double>(initTimeBase.numer) / initTimeBase.denom / UTIL_INT64(1000);
    }
};
InitializeTime initializeTime;

}
#endif

Threading::Time::Time() throw() :
    m_microsec(0)
{
}

#ifdef _WIN32
Threading::Time::Time(int year, int month, int day, int hour, int min, int sec, int DST)
{
    ENSURE_THROW(year >= 1900, Threading::IllegalArgumentException);
    ENSURE_THROW(month >= 1 && month <= 12, Threading::IllegalArgumentException);
    ENSURE_THROW(day >= 1 && day <= 31, Threading::IllegalArgumentException);
    ENSURE_THROW(hour >= 0 && hour <= 23, Threading::IllegalArgumentException);
    ENSURE_THROW(min >= 0 && min <= 59, Threading::IllegalArgumentException);
    ENSURE_THROW(sec >= 0 && sec <= 59, Threading::IllegalArgumentException);

    struct tm atm;

    atm.tm_sec = sec;
    atm.tm_min = min;
    atm.tm_hour = hour;
    atm.tm_mday = day;
    atm.tm_mon = month - 1;        // tm_mon is 0 based
    atm.tm_year = year - 1900;    // tm_year is 1900 based
    atm.tm_isdst = DST;

    Int64 time = _mktime64(&atm);
    assert(-1 != time);       // indicates an illegal input time
    if(-1 == time)
    {
        throw Threading::IllegalArgumentException(__FILE__, __LINE__);
    }

    m_microsec = static_cast<Int64>(time) * UTIL_INT64(1000000);
}

struct tm* 
Threading::Time::GetGmtTime(struct tm* ptm) const
{
    // Ensure ptm is valid
    ENSURE_THROW(0 != ptm, Threading::IllegalArgumentException);

    if (0 != ptm)
    {
        Int64 time = m_microsec / 1000000;
        struct tm tmTemp;
        errno_t err = _gmtime64_s(&tmTemp, &time);

        // Be sure the call succeeded
        if(0 != err) 
        {
            return 0; 
        }

        *ptm = tmTemp;
        return ptm;
    }

    return 0;
}

bool 
Threading::Time::GetAsSystemTime(SYSTEMTIME& timeDest) const throw()
{
    struct tm ttm;
    struct tm* ptm;

    ptm = GetLocalTime(&ttm);

    if(!ptm) 
    {
        return false; 
    }

    timeDest.wYear = (WORD) (1900 + ptm->tm_year);
    timeDest.wMonth = (WORD) (1 + ptm->tm_mon);
    timeDest.wDayOfWeek = (WORD) ptm->tm_wday;
    timeDest.wDay = (WORD) ptm->tm_mday;
    timeDest.wHour = (WORD) ptm->tm_hour;
    timeDest.wMinute = (WORD) ptm->tm_min;
    timeDest.wSecond = (WORD) ptm->tm_sec;
    timeDest.wMilliseconds = 0;

    return true;
}

#endif

Time
Threading::Time::Now(Clock clock)
{
    if (clock == Realtime)
    {
#ifdef _WIN32
#  if defined(_MSC_VER)
        struct _timeb tb;
        _ftime(&tb);
#  elif defined(__MINGW32__)
        struct timeb tb;
        ftime(&tb);
#  endif
        return Time(static_cast<Int64>(tb.time) * UTIL_INT64(1000000) + tb.millitm * 1000);
#else
        struct timeval tv;
        if (gettimeofday(&tv, 0) < 0)
        {
            assert(0);
            throw SyscallException(__FILE__, __LINE__, errno);
        }
        return Time(tv.tv_sec * UTIL_INT64(1000000) + tv.tv_usec);
#endif
    }
    else // Monotonic
    {
#if defined(_WIN32)
        if (frequency > 0.0)
        {
            Int64 count;
            if (!QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&count)))
            {
                assert(0);
                throw SyscallException(__FILE__, __LINE__, GetLastError());
            }
            return Time(static_cast<Int64>(count / frequency * 1000000.0));
        }
        else
        {
#  if defined(_MSC_VER)
            struct _timeb tb;
            _ftime(&tb);
#  elif defined(__MINGW32__)
            struct timeb tb;
            ftime(&tb);
#  endif
            return Time(static_cast<Int64>(tb.time) * UTIL_INT64(1000000) + tb.millitm * 1000);
        }
#elif defined(__hpux)
        //
        // HP does not support CLOCK_MONOTONIC
        //
        struct timeval tv;
        if (gettimeofday(&tv, 0) < 0)
        {
            assert(0);
            throw SyscallException(__FILE__, __LINE__, errno);
        }
        return Time(tv.tv_sec * UTIL_INT64(1000000) + tv.tv_usec);
#elif defined(__APPLE__)
       return Time(mach_absolute_time() * machMultiplier);
#else
        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
        {
            assert(0);
            throw SyscallException(__FILE__, __LINE__, errno);
        }
        return Time(ts.tv_sec * UTIL_INT64(1000000) + ts.tv_nsec / UTIL_INT64(1000));
#endif
    }
}

Time 
Time::TimeOfToday(size_t hour, size_t minute, size_t second)
{
    assert(minute < 60 && second < 60);

    Time base(Now(Realtime) + Hours(24) * ((int)hour / 24));
    time_t time = static_cast<long>(base.ToMicroSeconds() / 1000000);

    struct tm* t;
#ifdef _WIN32
    t = localtime(&time);
#else
    struct tm tr;
    localtime_r(&time, &tr);
    t = &tr;
#endif
    t->tm_hour = hour % 24;
    t->tm_min = minute;
    t->tm_sec = second;

    return Seconds(mktime(t));
}

Time
Threading::Time::Hours(size_t t)
{
    return Time(t * 3600 * UTIL_INT64(1000000));
}

Time
Threading::Time::Minutes(size_t t)
{
    return Time(t * 60 * UTIL_INT64(1000000));
}

Time
Threading::Time::Seconds(Int64 t)
{
    return Time(t * UTIL_INT64(1000000));
}

Time
Threading::Time::MilliSeconds(Int64 t)
{
    return Time(t * UTIL_INT64(1000));
}

Time
Threading::Time::MicroSeconds(Int64 t)
{
    return Time(t);
}

Time
Threading::Time::SecondsDouble(double t)
{
    return Time(Int64(t * 1000000));
}

Time
Threading::Time::MilliSecondsDouble(double t)
{
    return Time(Int64(t * 1000));
}

Time
Threading::Time::MicroSecondsDouble(double t)
{
    return Time(Int64(t));
}

#ifndef _WIN32
Threading::Time::operator timeval() const
{
    timeval tv;
    tv.tv_sec = static_cast<long>(m_microsec / 1000000);
    tv.tv_usec = static_cast<long>(m_microsec % 1000000);
    return tv;
}
#endif

Threading::Time::operator tm() const
{
    time_t time = static_cast<long>(m_microsec / 1000000);

    struct tm* t;
#ifdef _WIN32
    t = localtime(&time);
#else
    struct tm tr;
    localtime_r(&time, &tr);
    t = &tr;
#endif

    return *t;
}

Int64
Threading::Time::ToSeconds() const
{
    return m_microsec / 1000000;
}

Int64
Threading::Time::ToMilliSeconds() const
{
    return m_microsec / 1000;
}

Int64
Threading::Time::ToMicroSeconds() const
{
    return m_microsec;
}

double
Threading::Time::ToSecondsDouble() const
{
    return m_microsec / 1000000.0;
}

double
Threading::Time::ToMilliSecondsDouble() const
{
    return m_microsec / 1000.0;
}

double
Threading::Time::ToMicroSecondsDouble() const
{
    return static_cast<double>(m_microsec);
}

std::string
Threading::Time::ToDateTime(Clock clock) const
{
    time_t time = static_cast<long>(m_microsec / 1000000);

    struct tm* t;
#ifdef _WIN32
    t = localtime(&time);
#else
    struct tm tr;
    localtime_r(&time, &tr);
    t = &tr;
#endif

    char buf[32];
    if (Realtime == clock)
    {
        strftime(buf, sizeof(buf), "%x %H:%M:%S", t);
    }
    else
    {
        strftime(buf, sizeof(buf), "%H:%M:%S", t);
    }

    std::ostringstream os;
    os << buf << ".";
    os.fill('0');
    os.width(3);
    os << static_cast<long>(m_microsec % 1000000 / 1000);
    return os.str();
}

std::string
Threading::Time::ToDuration() const
{
    Int64 usecs = m_microsec % 1000000;
    Int64 secs = m_microsec / 1000000 % 60;
    Int64 mins = m_microsec / 1000000 / 60 % 60;
    Int64 hours = m_microsec / 1000000 / 60 / 60 % 24;
    Int64 days = m_microsec / 1000000 / 60 / 60 / 24;

    using namespace std;

    ostringstream os;
    if (days != 0)
    {
        os << days << "d ";
    }
    os << setfill('0') << setw(2) << hours << ":" << setw(2) << mins << ":" << setw(2) << secs;
    if (usecs != 0)
    {
        os << "." << setw(3) << (usecs / 1000);
    }

    return os.str();
}

Int64 
Threading::Time::GetTime() const throw()
{
    return(m_microsec / 1000000);
}

struct tm* 
Threading::Time::GetLocalTime(struct tm* ptm) const
{
    // Ensure ptm is valid
    ENSURE_THROW(0 != ptm, Threading::IllegalArgumentException);

    if (0 != ptm)
    {
        time_t time = static_cast<long>(m_microsec / 1000000);
        
        struct tm tval;
#ifdef _WIN32
        
        errno_t err = _localtime64_s(&tval, &time);
        if(0 != err) 
        {
            return NULL;
        }
#else
        localtime_r(&time, &tval);
#endif

        *ptm = tval;
        return ptm;
    }

    return NULL;
}

int 
Threading::Time::GetYear() const throw()
{ 
    struct tm ttm;
    struct tm * ptm;

    ptm = GetLocalTime(&ttm);
    return ptm ? (ptm->tm_year) + 1900 : 0 ; 
}

int 
Threading::Time::GetMonth() const throw()
{ 
    struct tm ttm;
    struct tm * ptm;

    ptm = GetLocalTime(&ttm);
    return ptm ? ptm->tm_mon + 1 : 0;
}

int 
Threading::Time::GetDay() const throw()
{
    struct tm ttm;
    struct tm * ptm;

    ptm = GetLocalTime(&ttm);
    return ptm ? ptm->tm_mday : 0 ; 
}

int 
Threading::Time::GetHour() const throw()
{
    struct tm ttm;
    struct tm * ptm;

    ptm = GetLocalTime(&ttm);
    return ptm ? ptm->tm_hour : -1 ; 
}

int 
Threading::Time::GetMinute() const throw()
{
    struct tm ttm;
    struct tm * ptm;

    ptm = GetLocalTime(&ttm);
    return ptm ? ptm->tm_min : -1 ; 
}

int 
Threading::Time::GetSecond() const throw()
{ 
    struct tm ttm;
    struct tm * ptm;

    ptm = GetLocalTime(&ttm);
    return ptm ? ptm->tm_sec : -1 ;
}

int 
Threading::Time::GetDayOfWeek() const throw()
{ 
    struct tm ttm;
    struct tm * ptm;

    ptm = GetLocalTime(&ttm);
    return ptm ? ptm->tm_wday + 1 : 0 ;
}

Time::Time(Int64 usec) :
    m_microsec(usec)
{
}

std::ostream&
Threading::operator <<(std::ostream& out, const Time& tm)
{
    return out << tm.ToMicroSeconds() / 1000000.0;
}
