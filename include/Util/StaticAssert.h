// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_STATIC_ASSERT_H
#define UTIL_STATIC_ASSERT_H

#include <Build/UsefulMacros.h>

UTIL_BEGIN

//
// version 1 - libcxx
//
#if !defined(LANG_CPP11)

template <bool> struct static_assert_test;
template <> struct static_assert_test<true> 
{
};

template <unsigned> struct static_assert_check
{
};

#define static_assert(expr, msg) \
    typedef static_assert_check<sizeof(static_assert_test<(expr)>)> \
    CONCAT(__t, __LINE__)

#endif

//
// version 2
//
// declare a tempalte class StaticAssert.
template <bool assertion> 
struct StaticAssert;

// only partial specializate parameter's value is true.
template <> 
struct StaticAssert<true> 
{
    enum { VALUE = 1 };
};

// 当expression为false时，sizeof(StaticAssert<false>) 没有实现(不能实例化)，为不完整类，编译器报错！

#if 0

#define STATIC_ASSERT(expression) (void)Util::StaticAssert<(static_cast<bool>(expression))>::VALUE 

#else

template<int x> 
struct StaticAssertTest
{
};

#define STATIC_ASSERT(expression) \
    typedef Util::StaticAssertTest<sizeof(Util::StaticAssert<(static_cast<bool>(expression))>)> \
    static_assert_typedef

#endif

// The COMPILE_ASSERT macro can be used to verify that a compile time
// expression is true. For example, you could use it to verify the
// size of a static array:
//
//   COMPILE_ASSERT(ARRAYSIZE_UNSAFE(content_type_names) == NUM_NAMES,
//                  content_type_names_incorrect_size);
//
// or to make sure a struct is smaller than a certain size:
//
//   COMPILE_ASSERT(sizeof(foo) < 128, foo_too_large);
//
// The second argument to the macro is the name of the variable. If
// the expression is false, most compilers will issue a warning/error
// containing the name of the variable.

#undef COMPILE_ASSERT

#if __cplusplus >= 201103L

// Under C++11, just use static_assert.
#define COMPILE_ASSERT(expr, msg) static_assert(expr, #msg)

#else

template <bool>
struct CompileAssert {
};

#define COMPILE_ASSERT(expr, msg) \
    typedef Util::CompileAssert<(static_cast<bool>(expr)) > \
    msg[static_cast<bool>(expr) ? 1 : -1] ATTRIBUTE_UNUSED

// Implementation details of COMPILE_ASSERT:
//
// - COMPILE_ASSERT works by defining an array type that has -1
//   elements (and thus is invalid) when the expression is false.
//
// - The simpler definition
//
//    #define COMPILE_ASSERT(expr, msg) typedef char msg[(expr) ? 1 : -1]
//
//   does not work, as gcc supports variable-length arrays whose sizes
//   are determined at run-time (this is gcc's extension and not part
//   of the C++ standard).  As a result, gcc fails to reject the
//   following code with the simple definition:
//
//     int foo;
//     COMPILE_ASSERT(foo, msg);    // not supposed to compile as foo is
//                                    // not a compile-time constant.
//
// - By using the type CompileAssert<(bool(expr))>, we ensures that
//   expr is a compile-time constant.  (Template arguments must be
//   determined at compile-time.)
//
// - The outter parentheses in CompileAssert<(bool(expr))> are necessary
//   to work around a bug in gcc 3.4.4 and 4.0.1.  If we had written
//
//     CompileAssert<bool(expr)>
//
//   instead, these compilers will refuse to compile
//
//     COMPILE_ASSERT(5 > 0, some_message);
//
//   (They seem to think the ">" in "5 > 0" marks the end of the
//   template argument list.)
//
// - The array size is (bool(expr) ? 1 : -1), instead of simply
//
//     ((expr) ? 1 : -1).
//
//   This is to avoid running into a bug in MS VC 7.1, which
//   causes ((0.0) ? 1 : -1) to incorrectly evaluate to 1.

#endif

UTIL_END

#endif
