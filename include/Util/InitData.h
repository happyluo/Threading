// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_INIT_DATA_H
#define UTIL_INIT_DATA_H

#include <Util/Config.h>
#include <Logging/Logger.h>
#include <Util/Properties.h>
#include <Unicoder/StringConverter.h>


namespace Util
{

//UTIL_API void CollectGarbage();

//////////////////////////////////////////////////////////////////////////
/// Args & StringSeq convert.
UTIL_API StringSeq ArgsToStringSeq(int, char*[]);

#ifdef _WIN32

UTIL_API StringSeq ArgsToStringSeq(int, wchar_t*[]);

UTIL_API StringSeq ArgsToStringSeq(int, wchar_t*[], const StringConverterPtr&);

#endif

UTIL_API void StringSeqToArgs(const StringSeq&, int&, char*[]);

//////////////////////////////////////////////////////////////////////////
/// CreateProperties
UTIL_API PropertiesPtr CreateProperties(const Util::StringConverterPtr& /*converter*/= 0);
UTIL_API PropertiesPtr CreateProperties(StringSeq& /*args*/, const PropertiesPtr& /*defaults*/= 0, 
										const Util::StringConverterPtr& /*converter*/= 0);
UTIL_API PropertiesPtr CreateProperties(int& /*argc*/, char* /*argv*/[], const PropertiesPtr& /*defaults*/= 0, 
										const Util::StringConverterPtr& /*converter*/= 0);

//////////////////////////////////////////////////////////////////////////
/// ProcessLogger
UTIL_API LoggerPtr GetProcessLogger();
UTIL_API void SetProcessLogger(const LoggerPtr&);

//////////////////////////////////////////////////////////////////////////
/// Version Util
UTIL_API void CheckVersion(Int version = INT_VERSION);

UTIL_API std::string ToStringVersion(Int version = INT_VERSION);

struct UTIL_API InitData
{
	InitData(const std::string& configFile = "", const std::string& internalCode = "");
	InitData(const InitData& initData);

	void SetStringConverter(const Util::StringConverterPtr& stringConverter);
	void SetWstringConverter(const Util::WstringConverterPtr& wstringConverter);
	void SetLogger(const Util::LoggerPtr& logger);
	

    PropertiesPtr m_properties;
    LoggerPtr m_logger;
    StringConverterPtr m_stringConverter;
    WstringConverterPtr m_wstringConverter;
    
};

}


#endif
