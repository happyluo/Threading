// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_CONFIG_H
#define CONCURRENCY_CONFIG_H

#include <Build/BuildConfig.h>

enum MutexProtocol
{
	PrioInherit,
	PrioNone
};

//
// Let's use these extensions with Util:
//
#ifdef  CONCURRENCY_API_EXPORTS
#	define CONCURRENCY_API	DECLSPEC_EXPORT
#else
#	define CONCURRENCY_API	DECLSPEC_IMPORT
#endif


//
// NAMESPACE 
//
#if defined(__cplusplus)
#	define CONCURRENCY_BEGIN			namespace Util {
#	define CONCURRENCY_END				}
#	define USING_CONCURRENCY			using namespace Util;
#	define CONCURRENCY					::Util::

#else // __cplusplus

#	define CONCURRENCY_BEGIN
#	define CONCURRENCY_END
#	define CONCURRENCY

#endif // __cplusplus

#endif