// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifdef _MSC_VER
#   pragma warning( disable : 4996 )
#endif


#include <Util/InitData.h>
#include <Util/Exception.h>
#include <Util/DisableWarnings.h>
#include <Util/ArgVector.h>
#include <Concurrency/Mutex.h>
#include <Concurrency/MutexPtrLock.h>
#if defined(ICONV_ON_WINDOWS)
//
// On Windows, Util/IcongStringConverter.h is not included by Util/Util.h
//
#	include <Util/IconvStringConverter.h>
#endif

#ifndef _WIN32
#   include <Util/SysLogger.h>

#   include <signal.h>
#   include <syslog.h>
#   include <pwd.h>
#   include <sys/types.h>
#endif

using namespace std;
using namespace Util;
using namespace UtilInternal;

//////////////////////////////////////////////////////////////////////////
/// Args & StringSeq convert.
StringSeq
Util::ArgsToStringSeq(int argc, char* argv[])
{
	StringSeq result;
	for (int i = 0; i < argc; i++)
	{
		result.push_back(argv[i]);
	}
	return result;
}

#ifdef _WIN32

StringSeq
Util::ArgsToStringSeq(int argc, wchar_t* argv[])
{
	return ArgsToStringSeq(argc, argv, 0);
}

StringSeq
Util::ArgsToStringSeq(int argc, wchar_t* argv[], const StringConverterPtr& converter)
{
	StringSeq args;
	for (int i = 0; argv[i] != 0; i++)
	{
		string value = Util::WstringToString(argv[i]);
		value = Util::UTF8ToNative(converter, value);
		args.push_back(value);
	}
	return args;
}

#endif

void
Util::StringSeqToArgs(const StringSeq& args, int& argc, char* argv[])
{
	const int argcOrig = argc;
	int i = 0;
	while (i < argc)
	{
		if (find(args.begin(), args.end(), argv[i]) == args.end())
		{
			for (int j = i; j < argc - 1; j++)
			{
				argv[j] = argv[j + 1];
			}
			--argc;
		}
		else
		{
			++i;
		}
	}

	if (argv && argcOrig != argc)
	{
		argv[argc] = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
/// CreateProperties
PropertiesPtr
Util::CreateProperties(const StringConverterPtr& converter)
{
	return new Properties(converter);
}

PropertiesPtr
Util::CreateProperties(StringSeq& args, const PropertiesPtr& defaults, const StringConverterPtr& converter)
{
	return new Properties(args, defaults, converter);
}

PropertiesPtr
Util::CreateProperties(int& argc, char* argv[], const PropertiesPtr& defaults, const StringConverterPtr& converter)
{
	StringSeq args = ArgsToStringSeq(argc, argv);
	PropertiesPtr properties = CreateProperties(args, defaults, converter);
	StringSeqToArgs(args, argc, argv);
	return properties;
}

//////////////////////////////////////////////////////////////////////////
/// ProcessLogger
static Util::Mutex* sProcessLoggerMutex = 0;
static Util::LoggerPtr sProcessLogger;

namespace
{
class Init
{
public:

	Init()
	{
		sProcessLoggerMutex = new Util::Mutex;
	}

	~Init()
	{
		delete sProcessLoggerMutex;
		sProcessLoggerMutex = 0;
	}
};

Init init;
}

LoggerPtr
Util::GetProcessLogger()
{
	Util::MutexPtrLock<Util::Mutex> lock(sProcessLoggerMutex);

	if (sProcessLogger == 0)
	{
		//
		// TODO: Would be nice to be able to use process name as prefix by default.
		//
		sProcessLogger = new Util::Logger("", "");
	}
	return sProcessLogger;
}

void
Util::SetProcessLogger(const LoggerPtr& logger)
{
	Util::MutexPtrLock<Util::Mutex> lock(sProcessLoggerMutex);
	sProcessLogger = logger;
}

//////////////////////////////////////////////////////////////////////////
/// Version Util
void 
Util::CheckVersion(Int version)
{
#ifndef IGNORE_VERSION

#   if INT_VERSION % 100 > 50
	//
	// Beta version: exact match required
	//
	if (INT_VERSION != version)
	{
		throw VersionMismatchException(__FILE__, __LINE__);
	}
#   else

	//
	// Major and minor version numbers must match.
	//
	if (INT_VERSION / 100 != version / 100)
	{
		throw VersionMismatchException(__FILE__, __LINE__);
	}

	//
	// Reject beta caller
	//
	if (version % 100 > 50)
	{
		throw VersionMismatchException(__FILE__, __LINE__);
	}

	//
	// The caller's patch level cannot be greater than library's patch level. (Patch level changes are
	// backward-compatible, but not forward-compatible.)
	//
	if (version % 100 > INT_VERSION % 100)
	{
		throw VersionMismatchException(__FILE__, __LINE__);
	}

#   endif    
#endif
}

std::string Util::ToStringVersion(Int version)
{
	int majorVersion = (version / 10000);
	int minorVersion = (version / 100) - majorVersion * 100;
	ostringstream os;
	os << majorVersion /* * 10*/ << '.' << minorVersion;

	int patchVersion = version % 100;
	if (patchVersion > 50)
	{
		os << 'b';
		if (patchVersion >= 52)
		{
			os << (patchVersion - 50);
		}
	}
	else if (0 != patchVersion)
	{
		os << '.' << patchVersion;
	}
	
	return os.str();
}

//////////////////////////////////////////////////////////////////////////
/// InitData
Util::InitData::InitData(const std::string& configFile, const std::string& internalCode) :
	m_properties(CreateProperties())
{
	if (!configFile.empty())
	{
		m_properties->Load(configFile);
	}

	std::string code = "" != internalCode ? internalCode : m_properties->GetProperty("Util.InternalCode");
	
	m_stringConverter = "" == code ?
#if defined(_WIN32) && !defined(ICONV_ON_WINDOWS)
		new WindowsStringConverter() : new WindowsStringConverter(code);
#else
		new IconvStringConverter<char>() : new IconvStringConverter<char>(code.c_str());
#endif

#if defined(_WIN32) && !defined(ICONV_ON_WINDOWS)
	m_wstringConverter = new UnicodeWstringConverter();
#else
	if(4 == sizeof(wchar_t))
	{
#	ifdef BIG_ENDIAN
		m_wstringConverter = new Util::IconvStringConverter<wchar_t>("UTF-32BE");
#	else
		m_wstringConverter = new Util::IconvStringConverter<wchar_t>("UTF-32LE");
#	endif
	}
	else
	{
#	ifdef BIG_ENDIAN
		m_wstringConverter = new Util::IconvStringConverter<wchar_t>("UTF-16BE");
#	else
		m_wstringConverter = new Util::IconvStringConverter<wchar_t>("UTF-16LE");
#	endif
	}
#endif

	string logfile = m_properties->GetProperty("Util.LogFile");
#ifndef _WIN32
	if (m_properties->GetPropertyAsInt("Util.UseSyslog") > 0)
	{
		if (!logfile.empty())
		{
			throw InitializationException(__FILE__, __LINE__, "Both syslog and file logger cannot be enabled.");
		}

		m_logger = new SysLogger(m_properties->GetProperty("Util.ProgramName"), 
			m_properties->GetPropertyWithDefault("Util.SyslogFacility", "LOG_USER"));
	}
	else
#endif
	if (!logfile.empty())
	{
		m_logger = new Logger(m_properties->GetProperty("Util.ProgramName"),
			NativeToUTF8(m_stringConverter, logfile));
	}
	else
	{
		m_logger = GetProcessLogger();
	}
}

Util::InitData::InitData(const InitData& initData) :
	m_properties(initData.m_properties)
	, m_logger(initData.m_logger)
	, m_stringConverter(initData.m_stringConverter)
	, m_wstringConverter(initData.m_wstringConverter)
{
	if (!m_properties)
	{
		m_properties = CreateProperties();
	}

	if (0 == m_stringConverter)
	{
		string internalCode = m_properties->GetProperty("Util.InternalCode");

		m_stringConverter = "" == internalCode ?
#if defined(_WIN32) && !defined(ICONV_ON_WINDOWS)
			new WindowsStringConverter() : new WindowsStringConverter(internalCode);
#else
			new IconvStringConverter<char>() : new IconvStringConverter<char>(internalCode.c_str());
#endif
	}

	if (0 == m_wstringConverter)
	{
#if defined(_WIN32) && !defined(ICONV_ON_WINDOWS)
		m_wstringConverter = new UnicodeWstringConverter();
#else
		if(4 == sizeof(wchar_t))
		{
#	ifdef BIG_ENDIAN
			m_wstringConverter = new Util::IconvStringConverter<wchar_t>("UTF-32BE");
#	else
			m_wstringConverter = new Util::IconvStringConverter<wchar_t>("UTF-32LE");
#	endif
		}
		else
		{
#	ifdef BIG_ENDIAN
			m_wstringConverter = new Util::IconvStringConverter<wchar_t>("UTF-16BE");
#	else
			m_wstringConverter = new Util::IconvStringConverter<wchar_t>("UTF-16LE");
#	endif
		}
#endif
	}

	if (!m_logger)
	{
		string logfile = m_properties->GetProperty("Util.LogFile");
#ifndef _WIN32
		if (m_properties->GetPropertyAsInt("Util.UseSyslog") > 0)
		{
			if (!logfile.empty())
			{
				throw InitializationException(__FILE__, __LINE__, "Both syslog and file logger cannot be enabled.");
			}

			m_logger = new SysLogger(m_properties->GetProperty("Util.ProgramName"), 
				m_properties->GetPropertyWithDefault("Util.SyslogFacility", "LOG_USER"));
		}
		else
#endif
		if (!logfile.empty())
		{
			m_logger = new Logger(m_properties->GetProperty("Util.ProgramName"),
				NativeToUTF8(m_stringConverter, logfile));
		}
		else
		{
			m_logger = GetProcessLogger();
		}
	}
}

void
Util::InitData::SetStringConverter(const Util::StringConverterPtr& stringConverter)
{
	m_stringConverter = stringConverter;
}

void
Util::InitData::SetWstringConverter(const Util::WstringConverterPtr& wstringConverter)
{
	m_wstringConverter = wstringConverter;
}

void
Util::InitData::SetLogger(const Util::LoggerPtr& logger)
{
	m_logger = logger;
}
