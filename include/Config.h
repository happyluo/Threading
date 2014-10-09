// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef THREADING_CONFIG_H
#define THREADING_CONFIG_H

#ifndef NOMINMAX
#    define NOMINMAX
#endif

#include <Build/BuildConfig.h>
#include <Build/UsefulMacros.h>
#include <Util/Port.h>

//
// Some include files we need almost everywhere.
//
#include <ctype.h>   // for isspace, etc
#include <stddef.h>  // for ptrdiff_t
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string> 
#include <vector>

#if defined(LANG_CPP11)
#    include <functional>
#else
#    if defined(_WIN32)
#        include <functional>
#    else
#        include <tr1/functional>
#    endif

namespace std {
    using tr1::function;
    using tr1::bind;
    using tr1::ref;
    namespace placeholders = tr1::placeholders;
    using tr1::hash;
}

#endif

//
// Let's use these extensions with Threading:
//
#ifdef  THREADING_API_EXPORTS
#    define THREADING_API    DECLSPEC_EXPORT
#else
#    define THREADING_API    DECLSPEC_IMPORT
#endif

//
// NAMESPACE 
//
#if defined(__cplusplus)
#    define THREADING_BEGIN            namespace Threading {
#    define THREADING_END              }
#    define USING_THREADING            using namespace Threading;
#    define THREADING                  ::Threading::

#else // __cplusplus

#    define THREADING_BEGIN
#    define THREADING_END
#    define THREADING

#endif // __cplusplus

namespace Threading
{

enum MutexProtocol
{
    PrioInherit,
    PrioNone
};

//
// Calculate current CPU's endianness
//
enum EndianType
{
    BigEndian = 0,        //BigEndian
    LittleEndian,        //LittleEndian
};

inline bool IsBigEndian() 
{
#ifdef BIG_ENDIAN
    return true;
#else
    return false;
#endif

    int iValue = 1;
    unsigned char cValue = *((unsigned char*)&iValue);
    return 0 == cValue;
}

inline EndianType CurrentEndian()
{
#ifdef BIG_ENDIAN
    return BigEndian;
#else
    return LittleEndian;
#endif
}

// TODO: Should not be inline, this is not performance critical.
#ifdef _WIN32
    inline int GetSystemErrno() { return GetLastError(); }
#else
    inline int GetSystemErrno() { return errno; }
#endif


//
// Int64 typedef
//
#if defined(_MSC_VER)
    //
    // With Visual C++,, long is always 32-bit
    //
    typedef __int64                Int64;
    typedef unsigned __int64       UInt64;
#elif defined(UTIL_64)
    typedef long                   Int64;
    typedef unsigned long          UInt64;
#else
    typedef long long              Int64;
    typedef unsigned long long     UInt64;  
#endif

typedef unsigned char            Byte;
typedef short                    Short;
typedef int                      Int;
typedef Threading::Int64         Long;
typedef float                    Float;
typedef double                   Double;

// A sequence of bools. 
typedef std::vector<bool>            BoolSeq;
// A sequence of bytes. 
typedef std::vector<Byte>            ByteSeq;
// A sequence of shorts. 
typedef std::vector<Short>           ShortSeq;
// A sequence of ints. 
typedef std::vector<Int>             IntSeq;
// A sequence of longs. 
typedef std::vector<Long>            LongSeq;
// A sequence of floats. 
typedef std::vector<Float>           FloatSeq;
// A sequence of doubles. 
typedef std::vector<Double>          DoubleSeq;
// A sequence of strings. 
typedef std::vector<std::string>     StringSeq;

}


//  NO_STDC_NAMESPACE workaround  --------------------------------------------//
//  Because std::size_t usage is so common, even in headers which do not
//  otherwise use the C library, the <cstddef> workaround is included here so
//  that ugly workaround code need not appear in many other boost headers.
//  NOTE WELL: This is a workaround for non-conforming compilers; <cstddef>
//  must still be #included in the usual places so that <cstddef> inclusion
//  works as expected with standard conforming compilers.  The resulting
//  double inclusion of <cstddef> is harmless.

#if defined(NO_STDC_NAMESPACE) && defined(__cplusplus)
#    include <cstddef>
     namespace std { using ::ptrdiff_t; using ::size_t; }
#    define ADD_TO_STD(Symbol)    namespace std { using ::Symbol; }
#endif

// A secret type that users don't know about.  It has no
// definition on purpose.  Therefore it's impossible to create a
// Secret object, which is what we want.
class Secret;

//
// The version information.
//
#define STRING_VERSION "1.0"    // "A.B.C", with A=major, B=minor, C=patch
#define INT_VERSION 10000        // AABBCC, with AA=major, BB=minor, CC=patch

#endif