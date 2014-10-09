// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_TYPE_TRAIT_H
#define UTIL_TYPE_TRAIT_H

#include <Config.h>
#include <Build/UsefulMacros.h>
#include <Logging/Logger.h>

//
// forward declaration
//
class ProtocolMessage;
namespace proto2
{
    class Message;
}


THREADING_BEGIN

// IsTrue() silences warnings: "Condition is always true",
// "unreachable code".
inline bool IsTrue(bool condition) 
{
    return condition; 
}

namespace traits_helper
{
typedef char yes_type;
struct no_type
{
    char padding[8];
};
} // namespace traits_helper

//
// Is the provided type a container?
// For now, the implementation only checks if there is a T::iterator typedef
// using SFINAE
//
template<typename T>
struct IsContainer
{
    template<typename C>
    static traits_helper::yes_type test(typename C::iterator*);

    template<typename C>
    static traits_helper::no_type test(...);

    static const bool value = sizeof(test<T>(0)) == sizeof(traits_helper::yes_type);
};

//
// Is the provided type a map?
// For now, the implementation only checks if there is a T::mapped_type typedef
// using SFINAE
//
template<typename T>
struct IsMap
{
    template<typename C>
    static traits_helper::yes_type test(typename C::mapped_type*);

    template<typename C>
    static traits_helper::no_type test(...);

    static const bool value = IsContainer<T>::value && sizeof(test<T>(0)) == sizeof(traits_helper::yes_type);
};

//
// type trait
//
template <class T>
struct TypeTrait
{
  typedef T  type;
  typedef T& pass;
  typedef const T& take;
  typedef T* pointer;
};

template <class T, int N>
struct TypeTrait<T[N]>
{
  typedef T*  type;
  typedef T*& pass;
  typedef const T*& take;
  typedef T** pointer;
};

template <class T>
struct TypeTrait<T&>
{
  typedef T  type;
  typedef T& pass;
  typedef T& take;
  typedef T* pointer;
};

template <class T>
struct TypeTrait<const T&>
{
  typedef const T  type;
  typedef const T& pass;
  typedef const T& take;
  typedef const T* pointer;
};

template<>
struct TypeTrait<void>
{
  typedef void  type;
  typedef void  pass;
  typedef void  take;
  typedef void* pointer;
};


//
// Is `Derived' derive from `Base'
//
template <class Base, class Derived>
struct IsBaseAndDerived        // is_base_and_derived
{
    static traits_helper::yes_type test(typename TypeTrait<Base>::pointer);

    static traits_helper::no_type test(...);

    STATIC_CONSTANT(bool, value = 
        sizeof(test(reinterpret_cast<typename TypeTrait<Derived>::pointer>(0))) == sizeof(traits_helper::yes_type));
};
#if !defined(NO_INCLASS_MEMBER_INITIALIZATION)
    template<class Base, class Derived> const bool IsBaseAndDerived<Base, Derived>::value;
#endif

template <class Base>
struct IsBaseAndDerived<Base, Base>
{
    STATIC_CONSTANT(bool, value = true);
};

#if !defined(NO_INCLASS_MEMBER_INITIALIZATION)
    template<class Base> const bool IsBaseAndDerived<Base, Base>::value;
#endif

//
// helper class
//
namespace traits_helper
{
template<bool bool_value> 
struct bool_constant
{
    STATIC_CONSTANT(bool, value = bool_value);
    //STATIC_CONSTANT(int, ivalue = bool_value ? 1 : 0);
    typedef bool_constant<bool_value> type;
    typedef bool value_type;
    operator bool() const { return this->value; }
};

#if !defined(NO_INCLASS_MEMBER_INITIALIZATION)
    template<bool bool_value> const bool bool_constant<bool_value>::value;
#endif

}

// shorcuts
typedef traits_helper::bool_constant<false> false_type;
typedef traits_helper::bool_constant<true> true_type;

template <typename T>
struct IsPointer : public false_type {};

template <typename T>
struct IsPointer<T*> : public true_type {};

template <typename Iterator>
struct IteratorTraits {
    typedef typename Iterator::value_type value_type;
};

template <typename T>
struct IteratorTraits<T*> 
{
    typedef T value_type;
};

template <typename T>
struct IteratorTraits<const T*>
{
    typedef T value_type;
};


// Removes the reference from a type if it is a reference type,
// otherwise leaves it unchanged.  This is the same as
// tr1::remove_reference, which is not widely available yet.
template <typename T>
struct RemoveReference { typedef T type; };  
template <typename T>
struct RemoveReference<T&> { typedef T type; };  

// A handy wrapper around RemoveReference that works when the argument
// T depends on template parameters.
#define REMOVE_REFERENCE(T) \
    typename Threading::RemoveReference<T>::type

// Removes const from a type if it is a const type, otherwise leaves
// it unchanged.  This is the same as tr1::remove_const, which is not
// widely available yet.
template <typename T>
struct RemoveConst { typedef T type; };  
template <typename T>
struct RemoveConst<const T> { typedef T type; };  

// MSVC 8.0, Sun C++, and IBM XL C++ have a bug which causes the above
// definition to fail to remove the const in 'const int[3]' and 'const
// char[3][4]'.  The following specialization works around the bug.
template <typename T, size_t N>
struct RemoveConst<const T[N]>
{
    typedef typename RemoveConst<T>::type type[N];
};

#if defined(_MSC_VER) && _MSC_VER < 1400
// This is the only specialization that allows VC++ 7.1 to remove const in
// 'const int[3] and 'const int[3][4]'.  However, it causes trouble with GCC
// and thus needs to be conditionally compiled.
template <typename T, size_t N>
struct RemoveConst<T[N]>
{
    typedef typename RemoveConst<T>::type type[N];
};
#endif

// A handy wrapper around RemoveConst that works when the argument
// T depends on template parameters.
#define REMOVE_CONST(T) \
    typename Threading::RemoveConst<T>::type

// Turns const U&, U&, const U, and U all into U.
#define REMOVE_REFERENCE_AND_CONST(T) \
    REMOVE_CONST(REMOVE_REFERENCE(T))

// Adds reference to a type if it is not a reference type,
// otherwise leaves it unchanged.  This is the same as
// tr1::add_reference, which is not widely available yet.
template <typename T>
struct AddReference { typedef T& type; };  
template <typename T>
struct AddReference<T&> { typedef T& type; };  

// A handy wrapper around AddReference that works when the argument T
// depends on template parameters.
#define ADD_REFERENCE(T) \
    typename Threading::AddReference<T>::type

// Adds a reference to const on top of T as necessary.  For example,
// it transforms
//
//   char         ==> const char&
//   const char   ==> const char&
//   char&        ==> const char&
//   const char&  ==> const char&
//
// The argument T must depend on some template parameters.
#define REFERENCE_TO_CONST(T) \
    ADD_REFERENCE(const REMOVE_REFERENCE(T))

// ImplicitlyConvertible<From, To>::value is a compile-time bool
// constant that's true iff type From can be implicitly converted to
// type To.
template <typename From, typename To>
class ImplicitlyConvertible 
{
private:
    // We need the following helper functions only for their types.
    // They have no implementations.

    // MakeFrom() is an expression whose type is From.  We cannot simply
    // use From(), as the type From may not have a public default
    // constructor.
    static typename AddReference<From>::type MakeFrom();

    // These two functions are overloaded.  Given an expression
    // Helper(x), the compiler will pick the first version if x can be
    // implicitly converted to type To; otherwise it will pick the
    // second version.
    //
    // The first version returns a value of size 1, and the second
    // version returns a value of size 2.  Therefore, by checking the
    // size of Helper(x), which can be done at compile time, we can tell
    // which version of Helper() is used, and hence whether x can be
    // implicitly converted to type To.
    static char Helper(To);
    static char (&Helper(...))[2];  

    // We have to put the 'public' section after the 'private' section,
    // or MSVC refuses to compile the code.
public:
    // MSVC warns about implicitly converting from double to int for
    // possible loss of data, so we need to temporarily disable the
    // warning.
#ifdef _MSC_VER
# pragma warning(push)          // Saves the current warning state.
# pragma warning(disable:4244)  // Temporarily disables warning 4244.

    static const bool value =
        sizeof(Helper(ImplicitlyConvertible::MakeFrom())) == 1;
# pragma warning(pop)           // Restores the warning state.
#elif defined(__BORLANDC__)
    // C++Builder cannot use member overload resolution during template
    // instantiation.  The simplest workaround is to use its C++0x type traits
    // functions (C++Builder 2009 and above only).
    static const bool value = __is_convertible(From, To);
#else
    static const bool value =
        sizeof(Helper(ImplicitlyConvertible::MakeFrom())) == 1;
#endif  // _MSV_VER
};
template <typename From, typename To>
const bool ImplicitlyConvertible<From, To>::value;

// IsAProtocolMessage<T>::value is a compile-time bool constant that's
// true iff T is type ProtocolMessage, proto2::Message, or a subclass
// of those.
template <typename T>
struct IsAProtocolMessage
    : public traits_helper::bool_constant<
    ImplicitlyConvertible<const T*, const ::ProtocolMessage*>::value ||
    ImplicitlyConvertible<const T*, const ::proto2::Message*>::value> 
{
};

// EnableIf<condition>::type is void when 'Cond' is true, and
// undefined when 'Cond' is false.  To use SFINAE(Substitution Failure Is Not an Error, ∆•≈‰ ß∞‹≤ª «¥ÌŒÛ) to make a function
// overload only apply when a particular expression is true, add
// "typename EnableIf<expression>::type* = 0" as the last parameter.
template<bool> struct EnableIf;
template<> struct EnableIf<true>
{
    typedef void type;
};


// StaticAssertTypeEqHelper is used by StaticAssertTypeEq.
//
// This template is declared, but intentionally undefined.
template <typename T1, typename T2>
struct StaticAssertTypeEqHelper;

template <typename T>
struct StaticAssertTypeEqHelper<T, T> 
{
};

// Compile-time assertion for type equality.
// StaticAssertTypeEq<type1, type2>() compiles iff type1 and type2 are
// the same type.  The value it returns is not interesting.
//
// Instead of making StaticAssertTypeEq a class template, we make it a
// function template that invokes a helper class template.  This
// prevents a user from misusing StaticAssertTypeEq<T1, T2> by
// defining objects of that type.
//
// CAVEAT:
//
// When used inside a method of a class template,
// StaticAssertTypeEq<T1, T2>() is effective ONLY IF the method is
// instantiated.  For example, given:
//
//   template <typename T> class Foo {
//    public:
//     void Bar() { testing::StaticAssertTypeEq<int, T>(); }
//   };
//
// the code:
//
//   void Test1() { Foo<bool> foo; }
//
// will NOT generate a compiler error, as Foo<bool>::Bar() is never
// actually instantiated.  Instead, you need:
//
//   void Test2() { Foo<bool> foo; foo.Bar(); }
//
// to cause a compiler error.
template <typename T1, typename T2>
bool StaticAssertTypeEq()
{
    (void)Threading::StaticAssertTypeEqHelper<T1, T2>();
    return true;
}

//
// Use ImplicitCast as a safe version of static_cast for upcasting in
// the type hierarchy (e.g. casting a Foo* to a SuperclassOfFoo* or a
// const Foo*).  When you use ImplicitCast, the compiler checks that
// the cast is safe.  Such explicit ImplicitCasts are necessary in
// surprisingly many situations where C++ demands an exact type match
// instead of an argument type convertable to a target type.
//
// The From type can be inferred, so the preferred syntax for using
// ImplicitCast is the same as for static_cast etc.:
//
//   ImplicitCast<ToType>(expr)
//
// ImplicitCast would have been part of the C++ standard library,
// but the proposal was submitted too late.  It will probably make
// its way into the language in the future.
//
// This relatively ugly name is intentional. It prevents clashes with
// similar functions users may have (e.g., implicit_cast). The internal
// namespace alone is not enough because the function can be found by ADL.
//
//template<typename To>
//inline To ImplicitCast(To x) 
//{
//    return x; 
//}
template<typename To, typename From>
inline To ImplicitCast(From const &f)
{
    return f;
}

// When you upcast (that is, cast a pointer from type Foo to type
// SuperclassOfFoo), it's fine to use ImplicitCast<>, since upcasts
// always succeed.  When you downcast (that is, cast a pointer from
// type Foo to type SubclassOfFoo), static_cast<> isn't safe, because
// how do you know the pointer is really of type SubclassOfFoo?  It
// could be a bare Foo, or of type DifferentSubclassOfFoo.  Thus,
// when you downcast, you should use this macro.  In debug mode, we
// use dynamic_cast<> to double-check the downcast is legal (we die
// if it's not).  In normal mode, we do the efficient static_cast<>
// instead.  Thus, it's important to test in debug mode to make sure
// the cast is legal!
//    This is the only place in the code we should use dynamic_cast<>.
// In particular, you SHOULDN'T be using dynamic_cast<> in order to
// do RTTI (eg code like this:
//    if (dynamic_cast<Subclass1>(foo)) HandleASubclass1Object(foo);
//    if (dynamic_cast<Subclass2>(foo)) HandleASubclass2Object(foo);
// You should design the code some other way not to need this.
//
// This relatively ugly name is intentional. It prevents clashes with
// similar functions users may have (e.g., down_cast). The internal
// namespace alone is not enough because the function can be found by ADL.
template<typename To, typename From>    // use like this: DownCast<T*>(foo);
inline To DownCast(From* f)                // so we only accept pointers
{  
    // Ensures that To is a sub-type of From *.  This test is here only
    // for compile-time type checking, and has no overhead in an
    // optimized build at run-time, as it will be optimized away
    // completely.
    if (false) 
    {
        const To to = NULL;
        ImplicitCast<From*>(to);
    }

#if HAS_RTTI
    // RTTI: debug mode only!
    CHECK_SUCCESS(f == NULL || dynamic_cast<To>(f) != NULL);
#endif
    return static_cast<To>(f);
}

// Downcasts the pointer of type Base to Derived.
// Derived must be a subclass of Base. The parameter MUST
// point to a class of type Derived, not any subclass of it.
// When RTTI is available, the function performs a runtime
// check to enforce this.
template <class Derived, class Base>
Derived* CheckedDowncastToActualType(Base* base) 
{
#if HAS_RTTI
    CHECK_SUCCESS(typeid(*base) == typeid(Derived));
    return dynamic_cast<Derived*>(base);  
#else
    return static_cast<Derived*>(base);  // Poor man's downcast.
#endif
}


#ifndef BOOL_C_BASE
#   define BOOL_C_BASE(bool_value) : public traits_helper::bool_constant<bool_value>
#endif

#define BOOL_TRAIT_VALUE_DECL(bool_value)                  \
    typedef traits_helper::bool_constant<bool_value> base; \
    using base::value;                        

#define BOOL_TRAIT_DEF(trait, T, bool_value)          \
    template<typename T> struct trait                 \
    BOOL_C_BASE(bool_value)                           \
{                                                     \
public:                                               \
    BOOL_TRAIT_VALUE_DECL(bool_value)                 \
};                                            

#define BOOL_TRAIT_SPEC(trait, sp, bool_value)        \
    template<> struct trait<sp>                       \
    BOOL_C_BASE(bool_value)                           \
{                                                     \
public:                                               \
    BOOL_TRAIT_VALUE_DECL(bool_value)                 \
};                                            

//
//* is a type T a floating-point type described in the standard
//
BOOL_TRAIT_DEF(IsFloat, T, false)
BOOL_TRAIT_SPEC(IsFloat, float, true)
BOOL_TRAIT_SPEC(IsFloat, double, true)
BOOL_TRAIT_SPEC(IsFloat, long double, true)

BOOL_TRAIT_DEF(IsFloatingPoint, T, false)
BOOL_TRAIT_SPEC(IsFloatingPoint, float, true)
BOOL_TRAIT_SPEC(IsFloatingPoint, double, true)
BOOL_TRAIT_SPEC(IsFloatingPoint, long double, true)

//
//* is a type T an integral type described in the standard
//
BOOL_TRAIT_DEF(IsIntegral, T, false)

BOOL_TRAIT_SPEC(IsIntegral, unsigned char, true)
BOOL_TRAIT_SPEC(IsIntegral, unsigned short, true)
BOOL_TRAIT_SPEC(IsIntegral, unsigned int, true)
BOOL_TRAIT_SPEC(IsIntegral, unsigned long, true)

BOOL_TRAIT_SPEC(IsIntegral, signed char, true)
BOOL_TRAIT_SPEC(IsIntegral, signed short, true)
BOOL_TRAIT_SPEC(IsIntegral, signed int, true)
BOOL_TRAIT_SPEC(IsIntegral, signed long, true)
BOOL_TRAIT_SPEC(IsIntegral, Threading::Int64, true)

BOOL_TRAIT_SPEC(IsIntegral, bool, true)
BOOL_TRAIT_SPEC(IsIntegral, char, true)
BOOL_TRAIT_SPEC(IsIntegral, wchar_t, true)

#if (defined(_MSC_VER) && (_MSC_VER < 1300)) 
BOOL_TRAIT_SPEC(IsIntegral, unsigned __int8,true)
BOOL_TRAIT_SPEC(IsIntegral, __int8,true)
BOOL_TRAIT_SPEC(IsIntegral, unsigned __int16,true)
BOOL_TRAIT_SPEC(IsIntegral, __int16,true)
BOOL_TRAIT_SPEC(IsIntegral, unsigned __int32,true)
BOOL_TRAIT_SPEC(IsIntegral, __int32,true)
BOOL_TRAIT_SPEC(IsIntegral, unsigned __int64,true)
BOOL_TRAIT_SPEC(IsIntegral, __int64,true)
#endif

//
// OR
//
template <bool b1, bool b2, bool b3 = false, bool b4 = false, bool b5 = false, bool b6 = false, bool b7 = false>
struct OR;

template <bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7>
struct OR
{
    STATIC_CONSTANT(bool, value = true);
};

template <>
struct OR<false, false, false, false, false, false, false>
{
    STATIC_CONSTANT(bool, value = false);
};

//
// AND
//
template <bool b1, bool b2, bool b3 = true, bool b4 = true, bool b5 = true, bool b6 = true, bool b7 = true>
struct AND;

template <bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7>
struct AND
{
    STATIC_CONSTANT(bool, value = false);
};

template <>
struct AND<true, true, true, true, true, true, true>
{
    STATIC_CONSTANT(bool, value = true);
};

//
// EQ & NE
//
template <int b1, int b2>
struct EQ
{
    STATIC_CONSTANT(bool, value = (b1 == b2));
};
template <int b1, int b2>
struct NE
{
    STATIC_CONSTANT(bool, value = (b1 != b2));
};

#ifndef NO_INCLASS_MEMBER_INITIALIZATION
    template <int b1, int b2> bool const EQ<b1,b2>::value;
    template <int b1, int b2> bool const NE<b1,b2>::value;
#endif

//
// NOT
//
template <bool b>
struct NOT
{
    STATIC_CONSTANT(bool, value = true);
};

template <>
struct NOT<true>
{
    STATIC_CONSTANT(bool, value = false);
};

THREADING_END

#endif // UTIL_TYPE_TRAIT_H
