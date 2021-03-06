// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <stdio.h>
#include <Util/Time.h>
#include <Util/StringUtil.h>
#include <Concurrency/Mutex.h>
#include <Concurrency/MutexPtrLock.h>
#include <Unicoder/StringConverter.h>
#include <Logging/Logger.h>

using namespace std;
using namespace Threading;


namespace
{

Threading::Mutex* outputMutex = 0;
static Threading::Mutex* sProcessLoggerMutex = 0;
static Threading::LoggerPtr sProcessLogger;

class Init
{
public:

    Init()
    {
        outputMutex = new Threading::Mutex;
        sProcessLoggerMutex = new Threading::Mutex;
    }

    ~Init()
    {
        delete outputMutex;
        outputMutex = 0;
        delete sProcessLoggerMutex;
        sProcessLoggerMutex = 0;
    }
};

Init init;
}

LoggerPtr
Threading::GetProcessLogger()
{
    Threading::MutexPtrLock<Threading::Mutex> lock(sProcessLoggerMutex);

    if (sProcessLogger == 0)
    {
        sProcessLogger = new Threading::Logger("", "");
    }
    return sProcessLogger;
}

void
Threading::SetProcessLogger(const LoggerPtr& logger)
{
    Threading::MutexPtrLock<Threading::Mutex> lock(sProcessLoggerMutex);
    sProcessLogger = logger;
}


Threading::Logger::Logger(const string& prefix, const string& file)
{
    if (!prefix.empty())
    {
        m_prefix = prefix + ": ";
    }

    if (!file.empty())
    {
        m_file = file;
        m_out.open(file, fstream::out | fstream::app);
        if (!m_out.is_open())
        {
            throw InitializationException(__FILE__, __LINE__, "FileLogger: cannot open " + m_file);
        }
    }
}


Threading::Logger::~Logger()
{
    if (m_out.is_open())
    {
        m_out.close();
    }
}

void
Threading::Logger::Print(const string& message)
{
    Write(message, false);
}

void
Threading::Logger::Trace(const string& category, const string& message)
{
    string s = "--[  INFO ] " + Threading::Time::Now().ToDateTime() + " " + m_prefix;
    if (!category.empty())
    {
        s += category + ": ";
    }
    s += message;

    Write(s, true);
}

void
Threading::Logger::Warning(const string& message)
{
    Write("-![WARNING] " + Threading::Time::Now().ToDateTime() + " " + m_prefix + "warning: " + message, true, fgyellow);
}

void
Threading::Logger::Error(const string& message)
{
    Write("!![ ERROR ] " + Threading::Time::Now().ToDateTime() + " " + m_prefix + "error: " + message, true, fgred);
}

LoggerPtr
Threading::Logger::CloneWithPrefix(const std::string& prefix)
{
    return new Logger(prefix, m_file);
}

void
Threading::Logger::Write(const string& message, bool indent, ostream& (*color)(ostream &out))
{
    Threading::MutexPtrLock<Threading::Mutex> sync(outputMutex);

    string s = message;

    if (indent)
    {
        string::size_type idx = 0;
        while ((idx = s.find("\n", idx)) != string::npos)
        {
            s.insert(idx + 1, "   ");
            ++idx;
        }
    }

    if (m_out.is_open())
    {
        m_out << s << endl;
    }
    else
    {
        colorostream out(cerr);
        if (0 != color)
        {
            out.tostd() << color;
        }
        out.tostd() << s << endl;
    }
}

//
// StderrLog    - log all messages to stderr.
// 
const char kUnknownFile[] = "unknown file";

// Formats a source file path and a line number as they would appear
// in an error message from the compiler used to compile this code.
THREADING_API ::std::string Threading::FormatFileLocation(const char* file, int line)
{
    const char* const file_name = file == NULL ? kUnknownFile : file;

    if (line < 0) 
    {
        return Format("%s:", file_name).c_str();
    }
#ifdef _MSC_VER
    return Format("%s(%d):", file_name, line).c_str();
#else
    return Format("%s:%d:", file_name, line).c_str();
#endif  // _MSC_VER
}


Threading::StderrLog::StderrLog(LogSeverity severity, const char* file, int line) :
    m_severity(severity) 
    , m_outstream(cerr)
    , m_filename(file)
    , m_line(line)
{
    const char* const marker =
        severity == LOGLEVEL_INFO ?    "[  INFO ]" :
        severity == LOGLEVEL_WARNING ? "[WARNING]" :
        severity == LOGLEVEL_ERROR ?   "[ ERROR ]" : "[ FATAL ]";

    //::std::ostream& (*color)(::std::ostream &) = 
    //    severity == LOGLEVEL_INFO ? &dft<char> :
    //    severity == LOGLEVEL_WARNING ? &fgyellow<char> :
    //    severity == LOGLEVEL_ERROR ? &fgred<char> : &fgred<char>;

    ::std::ostream& (*color)(::std::ostream &) = &dft<char>;
    if (LOGLEVEL_WARNING == severity)
    {
        color = &fgyellow<char>;
    }
    else if (LOGLEVEL_ERROR == severity || LOGLEVEL_FATAL == severity)
    {
        color = &fgred<char>;
    }

    GetStream() << ::std::endl << color << marker << " "
        << FormatFileLocation(file, line).c_str() << ": ";
}

// Flushes the buffers and, if severity is LOGLEVEL_FATAL, aborts the program.
Threading::StderrLog::~StderrLog() 
{
    fflush(stderr);
    if (LOGLEVEL_FATAL == m_severity)
    {
#if USE_EXCEPTIONS
        throw FatalException(m_filename, m_line, "");
#else
        ::abort();
#endif
    }
}


THREADING_BEGIN

namespace internal 
{

void DefaultLogHandler(LogSeverity level
                       , const char* filename
                       , int line
                       , const string& message
                       , LoggerPtr logger)
{
    static const char* slevelNames[] = { "[  INFO ]", "[WARNING]", "[ ERROR ]", "[ FATAL ]" };
    if (!logger)
    {
        ::Threading::StderrLog log(level, filename, line);
        log << message << "\n";
        fflush(stderr);  // Needed on MSVC.
    }
    else
    {
        static std::ostringstream out;
        out << slevelNames[level] << " "
            << FormatFileLocation(filename, line) << ": "<< message << "\n";
        logger->Print(out.str());
        out.str("");
    }
}

void NullLogHandler(LogSeverity
                    , const char*
                    , int
                    , const string&
                    , LoggerPtr) 
{
    // Nothing.
}

static LoggerPtr sLogger = 0;
static LogHandler* sLogHandler = &DefaultLogHandler;
//static int sLogSilencerCount = 0;

LogMessage& LogMessage::operator<<(const string& value)
{
    m_message += value;
    return *this;
}

LogMessage& LogMessage::operator<<(const char* value)
{
    m_message += value;
    return *this;
}

// Since this is just for logging, we don't care if the current locale changes
// the results -- in fact, we probably prefer that.  So we use snprintf()
// instead of Simple*toa().
#undef DECLARE_STREAM_OPERATOR
#define DECLARE_STREAM_OPERATOR(TYPE, FORMAT)                          \
    LogMessage& LogMessage::operator<<(TYPE value) {                   \
        /* 128 bytes should be big enough for any of the primitive */  \
        /* values which we print with this, but well use snprintf() */ \
        /* anyway to be extra safe. */                                 \
        char buffer[128];                                              \
        snprintf(buffer, sizeof(buffer), FORMAT, value);               \
        /* Guard against broken MSVC snprintf(). */                    \
        buffer[sizeof(buffer)-1] = '\0';                               \
        m_message += buffer;                                           \
        return *this;                                                  \
    }

DECLARE_STREAM_OPERATOR(char         , "%c" )
DECLARE_STREAM_OPERATOR(int          , "%d" )
DECLARE_STREAM_OPERATOR(uint         , "%u" )
DECLARE_STREAM_OPERATOR(long         , "%ld")
DECLARE_STREAM_OPERATOR(unsigned long, "%lu")
DECLARE_STREAM_OPERATOR(double       , "%g" )
#undef DECLARE_STREAM_OPERATOR

LogMessage::LogMessage(LogSeverity level, const char* filename, int line)
    : m_level(level), m_filename(filename), m_line(line) {}
LogMessage::~LogMessage() {}

void LogMessage::Finish()
{
    sLogHandler(m_level, m_filename, m_line, m_message, sLogger);

    if (m_level == LOGLEVEL_FATAL) 
    {
#if USE_EXCEPTIONS
        throw FatalException(m_filename, m_line, m_message);
#else
        ::abort();
#endif
    }
}

void LogFinisher::operator=(LogMessage& other)
{
    other.Finish();
}

}  // namespace internal

Threading::LoggerPtr SetLogger(Threading::LoggerPtr newlogger)
{
    std::swap(internal::sLogger, newlogger);
    return newlogger;
}

LogHandler* SetLogHandler(LogHandler* newfunc)
{
    LogHandler* old = internal::sLogHandler;
    if (old == &internal::NullLogHandler) 
    {
        old = NULL;
    }
    if (newfunc == NULL)
    {
        internal::sLogHandler = &internal::NullLogHandler;
    }
    else
    {
        internal::sLogHandler = newfunc;
    }
    return old;
}

THREADING_END
