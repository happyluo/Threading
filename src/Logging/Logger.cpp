// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Util/Time.h>
#include <Concurrency/Mutex.h>
#include <Concurrency/MutexPtrLock.h>
#include <Unicoder/StringConverter.h>
#include <Logging/Logger.h>
#include <Util/StringUtil.h>

using namespace std;
using namespace Util;
using namespace UtilInternal;

namespace
{

Util::Mutex* outputMutex = 0;

class Init
{
public:

    Init()
    {
        outputMutex = new Util::Mutex;
    }

    ~Init()
    {
        delete outputMutex;
        outputMutex = 0;
    }
};

Init init;

}

Util::Logger::Logger(const string& prefix, const string& file)
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


Util::Logger::~Logger()
{
    if (m_out.is_open())
    {
        m_out.close();
    }
}

void
Util::Logger::Print(const string& message)
{
    Write(message, false);
}

void
Util::Logger::Trace(const string& category, const string& message)
{
    string s = "--[  INFO ] " + Util::Time::Now().ToDateTime() + " " + m_prefix;
    if (!category.empty())
    {
        s += category + ": ";
    }
    s += message;

    Write(s, true);
}

void
Util::Logger::Warning(const string& message)
{
    Write("-![WARNING] " + Util::Time::Now().ToDateTime() + " " + m_prefix + "warning: " + message, true, fgyellow);
}

void
Util::Logger::Error(const string& message)
{
    Write("!![ ERROR ] " + Util::Time::Now().ToDateTime() + " " + m_prefix + "error: " + message, true, fgred);
}

LoggerPtr
Util::Logger::CloneWithPrefix(const std::string& prefix)
{
    return new Logger(prefix, m_file);
}

void
Util::Logger::Write(const string& message, bool indent, ostream& (*color)(ostream &out))
{
    Util::MutexPtrLock<Util::Mutex> sync(outputMutex);

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

// ===================================================================
//
// StderrLog	- log all messages to stderr.
// 
const char kUnknownFile[] = "unknown file";

// Formats a source file path and a line number as they would appear
// in an error message from the compiler used to compile this code.
UTIL_API ::std::string Util::FormatFileLocation(const char* file, int line)
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

// Formats a file location for compiler-independent XML output.
// Although this function is not platform dependent, we put it next to
// FormatFileLocation in order to contrast the two functions.
// Note that FormatCompilerIndependentFileLocation() does NOT append colon
// to the file location it produces, unlike FormatFileLocation().
UTIL_API ::std::string Util::FormatCompilerIndependentFileLocation(
	const char* file, int line) 
{
		const char* const file_name = file == NULL ? kUnknownFile : file;

		if (line < 0)
		{
			return file_name;
		}
		else
		{
			return Format("%s:%d", file_name, line).c_str();
		}
}


Util::StderrLog::StderrLog(LogSeverity severity, const char* file, int line) :
	m_severity(severity) 
	, m_outstream(cerr)
	, m_filename(file)
	, m_line(line)
{
	const char* const marker =
		severity == LOGLEVEL_INFO ?    "[  INFO ]" :
		severity == LOGLEVEL_WARNING ? "[WARNING]" :
		severity == LOGLEVEL_ERROR ?   "[ ERROR ]" : "[ FATAL ]";

	::std::ostream& (*color)(::std::ostream &) = 
		severity == LOGLEVEL_INFO ? &dft<char> :
		severity == LOGLEVEL_WARNING ? &fgyellow<char> :
		severity == LOGLEVEL_ERROR ?   &fgred<char> : &fgred<char>;

	GetStream() << ::std::endl << color << marker << " "
		<< FormatFileLocation(file, line).c_str() << ": ";
}

// Flushes the buffers and, if severity is LOGLEVEL_FATAL, aborts the program.
Util::StderrLog::~StderrLog() 
{
	//GetStream() << ::std::endl;
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

// ===================================================================
// emulates google3/base/logging.cc

//#include <Util/Shutdown.h>
#include <stdio.h>

namespace Util
{
namespace internal 
{

void DefaultLogHandler(LogSeverity level
					   , const char* filename
					   , int line
					   , const string& message
					   , LoggerPtr logger)
{
	static const char* slevelNames[] = { "[  INFO ]", "[WARNING]", "[ ERROR ]", "[ FATAL ]" };
#if 0	

	// We use fprintf() instead of cerr because we want this to work at static
	// initialization time.
	//fprintf(stderr, "[Util %s %s:%d] %s\n",
	//	slevelNames[level], filename, line, message.c_str());
	fprintf(stderr, "%s %s(%d): %s\n",
		slevelNames[level], filename, line, message.c_str());
	fflush(stderr);  // Needed on MSVC.
#else
	if (!logger)
	{
		::Util::StderrLog(level, filename, line) << message << "\n";
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
#endif
}

void NullLogHandler(LogSeverity     /* level */
					, const char*   /* filename */
					, int           /* line */
					, const string& /* message */
					, LoggerPtr     /* logger */) 
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
#define DECLARE_STREAM_OPERATOR(TYPE, FORMAT)                           \
	LogMessage& LogMessage::operator<<(TYPE value) {                    \
		/* 128 bytes should be big enough for any of the primitive */   \
		/* values which we print with this, but well use snprintf() */  \
		/* anyway to be extra safe. */                                  \
		char buffer[128];                                               \
		snprintf(buffer, sizeof(buffer), FORMAT, value);                \
		/* Guard against broken MSVC snprintf(). */                     \
		buffer[sizeof(buffer)-1] = '\0';                                \
		m_message += buffer;                                            \
		return *this;                                                   \
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

Util::LoggerPtr SetLogger(Util::LoggerPtr newlogger)
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

}
