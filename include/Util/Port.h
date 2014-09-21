// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_PORT_H
#define UTIL_PORT_H

#include <stdlib.h>
#include <stdio.h>

// Determines the version of gcc that is used to compile this.
#ifdef __GNUC__
// 40302 means version 4.3.2.
# define GCC_VER_ \
	(__GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL__)
#endif  // __GNUC__

// Determines the platform on which this library is compiled.
#ifdef __CYGWIN__
#	define OS_CYGWIN 1
#elif defined __SYMBIAN32__
#	define OS_SYMBIAN 1
#elif defined(_WIN32) || defined(_WINDOWS)
#	define OS_WINDOWS 1
#	ifdef _WIN32_WCE
#		define OS_WINDOWS_MOBILE 1
#	elif defined(__MINGW__) || defined(__MINGW32__)
#		define OS_WINDOWS_MINGW 1
#	else
#		define OS_WINDOWS_DESKTOP 1
# endif  // _WIN32_WCE
#elif defined __APPLE__
#	define OS_MAC 1
#	if TARGET_OS_IPHONE
#		define OS_IOS 1
#		if TARGET_IPHONE_SIMULATOR
#			define OS_IOS_SIMULATOR 1
#		endif
#	endif
#elif defined __linux__
#	define OS_LINUX 1
#	if defined __ANDROID__
#		define OS_LINUX_ANDROID 1
#	endif
#elif defined __MVS__
#	define OS_ZOS 1
#elif defined(__sun) && defined(__SVR4)
#	define OS_SOLARIS 1
#elif defined(_AIX)
#	define OS_AIX 1
#elif defined(__hpux)
#	define OS_HPUX 1
#elif defined __native_client__
#	define OS_NACL 1
#elif defined __OpenBSD__
#	define OS_OPENBSD 1
#elif defined __QNX__
#	define OS_QNX 1
#endif  // __CYGWIN__

#ifndef LANG_CXX11
// gcc and clang define __GXX_EXPERIMENTAL_CXX0X__ when
// -std={c,gnu}++{0x,11} is passed.  The C++11 standard specifies a
// value for __cplusplus, and recent versions of clang, gcc, and
// probably other compilers set that too in C++11 mode.
#	if __GXX_EXPERIMENTAL_CXX0X__ || __cplusplus >= 201103L
// Compiling in at least C++11 mode.
#		define LANG_CXX11 1
#	else
#		define LANG_CXX11 0
#	endif
#endif

// C++11 specifies that <initializer_list> provides std::initializer_list. Use
// that if C++11 mode available and libstdc++ isn't very old (binaries
// targeting OS X 10.6 can build with clang but need to use gcc4.2's
// libstdc++).
#if LANG_CXX11 && (!defined(__GLIBCXX__) || __GLIBCXX__ > 20110325)
#	define HAS_STD_INITIALIZER_LIST_ 1
#endif

// Brings in definitions for functions used in the testing::internal::posix
// namespace (read, write, close, chdir, isatty, stat). We do not currently
// use them on Windows Mobile.
#if !OS_WINDOWS
// This assumes that non-Windows OSes provide unistd.h. For OSes where this
// is not the case, we need to include headers that provide the functions
// mentioned above.
#	include <unistd.h>
#	include <strings.h>
#elif !OS_WINDOWS_MOBILE
#	include <direct.h>
#	include <io.h>
#endif

#if OS_LINUX_ANDROID
// Used to define __ANDROID_API__ matching the target NDK API level.
#	include <android/api-level.h>  // NOLINT
#endif


#if !defined(HAS_STD_STRING)
// Even though we don't use this macro any longer, we keep it in case
// some clients still depend on it.
#	define HAS_STD_STRING 1
#elif !HAS_STD_STRING
// The user told us that ::std::string isn't available.
#	error "Util cannot be used where ::std::string isn't available."
#endif  // !defined(HAS_STD_STRING)

#ifndef HAS_GLOBAL_STRING
// The user didn't tell us whether ::string is available, so we need
// to figure it out.

#	define HAS_GLOBAL_STRING 0

#endif  // HAS_GLOBAL_STRING

#ifndef HAS_STD_WSTRING
// The user didn't tell us whether ::std::wstring is available, so we need
// to figure it out.

// Cygwin 1.7 and below doesn't support ::std::wstring.
// Solaris' libc++ doesn't support it either.  Android has
// no support for it at least as recent as Froyo (2.2).
#	define HAS_STD_WSTRING \
		(!(OS_LINUX_ANDROID || OS_CYGWIN || OS_SOLARIS))

#endif  // HAS_STD_WSTRING

#ifndef HAS_GLOBAL_WSTRING
// The user didn't tell us whether ::wstring is available, so we need
// to figure it out.
#	define HAS_GLOBAL_WSTRING \
		(HAS_STD_WSTRING && HAS_GLOBAL_STRING)
#endif  // HAS_GLOBAL_WSTRING

// Determines whether RTTI is available.
#ifndef HAS_RTTI
// The user didn't tell us whether RTTI is enabled, so we need to
// figure it out.

#	ifdef _MSC_VER

#		ifdef _CPPRTTI  // MSVC defines this macro iff RTTI is enabled.
#			define HAS_RTTI 1
#		else
#			define HAS_RTTI 0
#	endif

// Starting with version 4.3.2, gcc defines __GXX_RTTI iff RTTI is enabled.
#	elif defined(__GNUC__) && (GCC_VER_ >= 40302)

#		ifdef __GXX_RTTI
// When building against STLport with the Android NDK and with
// -frtti -fno-exceptions, the build fails at link time with undefined
// references to __cxa_bad_typeid. Note sure if STL or toolchain bug,
// so disable RTTI when detected.
#			if OS_LINUX_ANDROID && defined(_STLPORT_MAJOR) && \
			!defined(__EXCEPTIONS)
#				define HAS_RTTI 0
#			else
#				define HAS_RTTI 1
#			endif  // OS_LINUX_ANDROID && __STLPORT_MAJOR && !__EXCEPTIONS
#		else
#			define HAS_RTTI 0
#		endif  // __GXX_RTTI

// Clang defines __GXX_RTTI starting with version 3.0, but its manual recommends
// using has_feature instead. has_feature(cxx_rtti) is supported since 2.7, the
// first version with C++ support.
#	elif defined(__clang__)

#		define HAS_RTTI __has_feature(cxx_rtti)

// Starting with version 9.0 IBM Visual Age defines __RTTI_ALL__ to 1 if
// both the typeid and dynamic_cast features are present.
#	elif defined(__IBMCPP__) && (__IBMCPP__ >= 900)

#		ifdef __RTTI_ALL__
#			define HAS_RTTI 1
#		else
#			define HAS_RTTI 0
#		endif

#	else

// For all other compilers, we assume RTTI is enabled.
#		define HAS_RTTI 1

#	endif  // _MSC_VER

#endif  // HAS_RTTI

// It's this header's responsibility to #include <typeinfo> when RTTI
// is enabled.
#if HAS_RTTI
#	include <typeinfo>
#endif

// Determines whether we can use the pthreads library.
#ifndef HAS_PTHREAD
// The user didn't tell us explicitly, so we assume pthreads support is
// available on Linux and Mac.
//
#	define HAS_PTHREAD (OS_LINUX || OS_MAC || OS_HPUX || OS_QNX)
#endif  // HAS_PTHREAD

#if HAS_PTHREAD
// Port.h guarantees to #include <pthread.h> when HAS_PTHREAD is
// true.
#	include <pthread.h>  // NOLINT

// For timespec and nanosleep, used below.
#	include <time.h>  // NOLINT
#endif


// At this time, libstdc++ 4.0.0+ and
// MSVC 2010 are the only mainstream standard libraries that come
// with a TR1 tuple implementation.  NVIDIA's CUDA NVCC compiler
// pretends to be GCC by defining __GNUC__ and friends, but cannot
// compile GCC's tuple implementation.  MSVC 2008 (9.0) provides TR1
// tuple in a 323 MB Feature Pack download, which we cannot assume the
// user has.  QNX's QCC compiler is a modified GCC but it doesn't
// support TR1 tuple.  libc++ only provides std::tuple, in C++11 mode,
// and it can be used with some compilers that define __GNUC__.
#if (defined(__GNUC__) && !defined(__CUDACC__) && (GCC_VER_ >= 40000) \
	&& !OS_QNX && !defined(_LIBCPP_VERSION)) || _MSC_VER >= /*1400*/1600
# define ENV_HAS_TR1_TUPLE 1
#endif

// C++11 specifies that <tuple> provides std::tuple. Use that if gtest is used
// in C++11 mode and libstdc++ isn't very old (binaries targeting OS X 10.6
// can build with clang but need to use gcc4.2's libstdc++).
#if LANG_CXX11 && (!defined(__GLIBCXX__) || __GLIBCXX__ > 20110325)
# define ENV_HAS_STD_TUPLE 1
#endif


// Determines whether we can use tr1/tuple.  You can define
// this macro to 0 to prevent using tuple (any
// feature depending on tuple with be disabled in this mode).
#ifndef HAS_TR1_TUPLE
# if OS_LINUX_ANDROID && defined(_STLPORT_MAJOR)
// STLport, provided with the Android NDK, has neither <tr1/tuple> or <tuple>.
#  define HAS_TR1_TUPLE 0
# elif ENV_HAS_TR1_TUPLE || ENV_HAS_STD_TUPLE
#  define HAS_TR1_TUPLE 1
# else
#  define HAS_TR1_TUPLE 0
# endif
#endif  // HAS_TR1_TUPLE

// To avoid conditional compilation everywhere, we make it
// gtest-port.h's responsibility to #include the header implementing
// tr1/tuple.
#if HAS_TR1_TUPLE

#if ENV_HAS_STD_TUPLE
# include <tuple>
// C++11 puts its tuple into the ::std namespace rather than
// ::std::tr1.  gtest expects tuple to live in ::std::tr1, so put it there.
// This causes undefined behavior, but supported compilers react in
// the way we intend.
namespace std {
namespace tr1 {
using ::std::get;
using ::std::make_tuple;
using ::std::tuple;
using ::std::tuple_element;
using ::std::tuple_size;
}
}

# elif OS_SYMBIAN

// On Symbian, BOOST_HAS_TR1_TUPLE causes Boost's TR1 tuple library to
// use STLport's tuple implementation, which unfortunately doesn't
// work as the copy of STLport distributed with Symbian is incomplete.
// By making sure BOOST_HAS_TR1_TUPLE is undefined, we force Boost to
// use its own tuple implementation.
#  ifdef BOOST_HAS_TR1_TUPLE
#   undef BOOST_HAS_TR1_TUPLE
#  endif  // BOOST_HAS_TR1_TUPLE

// This prevents <boost/tr1/detail/config.hpp>, which defines
// BOOST_HAS_TR1_TUPLE, from being #included by Boost's <tuple>.
#  define BOOST_TR1_DETAIL_CONFIG_HPP_INCLUDED
#  include <tuple>

# elif defined(__GNUC__) && (GCC_VER_ >= 40000)
// GCC 4.0+ implements tr1/tuple in the <tr1/tuple> header.  This does
// not conform to the TR1 spec, which requires the header to be <tuple>.

#  if !HAS_RTTI && GCC_VER_ < 40302
// Until version 4.3.2, gcc has a bug that causes <tr1/functional>,
// which is #included by <tr1/tuple>, to not compile when RTTI is
// disabled.  _TR1_FUNCTIONAL is the header guard for
// <tr1/functional>.  Hence the following #define is a hack to prevent
// <tr1/functional> from being included.
#   define _TR1_FUNCTIONAL 1
#   include <tr1/tuple>
#   undef _TR1_FUNCTIONAL  // Allows the user to #include
// <tr1/functional> if he chooses to.
#  else
#   include <tr1/tuple>  // NOLINT
#  endif  // !HAS_RTTI && GCC_VER_ < 40302

# else if ENV_HAS_TR1_TUPLE
// If the compiler is not GCC 4.0+, we assume the user is using a
// spec-conforming TR1 implementation.
#  include <tuple>  // NOLINT
# endif  // USE_OWN_TR1_TUPLE

#endif  // HAS_TR1_TUPLE


// Determines whether clone(2) is supported.
// Usually it will only be available on Linux, excluding
// Linux on the Itanium architecture.
// Also see http://linux.die.net/man/2/clone.
#ifndef HAS_CLONE
// The user didn't tell us, so we need to figure it out.

#	if OS_LINUX && !defined(__ia64__)
#		if OS_LINUX_ANDROID
// On Android, clone() is only available on ARM starting with Gingerbread.
#			if defined(__arm__) && __ANDROID_API__ >= 9
#				define HAS_CLONE 1
#			else
#				define HAS_CLONE 0
#			endif
#		else
#			define HAS_CLONE 1
#		endif
#	else
#		define HAS_CLONE 0
#	endif  // OS_LINUX && !defined(__ia64__)

#endif  // HAS_CLONE

// Determines whether the system compiler uses UTF-16 for encoding wide strings.
#define WIDE_STRING_USES_UTF16 \
	(OS_WINDOWS || OS_CYGWIN || OS_SYMBIAN || OS_AIX)


// A function level attribute to disable checking for use of uninitialized
// memory when built with MemorySanitizer.
#if defined(__clang__)
#	if __has_feature(memory_sanitizer)
#		define ATTRIBUTE_NO_SANITIZE_MEMORY __attribute__((no_sanitize_memory))
#	else
#		define ATTRIBUTE_NO_SANITIZE_MEMORY
#	endif
#else
#	define ATTRIBUTE_NO_SANITIZE_MEMORY
#endif

//
// includes
//
#if OS_LINUX

# define HAS_GETTIMEOFDAY 1

# include <fcntl.h>  // NOLINT
# include <limits.h>  // NOLINT
# include <sched.h>  // NOLINT
// Declares vsnprintf().  This header is not available on Windows.
# include <strings.h>  // NOLINT
# include <sys/mman.h>  // NOLINT
# include <sys/time.h>  // NOLINT
# include <unistd.h>  // NOLINT
# include <string>

#elif OS_SYMBIAN
# define HAS_GETTIMEOFDAY 1
# include <sys/time.h>  // NOLINT

#elif OS_ZOS
# define HAS_GETTIMEOFDAY 1
# include <sys/time.h>  // NOLINT

// On z/OS we additionally need strings.h for strcasecmp.
# include <strings.h>  // NOLINT

#elif OS_WINDOWS_MOBILE  // We are on Windows CE.

# include <windows.h>  // NOLINT

#elif OS_WINDOWS  // We are on Windows proper.

# include <io.h>  // NOLINT
# include <sys/timeb.h>  // NOLINT
# include <sys/types.h>  // NOLINT
# include <sys/stat.h>  // NOLINT

# if OS_WINDOWS_MINGW
// MinGW has gettimeofday() but not _ftime64().
// TODO: There are other ways to get the time on
//   Windows, like GetTickCount() or GetSystemTimeAsFileTime().  MinGW
//   supports these.  consider using them instead.
#  define HAS_GETTIMEOFDAY 1
#  include <sys/time.h>  // NOLINT
# endif  // OS_WINDOWS_MINGW

// cpplint thinks that the header is already included, so we want to
// silence it.
# include <windows.h>  // NOLINT

#else

// Assume other platforms have gettimeofday().
# define HAS_GETTIMEOFDAY 1

// cpplint thinks that the header is already included, so we want to
// silence it.
# include <sys/time.h>  // NOLINT
# include <unistd.h>  // NOLINT

#endif  // OS_LINUX

#if CAN_STREAM_RESULTS_
# include <arpa/inet.h>  // NOLINT
# include <netdb.h>  // NOLINT
#endif


namespace Util
{
namespace Posix
{

	// Functions with a different name on Windows.

#if OS_WINDOWS

	typedef struct _stat StatStruct;

# ifdef __BORLANDC__
	inline int IsATTY(int fd) { return isatty(fd); }
	inline int StrCaseCmp(const char* s1, const char* s2) {
		return stricmp(s1, s2);
	}
	inline char* StrDup(const char* src) { return strdup(src); }
# else  // !__BORLANDC__
#  if OS_WINDOWS_MOBILE
	inline int IsATTY(int /* fd */) { return 0; }
#  else
	inline int IsATTY(int fd) { return _isatty(fd); }
#  endif  // OS_WINDOWS_MOBILE
	inline int StrCaseCmp(const char* s1, const char* s2) {
		return _stricmp(s1, s2);
	}
	inline char* StrDup(const char* src) { return _strdup(src); }
# endif  // __BORLANDC__

# if OS_WINDOWS_MOBILE
	inline int FileNo(FILE* file) { return reinterpret_cast<int>(_fileno(file)); }
	// Stat(), RmDir(), and IsDir() are not needed on Windows CE at this
	// time and thus not defined there.
# else
	inline int FileNo(FILE* file) { return _fileno(file); }
	inline int Stat(const char* path, StatStruct* buf) { return _stat(path, buf); }
	inline int RmDir(const char* dir) { return _rmdir(dir); }
	inline bool IsDir(const StatStruct& st) {
		return (_S_IFDIR & st.st_mode) != 0;
	}
# endif  // OS_WINDOWS_MOBILE

#else

	typedef struct stat StatStruct;

	inline int FileNo(FILE* file) { return fileno(file); }
	inline int IsATTY(int fd) { return isatty(fd); }
	inline int Stat(const char* path, StatStruct* buf) { return stat(path, buf); }
	inline int StrCaseCmp(const char* s1, const char* s2) {
		return strcasecmp(s1, s2);
	}
	inline char* StrDup(const char* src) { return strdup(src); }
	inline int RmDir(const char* dir) { return rmdir(dir); }
	inline bool IsDir(const StatStruct& st) { return S_ISDIR(st.st_mode); }

#endif  // OS_WINDOWS

	// Functions deprecated by MSVC 8.0.

#ifdef _MSC_VER
	// Temporarily disable warning 4996 (deprecated function).
#	pragma warning(push)
#	pragma warning(disable:4996)
#endif

	inline const char* StrNCpy(char* dest, const char* src, size_t n) {
		return strncpy(dest, src, n);
	}

	inline FILE* FOpen(const char* path, const char* mode)
	{
		return fopen(path, mode);
	}

	inline int FClose(FILE* fp)
	{ 
		return fclose(fp);
	}

	// ChDir(), FReopen(), FDOpen(), Read(), Write(), Close(), and
	// StrError() aren't needed on Windows CE at this time and thus not
	// defined there.

#if !OS_WINDOWS_MOBILE
	inline int ChDir(const char* dir) { return chdir(dir); }

	inline FILE *FReopen(const char* path, const char* mode, FILE* stream) {
		return freopen(path, mode, stream);
	}
	inline FILE* FDOpen(int fd, const char* mode) { return fdopen(fd, mode); }

	inline int Read(int fd, void* buf, unsigned int count) {
		return static_cast<int>(read(fd, buf, count));
	}
	inline int Write(int fd, const void* buf, unsigned int count) {
		return static_cast<int>(write(fd, buf, count));
	}
	inline int Close(int fd) { return close(fd); }
	inline const char* StrError(int errnum) { return strerror(errnum); }

#endif

	inline const char* GetEnv(const char* name) 
	{
#	if OS_WINDOWS_MOBILE
		// We are on Windows CE, which has no environment variables.
		return NULL;
#	elif defined(__BORLANDC__) || defined(__SunOS_5_8) || defined(__SunOS_5_9)
		// Environment variables which we programmatically clear will be set to the
		// empty string rather than unset (NULL).  Handle that case.
		const char* const env = getenv(name);
		return (env != NULL && env[0] != '\0') ? env : NULL;
#	else
		return getenv(name);
#	endif
	}

#ifdef _MSC_VER
# pragma warning(pop)  // Restores the warning state.
#endif

#if OS_WINDOWS_MOBILE
	// Windows CE has no C library. This implementation provides a reasonable
	// imitation of standard behaviour.
	void Abort();
#else
	inline void Abort() { abort(); }
#endif  // OS_WINDOWS_MOBILE

}	// namespace Posix

}	// namespace Util

#endif