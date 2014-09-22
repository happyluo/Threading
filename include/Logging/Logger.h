// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_LOGGER_H
#define UTIL_LOGGER_H

#include <Util/Config.h>
#include <Logging/ColorStream.h>
#include <Util/Shared.h>
#include <Util/SharedPtr.h>
#include <Util/FileUtilInternal.h>


namespace Util
{

class Logger;
typedef Util::SharedPtr<Logger> LoggerPtr;

class UTIL_API Logger : public Shared
{
public:

    Logger(const std::string& prefix, const std::string& file);
    ~Logger();

    virtual void Print(const std::string& message);
    virtual void Trace(const std::string& category, const std::string& message);
    virtual void Warning(const std::string& message);
    virtual void Error(const std::string& message);
    virtual LoggerPtr CloneWithPrefix(const std::string& prefix);

protected:
	Logger(){}

private:

	void Write(const std::string& message, bool indent, std::ostream& (*color)(std::ostream &out) = 0);

    std::string m_prefix;
    UtilInternal::ofstream m_out;
    std::string m_file;
};

// ===================================================================
//
// StderrLog	- log all messages to stderr.
// 
// Formats a source file path and a line number as they would appear
// in an error message from the compiler used to compile this code.
UTIL_API ::std::string FormatFileLocation(const char* file, int line);

// Formats a file location for compiler-independent XML output.
// Although this function is not platform dependent, we put it next to
// FormatFileLocation in order to contrast the two functions.
UTIL_API ::std::string FormatCompilerIndependentFileLocation(const char* file,
															   int line);

// Defines logging utilities:
//   STDERR_LOG(severity) - logs messages at the specified severity level. The
//                          message itself is streamed into the macro.
//   LogToStderr()  - directs all log messages to stderr.
//   FlushInfoLog() - flushes informational log messages.

enum LogSeverity 
{
	LOGLEVEL_INFO,     // Informational.  
	LOGLEVEL_WARNING,  // Warns.
	LOGLEVEL_ERROR,    // An error occurred which should never happen during
	                   // normal use.
	LOGLEVEL_FATAL,    // An error occurred from which the library cannot
	                   // recover.  This usually indicates a programming error
	                   // in the code which calls the library, especially when
	                   // compiled in debug mode.

#ifdef NDEBUG
	LOGLEVEL_DFATAL = LOGLEVEL_ERROR
#else
	LOGLEVEL_DFATAL = LOGLEVEL_FATAL
#endif
};

// Formats log entry severity, provides a stream object for streaming the
// log message, and terminates the message with a newline when going out of
// scope.
class UTIL_API StderrLog 
{
public:
	StderrLog(LogSeverity severity, const char* file, int line);

	// Flushes the buffers and, if severity is LOGLEVEL_FATAL, aborts the program.
	~StderrLog();

	::std::ostream& GetStream() { return m_outstream.tostd(); }

private:
	const LogSeverity m_severity;
	colorostream m_outstream;
	const char* m_filename;
	int m_line;

	DISALLOW_COPY_AND_ASSIGN(StderrLog);
};

template<typename T>
inline StderrLog&
operator <<(StderrLog& out, const T& val)
{
	out.GetStream() << val;
	return out;
}

#define STDERR_LOG(severity) \
	::Util::StderrLog(::Util::##severity, __FILE__, __LINE__).GetStream()

//inline void LogToStderr() {}
inline void FlushInfoLog()
{
	fflush(NULL); 
}

// INTERNAL IMPLEMENTATION - DO NOT USE.
//
// CHECK_SUCCESS is an all-mode assert. It aborts the program if the condition
// is not satisfied.
//  Synopsys:
//    CHECK_SUCCESS(boolean_condition);
//     or
//    CHECK_SUCCESS(boolean_condition) << "Additional message";
//
//    This checks the condition and if the condition is not satisfied
//    it prints message about the condition violation, including the
//    condition itself, plus additional message streamed into it, if any,
//    and then it aborts the program. It aborts the program irrespective of
//    whether it is built in the debug mode or not.
#define CHECK_SUCCESS(condition)        \
	AMBIGUOUS_ELSE_BLOCKER              \
	if (condition)                      \
		;                               \
	else                                \
		STDERR_LOG(LOGLEVEL_FATAL) << "Condition " #condition " failed. "

// An all-mode assert to verify that the given POSIX-style function
// call returns 0 (indicating success).  Known limitation: this
// doesn't expand to a balanced 'if' statement, so enclose the macro
// in {} if you need to use it as the only statement in an 'if'
// branch.
#define CHECK_POSIX_SUCCESS(posix_call)				\
	if (const int errorcode = (posix_call))			\
		STDERR_LOG(LOGLEVEL_FATAL) << #posix_call	\
                    << "failed with error " << errorcode


// ===================================================================
// emulates google3/base/logging.h

namespace internal
{
class LogFinisher;

class UTIL_API LogMessage
{
public:
	LogMessage(LogSeverity level, const char* filename, int line);
	~LogMessage();

	LogMessage& operator<<(const std::string& value);
	LogMessage& operator<<(const char* value);
	LogMessage& operator<<(char value);
	LogMessage& operator<<(int value);
	LogMessage& operator<<(uint value);
	LogMessage& operator<<(long value);
	LogMessage& operator<<(unsigned long value);
	LogMessage& operator<<(double value);

private:
	friend class LogFinisher;
	void Finish();

	LogSeverity m_level;
	const char* m_filename;
	int m_line;
	std::string m_message;
};

// Used to make the entire "LOG(BLAH) << etc." expression have a void return
// type and print a newline after each message.
class UTIL_API LogFinisher
{
public:
	void operator=(LogMessage& other);
};

}  // namespace internal

// wingdi.h defines ERROR to be 0. When we call LOG(ERROR), it gets
// substituted with 0, and it expands to COMPACT_GOOGLE_LOG_0. To allow us
// to keep using this syntax, we define this macro to do the same thing
// as COMPACT_GOOGLE_LOG_ERROR, and also define ERROR the same way that
// the Windows SDK does for consistency.
#define ERROR 0
#define LOGLEVEL_0  LOGLEVEL_ERROR

// Undef everything in case we're being mixed with some other library
// which already defined them itself. Presumably all libraries will
// support the same syntax for these so it should not be a big deal if they
// end up using our definitions instead.
#undef UTIL_LOG
#undef UTIL_LOG_IF

#undef UTIL_CHECK
#undef UTIL_CHECK_EQ
#undef UTIL_CHECK_NE
#undef UTIL_CHECK_LT
#undef UTIL_CHECK_LE
#undef UTIL_CHECK_GT
#undef UTIL_CHECK_GE
#undef UTIL_CHECK_NOTNULL

#undef UTIL_DLOG
#undef UTIL_DCHECK
#undef UTIL_DCHECK_EQ
#undef UTIL_DCHECK_NE
#undef UTIL_DCHECK_LT
#undef UTIL_DCHECK_LE
#undef UTIL_DCHECK_GT
#undef UTIL_DCHECK_GE

#define UTIL_LOG(LEVEL)                               \
	::Util::internal::LogFinisher() =                 \
	::Util::internal::LogMessage(                     \
	::Util::LOGLEVEL_##LEVEL, __FILE__, __LINE__)
#define UTIL_LOG_IF(LEVEL, CONDITION)                 \
	!(CONDITION) ? (void)0 : UTIL_LOG(LEVEL)

#define UTIL_CHECK(EXPRESSION)                        \
	UTIL_LOG_IF(FATAL, !(EXPRESSION)) << "CHECK failed: " #EXPRESSION ": "
#define UTIL_CHECK_EQ(A, B) UTIL_CHECK((A) == (B))
#define UTIL_CHECK_NE(A, B) UTIL_CHECK((A) != (B))
#define UTIL_CHECK_LT(A, B) UTIL_CHECK((A) <  (B))
#define UTIL_CHECK_LE(A, B) UTIL_CHECK((A) <= (B))
#define UTIL_CHECK_GT(A, B) UTIL_CHECK((A) >  (B))
#define UTIL_CHECK_GE(A, B) UTIL_CHECK((A) >= (B))

#define UTIL_LOG_ASSERT(condition)  \
	UTIL_LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition ". "

namespace internal
{
template<typename T>
T* CheckNotNull(const char* /* file */, int /* line */, const char* name, T* val)
{
	if (val == NULL)
	{
		UTIL_LOG(FATAL) << name;
	}
	return val;
}
}  // namespace internal

#define UTIL_CHECK_NOTNULL(A) \
	::Util::internal::CheckNotNull(__FILE__, __LINE__, "'" #A "' must not be NULL", (A))

#ifdef NDEBUG

#define UTIL_DLOG UTIL_LOG_IF(INFO, false)

#define UTIL_DLOG_EAT_STREAM_PARAMETERS  while(false) UTIL_CHECK(EXPRESSION)

#define UTIL_DCHECK(EXPRESSION) UTIL_DLOG_EAT_STREAM_PARAMETERS
#define UTIL_DCHECK_EQ(A, B) UTIL_DCHECK((A) == (B))
#define UTIL_DCHECK_NE(A, B) UTIL_DCHECK((A) != (B))
#define UTIL_DCHECK_LT(A, B) UTIL_DCHECK((A) <  (B))
#define UTIL_DCHECK_LE(A, B) UTIL_DCHECK((A) <= (B))
#define UTIL_DCHECK_GT(A, B) UTIL_DCHECK((A) >  (B))
#define UTIL_DCHECK_GE(A, B) UTIL_DCHECK((A) >= (B))

#define UTIL_DLOG_ASSERT   UTIL_DLOG_EAT_STREAM_PARAMETERS

#else  // NDEBUG

#define UTIL_DLOG          UTIL_LOG

#define UTIL_DCHECK        UTIL_CHECK
#define UTIL_DCHECK_EQ     UTIL_CHECK_EQ
#define UTIL_DCHECK_NE     UTIL_CHECK_NE
#define UTIL_DCHECK_LT     UTIL_CHECK_LT
#define UTIL_DCHECK_LE     UTIL_CHECK_LE
#define UTIL_DCHECK_GT     UTIL_CHECK_GT
#define UTIL_DCHECK_GE     UTIL_CHECK_GE

#define UTIL_DLOG_ASSERT   UTIL_LOG_ASSERT

#endif  // !NDEBUG

#define NOTREACHED() UTIL_DCHECK(false)

// Redefine the standard assert to use our nice log files
#undef assert
#define assert(x) UTIL_DLOG_ASSERT(x)

//
// for test
//
#undef EXPECT_TEST
#undef EXPECT_EQ
#undef EXPECT_NE
#undef EXPECT_LT
#undef EXPECT_LE
#undef EXPECT_GT
#undef EXPECT_GE
#undef EXPECT_TRUE
#undef EXPECT_FALSE

#define TEST_LOG(LEVEL)                               \
	::Util::internal::LogFinisher() =                 \
	::Util::internal::LogMessage(                     \
	::Util::LOGLEVEL_##LEVEL, __FILE__, __LINE__)
#define TEST_LOG_IF(LEVEL, CONDITION)                 \
	!(CONDITION) ? (void)0 : TEST_LOG(LEVEL)

#define EXPECT_TEST(EXPRESSION)                       \
	TEST_LOG_IF(FATAL, !(EXPRESSION)) << "CHECK failed: " #EXPRESSION ": "
#define EXPECT_EQ(A, B) EXPECT_TEST((A) == (B))
#define EXPECT_NE(A, B) EXPECT_TEST((A) != (B))
#define EXPECT_LT(A, B) EXPECT_TEST((A) <  (B))
#define EXPECT_LE(A, B) EXPECT_TEST((A) <= (B))
#define EXPECT_GT(A, B) EXPECT_TEST((A) >  (B))
#define EXPECT_GE(A, B) EXPECT_TEST((A) >= (B))
#define EXPECT_TRUE(exp) EXPECT_TEST(exp)
#define EXPECT_FALSE(exp) EXPECT_TEST(!exp)

#define ASSERT_TEST(condition)  \
	TEST_LOG_IF(FATAL, !(condition)) << "ASSERT failed: " #condition ". "

#define ASSERT_EQ(A, B) ASSERT_TEST((A) == (B))

UTIL_API LoggerPtr SetLogger(LoggerPtr newlogger);

typedef void LogHandler(LogSeverity level, const char* filename, int line,
						const std::string& message, LoggerPtr logger);

UTIL_API LogHandler* SetLogHandler(LogHandler* newfunc);

}

// The NOTIMPLEMENTED() macro annotates codepaths which have
// not been implemented yet.
//
// The implementation of this macro is controlled by NOTIMPLEMENTED_POLICY:
//   0 -- Do nothing (stripped by compiler)
//   1 -- Warn at compile time
//   2 -- Fail at compile time
//   3 -- Fail at runtime (DCHECK)
//   4 -- [default] LOG(ERROR) at runtime
//   5 -- LOG(ERROR) at runtime, only once per call-site

#ifndef NOTIMPLEMENTED_POLICY
// Select default policy: LOG(ERROR)
#define NOTIMPLEMENTED_POLICY 4
#endif

#if defined(__GNUC__)	// COMPILER_GCC
// On Linux, with GCC, we can use __PRETTY_FUNCTION__ to get the demangled name
// of the current function in the NOTIMPLEMENTED message.
#	define NOTIMPLEMENTED_MSG "Not implemented reached in " << __PRETTY_FUNCTION__
#else
#	define NOTIMPLEMENTED_MSG "NOT IMPLEMENTED"
#endif

#if NOTIMPLEMENTED_POLICY == 0
#	define NOTIMPLEMENTED() ;
#elif NOTIMPLEMENTED_POLICY == 1
// TODO, figure out how to generate a warning
#	define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 2
#	define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 3
#	define NOTIMPLEMENTED() NOTREACHED()
#elif NOTIMPLEMENTED_POLICY == 4
#	define NOTIMPLEMENTED() UTIL_LOG(ERROR) << NOTIMPLEMENTED_MSG
#elif NOTIMPLEMENTED_POLICY == 5
#	define NOTIMPLEMENTED() do {                           \
	static int count = 0;                                  \
	UTIL_LOG_IF(ERROR, 0 == count++) << NOTIMPLEMENTED_MSG;\
} while(0)
#endif

#endif
