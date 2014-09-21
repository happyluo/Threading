// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_STRING_UTIL_H
#define UTIL_STRING_UTIL_H

//#ifndef UTIL_STRING_FORMATTER_H
//#define UTIL_STRING_FORMATTER_H

#include <string>
//#include <vector>
//#include <algorithm>
#include <Util/Config.h>
#include <Util/StaticAssert.h>
//#include <Util/TypeTraits.h>
#include <Util/NumericalUtil.h>
#include <Util/ErrorToString.h>

UTIL_BEGIN

class UTIL_API String : public std::string
{
public:
	String();

	~String();

	String(const String& other);

	String(const std::string& src);

	String(const String& src, size_type i, size_type n=npos);

	String(const char* src, size_type n);

	String(const char* src);

	String(size_type n, char c);

	template <class In> String(In pbegin, In pend);

	String ToUTF8(const std::string& internalCode = "local") const;

	static String ToUTF8(const String& src, const std::string& internalCode = "local");

	std::wstring ToWString(const std::string& internalCode = "local") const;

	static std::wstring ToWString(const String& src, const std::string& internalCode = "local");

#ifdef _WIN32
	// Creates a UTF-16 wide string from the given ANSI string, allocating
	// memory using new. The caller is responsible for deleting the return
	// value using delete[]. Returns the wide string, or NULL if the
	// input is NULL.
	static LPCWSTR AnsiToUTF16(const char* ansi);

	// Creates an ANSI string from the given wide string, allocating
	// memory using new. The caller is responsible for deleting the return
	// value using delete[]. Returns the ANSI string, or NULL if the
	// input is NULL.
	static const char* UTF16ToAnsi(LPCWSTR utf16_str);
#endif

	//operator std::string&()
	//{
	//	return *this;
	//}

	//operator const std::string&() const
	//{
	//	return const_cast<const std::string&>(
	//		const_cast<Util::String*>(this)->operator std::string&());
	//}

	//
	// Message formatting.
	//

	//
	// Returns fmt as is, but checks for invalid references in the Format string.
	//
	template <class T1>
	static inline
	std::string Compose(const std::string& fmt);

	//
	//! Substitute placeholders in a Format string with the referenced arguments.
	// The template string should be in <tt>qt-Format</tt>, that is
	// <tt>"%1"</tt>, <tt>"%2"</tt>, ..., <tt>"%9"</tt> are used as placeholders
	// and <tt>"%%"</tt> denotes a literal <tt>"%"</tt>.  Substitutions may be
	// reordered.
	// @par Example:
	// @code
	// using std::string;
	// const int percentage = 50;
	// const std::string text = Compose("%1%% done", percentage);
	// @endcode
	// @param fmt A template string in <tt>qt-Format</tt>.
	// @param a1 The argument to substitute for <tt>"%1"</tt>.
	// @return The substituted message string.
	// @throw ConvertError
	//
	// @newin{2,16}
	//
	template <class T1>
	static inline
	std::string Compose(const std::string& fmt, const T1& a1);

	template <class T1, class T2>
	static inline
	std::string Compose(const std::string& fmt, const T1& a1, const T2& a2);

	template <class T1, class T2, class T3>
	static inline
	std::string Compose(const std::string& fmt,
		const T1& a1, const T2& a2, const T3& a3);

	template <class T1, class T2, class T3, class T4>
	static inline
	std::string Compose(const std::string& fmt,
		const T1& a1, const T2& a2, const T3& a3,
		const T4& a4);

	template <class T1, class T2, class T3, class T4, class T5>
	static inline
	std::string Compose(const std::string& fmt,
		const T1& a1, const T2& a2, const T3& a3,
		const T4& a4, const T5& a5);

	template <class T1, class T2, class T3, class T4, class T5, class T6>
	static inline
	std::string Compose(const std::string& fmt,
		const T1& a1, const T2& a2, const T3& a3,
		const T4& a4, const T5& a5, const T6& a6);

	template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
	static inline
	std::string Compose(const std::string& fmt,
		const T1& a1, const T2& a2, const T3& a3,
		const T4& a4, const T5& a5, const T6& a6,
		const T7& a7);

	template <class T1, class T2, class T3, class T4,
	class T5, class T6, class T7, class T8>
	static inline
	std::string Compose(const std::string& fmt,
		const T1& a1, const T2& a2, const T3& a3,
		const T4& a4, const T5& a5, const T6& a6,
		const T7& a7, const T8& a8);

	template <class T1, class T2, class T3, class T4, class T5,
	class T6, class T7, class T8, class T9>
	static inline
	std::string Compose(const std::string& fmt,
		const T1& a1, const T2& a2, const T3& a3,
		const T4& a4, const T5& a5, const T6& a6,
		const T7& a7, const T8& a8, const T9& a9);

	//
	//! Format the argument to its string representation.
	// Applies the arguments in order to an std::wostringstream and returns the
	// resulting string.  I/O manipulators may also be used as arguments.  This
	// greatly simplifies the common task of converting a number to a string, as
	// demonstrated by the example below.  The Format() methods can also be used
	// in conjunction with Compose() to facilitate localization of user-visible
	// messages.
	// @code
	// using std::string;
	// double value = 22.0 / 7.0;
	// std::string text = Format(std::fixed, std::setprecision(2), value);
	// @endcode
	// @note The use of a wide character stream in the implementation of Format()
	// is almost completely transparent.  However, one of the instances where the
	// use of wide streams becomes visible is when the std::setfill() stream
	// manipulator is used.  In order for std::setfill() to work the argument
	// must be of type <tt>wchar_t</tt>.  This can be achieved by using the
	// <tt>L</tt> prefix with a character literal, as shown in the example.
	// @code
	// using std::string;
	// // Insert leading zeroes to fill in at least six digits
	// std::string text = Format(std::setfill(L'0'), std::setw(6), 123);
	// @endcode
	//
	// @param a1 A streamable value or an I/O manipulator.
	// @return The string representation of the argument stream.
	// @throw ConvertError
	//
	// @newin{2,16}
	//
	template <class T1>
	static inline
	std::string Format(const T1& a1);

	template <class T1, class T2>
	static inline
	std::string Format(const T1& a1, const T2& a2);

	template <class T1, class T2, class T3>
	static inline
	std::string Format(const T1& a1, const T2& a2, const T3& a3);

	template <class T1, class T2, class T3, class T4>
	static inline
	std::string Format(const T1& a1, const T2& a2, const T3& a3, const T4& a4);

	template <class T1, class T2, class T3, class T4, class T5>
	static inline
	std::string Format(const T1& a1, const T2& a2, const T3& a3,
		const T4& a4, const T5& a5);

	template <class T1, class T2, class T3, class T4, class T5, class T6>
	static inline
	std::string Format(const T1& a1, const T2& a2, const T3& a3,
		const T4& a4, const T5& a5, const T6& a6);

	template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
	static inline
	std::string Format(const T1& a1, const T2& a2, const T3& a3, const T4& a4,
		const T5& a5, const T6& a6, const T7& a7);

	template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
	static inline
	std::string Format(const T1& a1, const T2& a2, const T3& a3, const T4& a4,
		const T5& a5, const T6& a6, const T7& a7, const T8& a8);

	static std::string ComposeArgv(const std::string& fmt, int argc, const std::string* const* argv);

	//
	// Add escape sequences (like "\n", or "\0xxx") to make a string
	// readable in ASCII.
	//
	static std::string EscapeString(const std::string&, const std::string&);

	//
	// Remove escape sequences added by escapeString. Throws IllegalArgumentException
	// for an invalid input string.
	//
	static std::string UnescapeString(const std::string&, std::string::size_type, std::string::size_type);

	//
	// Split a string using the given delimiters. Considers single and double quotes;
	// returns false for unbalanced quote, true otherwise.
	//
	static bool SplitString(const std::string& str, const std::string& delim, std::vector<std::string>& result, bool keepblank = false);

	//
	// Join a list of strings using the given delimiter. 
	//
	static std::string JoinString(const std::vector<std::string>& values, const std::string& delimiter);

	//
	// Trim white space
	//
	static std::string Trim(const std::string& src);

	//
	// If a single or double quotation mark is found at the start
	// position, then the position of the matching closing quote is
	// returned. If no quotation mark is found at the start position, then
	// 0 is returned. If no matching closing quote is found, then
	// std::string::npos is returned.
	//
	static std::string::size_type CheckQuote(const std::string&, std::string::size_type = 0);

	static std::string::size_type ExistQuote(const std::string&, std::string::size_type = 0);

	//
	// Match `s' against the pattern `pat'. A * in the pattern acts
	// as a wildcard: it matches any non-empty sequence of characters
	// other than a period (`.'). We match by hand here because
	// it's portable across platforms (whereas regex() isn't).
	//
	static bool Match(const std::string& s, const std::string& pat, bool = false);

	//
	// Translating both the two-character sequence #xD #xA and any #xD that is not followed by #xA to 
	// a single #xA character.
	//	LF  (Line feed, '\n', 0x0A, 10 in decimal)  
	//	CR (Carriage return, '\r', 0x0D, 13 in decimal) 
	//
	static std::string TranslatingCR2LF(const std::string& src);

	//static std::string ToHexString(unsigned long n, bool bupper = false);

	template <typename T>
	static inline
	std::string ToHexString(T n, bool bupper = false);

	//static std::string ToOctalString(unsigned long n)

	template <typename T>
	static inline
	std::string ToOctalString(T n);

	//static std::string ToBinaryString(unsigned long n);

	template <typename T>
	static inline
	std::string ToBinaryString(T n);

	//
	// Functions to convert to lower/upper case. These functions accept
	// UTF8 string/characters but ignore non ASCII characters. Unlike, the
	// C methods, these methods are not local dependent.
	//
	static std::string ToLower(const std::string&);
	static std::string ToUpper(const std::string&);

	static unsigned long Hash(const std::string&);

	//
	// Remove all whitespace from a string
	//
	static std::string RemoveWhitespace(const std::string&);

	//////////////////////////////////////////////////////////////////////////
	/// string & data convert
	//
	// Portable strtoll/_strtoi64
	//
	static Util::Int64 ToInt64(const char* s, char** endptr, int base);

	//
	// ToInt64 converts a string into a signed 64-bit integer.
	// It's a simple wrapper around ToInt64.
	//
	// Semantics:
	//
	// - Ignore leading whitespace
	//
	// - If the string starts with '0', parse as octal
	//
	// - If the string starts with "0x" or "0X", parse as hexadecimal
	//
	// - Otherwise, parse as decimal
	//
	// - return value == true indicates a successful conversion and result contains the converted value
	// - return value == false indicates an unsuccessful conversion:
	//      - result == 0 indicates that no digits were available for conversion
	//      - result == "Int64 Min" or result == "Int64 Max" indicate underflow or overflow.
	//
	static  bool ToInt64(const std::string& s,  Util::Int64& result);

	static unsigned long ToULong(const std::string& strval, size_t* endindex = 0, unsigned int base = 10);

	static long ToLong(const std::string& strval, size_t* endindex = 0, unsigned int base = 10);

	static double ToDouble(const std::string& strval, size_t* endindex = 0, int precision = 6);

	template <typename T>
	static inline
	T ToData(const std::string& strval, size_t* endindex = 0, unsigned int base = 10);

	static std::string ToString(unsigned long n);

	static std::string ToString(long n);

	static std::string ToString(double data, int precision = 6);

	static std::string ToExpString(double data, int precision = 6);

	template <typename T>
	static inline
	std::string ToString(const T& val);

	//
	// Determines if a string is a number of not.
	//
	static  bool IsNumber(const std::string& s, bool* isdecimal);

	//
	// Skip leading none digit character, get the first number in string.
	//
	static  int GetIntInString(const char* s, char** endptr, int base);

	static const Util::Byte* FindStringInBuffer(Util::Byte* pBuff, size_t iBuffSize, const std::string& strSearch);

	//
	// Swap lhs and rhs's value
	//
	static inline void Swap(unsigned char* lhs, unsigned char* rhs);

	//
	// Reverse elements in [begin, end)
	//
	static inline void ReverseBuffer(unsigned char* begin, unsigned char* end);

	static std::string BytesToString(const Util::Byte* src, size_t size);
	static std::string BytesToString(const Util::ByteSeq& bytes);
	static Util::ByteSeq StringToBytes(const std::string&);

	//
	// Hex-dump at most 16 bytes starting at offset from a memory area of size
	// bytes.  Return the number of bytes actually dumped.
	//
	static size_t HexDumpLine(const void* ptr, size_t offset, size_t size, std::string& line, size_t linelength = 16);

	template <typename OutIt>
	static inline
	void HexDump(const void* ptr, size_t size, OutIt out, size_t linelength/* = 16*/);

	static std::string HexDump(const void* ptr, size_t size, size_t linelength = 16);

	static std::string HexStringToBuffer(const std::string &hexString, std::string &buffer, const std::string& delimiter = ",");

	static size_t BinDumpLine(const void* ptr, size_t offset, size_t size, std::string& line, size_t linelength = 8);

	template <typename OutIt>
	static inline
	void BinDump(const void* ptr, size_t size, OutIt out, size_t linelength/* = 8*/);

	static std::string BinDump(const void* ptr, size_t size, size_t linelength = 8);

	// If *pstr starts with the given prefix, modifies *pstr to be right
	// past the prefix and returns true; otherwise leaves *pstr unchanged
	// and returns false.  None of pstr, *pstr, and prefix can be NULL.
	static bool SkipPrefix(const char* prefix, const char** pstr);

	//! @}
	//template <class In, class ValueType = typename std::iterator_traits<In>::value_type> struct SequenceToString;

	//The Tru64 compiler needs these partial specializations to be declared here,
	//as well as defined later. That's probably correct. murrayc.
	//template <class In> struct SequenceToString<In, char>;

	template <class T> class Stringify;
	class FormatStream;
};

template <class In>
String::String(In pbegin, In pend) : std::string(pbegin, pend)
{
}

//////////////////////////////////////////////////////////////////////////
// String::SequenceToString
//template <class In, class ValueType>
//struct String::SequenceToString
//{
//};
//
//template <class In>
//struct String::SequenceToString<In, char> : public std::string
//{
//	SequenceToString(In pbegin, In pend) : std::string(pbegin, pend)
//	{
//	}
//};

class UTIL_API String::FormatStream
{
public:
	FormatStream();
	~FormatStream();

	template <class T> inline void stream(const T& value);

	inline void stream(const char* value);

	//This overload exists to avoid the templated stream() being called for non-const char*.
	inline void stream(char* value);

	std::string to_string() const;

private:
#ifdef HAS_STD_WSTREAM
	typedef std::wostringstream StreamType;
#else
	typedef std::ostringstream StreamType;
#endif
	StreamType m_stream;

	// noncopyable
	FormatStream(const FormatStream&);
	FormatStream& operator=(const FormatStream&);
};


#ifdef HAS_STD_WSTREAM

//
// Wide stream input operator.
//
std::wistream& operator >>(std::wistream& is, Util::String& strret);

//
// Wide stream output operator.
//
std::wostream& operator <<(std::wostream& os, const Util::String& strsrc);

#endif // HAS_STD_WSTREAM

//
// std::string::FormatStream 
//
template <class T> inline
void String::FormatStream::stream(const T& value)
{
	m_stream << value;
}

inline
void String::FormatStream::stream(const char* value)
{
	m_stream << Util::String(value);
}

inline
void String::FormatStream::stream(char* value)
{
	m_stream << Util::String(value);
}

//
// An inner class used by String.
//
template <class T>
class String::Stringify
{
public:
	explicit inline Stringify(const T& arg) : m_string(String::Format(arg))
	{
	}

	//TODO: Why is this here? See the template specialization:
	explicit inline Stringify(const char* arg) : m_string(arg) 
	{
	}

	inline const std::string* ptr() const 
	{
		return &m_string; 
	}

private:
	std::string m_string;

	// noncopyable
	Stringify(const String::Stringify<T>&);
	Stringify<T>& operator=(const String::Stringify<T>&);
};

/// A template specialization for Stringify<string>:
template <>
class String::Stringify<std::string>
{
public:
	explicit inline Stringify(const String& arg) : m_string(arg) 
	{
	}

	inline const String* ptr() const
	{
		return &m_string; 
	}

private:
	const String& m_string;

	// noncopyable
	Stringify(const String::Stringify<String>&);
	Stringify<String>& operator=(const String::Stringify<String>&);
};

//
// A template specialization for Stringify<const char*>,
// because the regular template has ambiguous constructor overloads for char*.
//
template <>
class String::Stringify<const char*>
{
public:
	explicit inline Stringify(const char* arg) : m_string(arg)
	{
	}

	inline const std::string* ptr() const 
	{
		return &m_string; 
	}

private:
	const std::string m_string;

	// noncopyable
	Stringify(const String::Stringify<const char*>&);
	Stringify<String>& operator =(const String::Stringify<const char*>&);
};

//
// A template specialization for Stringify<char[N]> (for String literals),
// because the regular template has ambiguous constructor overloads for char*.
//
template <std::size_t N>
class String::Stringify<char[N]>
{
public:
	explicit inline Stringify(const char arg[N]) : m_string(arg)
	{
	}

	inline const String* ptr() const 
	{
		return &m_string; 
	}

private:
	const String m_string;

	// noncopyable
	Stringify(const String::Stringify<char[N]>&);
	Stringify<String>& operator=(const String::Stringify<char[N]>&);
};

//////////////////////////////////////////////////////////////////////////
/// Compose
template <class T1>
inline // static
std::string String::Compose(const std::string& fmt)
{
	return ComposeArgv(fmt, 0, 0);
}

template <class T1>
inline // static
std::string String::Compose(const std::string& fmt, const T1& a1)
{
	const Stringify<T1> s1(a1);

	const std::string *const argv[] = { s1.ptr() };
	return ComposeArgv(fmt, ARRAYSIZE_UNSAFE(argv), argv);
}

template <class T1, class T2>
inline // static
std::string String::Compose(const std::string& fmt, const T1& a1, const T2& a2)
{
	const Stringify<T1> s1(a1);
	const Stringify<T2> s2(a2);

	const std::string *const argv[] = { s1.ptr(), s2.ptr() };
	return ComposeArgv(fmt, ARRAYSIZE_UNSAFE(argv), argv);
}

template <class T1, class T2, class T3>
inline // static
std::string String::Compose(const std::string& fmt,
							const T1& a1, const T2& a2, const T3& a3)
{
	const Stringify<T1> s1(a1);
	const Stringify<T2> s2(a2);
	const Stringify<T3> s3(a3);

	const std::string *const argv[] = { s1.ptr(), s2.ptr(), s3.ptr() };
	return ComposeArgv(fmt, ARRAYSIZE_UNSAFE(argv), argv);
}

template <class T1, class T2, class T3, class T4>
inline // static
std::string String::Compose(const std::string& fmt,
							const T1& a1, const T2& a2, const T3& a3, const T4& a4)
{
	const Stringify<T1> s1(a1);
	const Stringify<T2> s2(a2);
	const Stringify<T3> s3(a3);
	const Stringify<T4> s4(a4);

	const std::string *const argv[] = { s1.ptr(), s2.ptr(), s3.ptr(), s4.ptr() };
	return ComposeArgv(fmt, ARRAYSIZE_UNSAFE(argv), argv);
}

template <class T1, class T2, class T3, class T4, class T5>
inline // static
std::string String::Compose(const std::string& fmt,
							const T1& a1, const T2& a2, const T3& a3,
							const T4& a4, const T5& a5)
{
	const Stringify<T1> s1(a1);
	const Stringify<T2> s2(a2);
	const Stringify<T3> s3(a3);
	const Stringify<T4> s4(a4);
	const Stringify<T5> s5(a5);

	const std::string *const argv[] = { s1.ptr(), s2.ptr(), s3.ptr(), s4.ptr(), s5.ptr() };
	return ComposeArgv(fmt, ARRAYSIZE_UNSAFE(argv), argv);
}

template <class T1, class T2, class T3, class T4, class T5, class T6>
inline // static
std::string String::Compose(const std::string& fmt,
							const T1& a1, const T2& a2, const T3& a3,
							const T4& a4, const T5& a5, const T6& a6)
{
	const Stringify<T1> s1(a1);
	const Stringify<T2> s2(a2);
	const Stringify<T3> s3(a3);
	const Stringify<T4> s4(a4);
	const Stringify<T5> s5(a5);
	const Stringify<T6> s6(a6);

	const std::string *const argv[] = { s1.ptr(), s2.ptr(), s3.ptr(), s4.ptr(),
		s5.ptr(), s6.ptr() };
	return ComposeArgv(fmt, ARRAYSIZE_UNSAFE(argv), argv);
}

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
inline // static
std::string String::Compose(const std::string& fmt,
							const T1& a1, const T2& a2, const T3& a3,
							const T4& a4, const T5& a5, const T6& a6, const T7& a7)
{
	const Stringify<T1> s1(a1);
	const Stringify<T2> s2(a2);
	const Stringify<T3> s3(a3);
	const Stringify<T4> s4(a4);
	const Stringify<T5> s5(a5);
	const Stringify<T6> s6(a6);
	const Stringify<T7> s7(a7);

	const std::string *const argv[] = { s1.ptr(), s2.ptr(), s3.ptr(), s4.ptr(),
		s5.ptr(), s6.ptr(), s7.ptr() };
	return ComposeArgv(fmt, ARRAYSIZE_UNSAFE(argv), argv);
}

template <class T1, class T2, class T3, class T4,
class T5, class T6, class T7, class T8>
inline // static
std::string String::Compose(const std::string& fmt,
							const T1& a1, const T2& a2, const T3& a3,
							const T4& a4, const T5& a5, const T6& a6,
							const T7& a7, const T8& a8)
{
	const Stringify<T1> s1(a1);
	const Stringify<T2> s2(a2);
	const Stringify<T3> s3(a3);
	const Stringify<T4> s4(a4);
	const Stringify<T5> s5(a5);
	const Stringify<T6> s6(a6);
	const Stringify<T7> s7(a7);
	const Stringify<T8> s8(a8);

	const std::string *const argv[] = { s1.ptr(), s2.ptr(), s3.ptr(), s4.ptr(),
		s5.ptr(), s6.ptr(), s7.ptr(), s8.ptr() };
	return ComposeArgv(fmt, ARRAYSIZE_UNSAFE(argv), argv);
}

template <class T1, class T2, class T3, class T4, class T5,
class T6, class T7, class T8, class T9>
inline // static
std::string String::Compose(const std::string& fmt,
							const T1& a1, const T2& a2, const T3& a3,
							const T4& a4, const T5& a5, const T6& a6,
							const T7& a7, const T8& a8, const T9& a9)
{
	const Stringify<T1> s1(a1);
	const Stringify<T2> s2(a2);
	const Stringify<T3> s3(a3);
	const Stringify<T4> s4(a4);
	const Stringify<T5> s5(a5);
	const Stringify<T6> s6(a6);
	const Stringify<T7> s7(a7);
	const Stringify<T8> s8(a8);
	const Stringify<T9> s9(a9);

	const std::string *const argv[] = { s1.ptr(), s2.ptr(), s3.ptr(), s4.ptr(),
		s5.ptr(), s6.ptr(), s7.ptr(), s8.ptr(), s9.ptr() };
	return ComposeArgv(fmt, ARRAYSIZE_UNSAFE(argv), argv);
}

//////////////////////////////////////////////////////////////////////////
/// Format
template <class T1>
inline // static
std::string String::Format(const T1& a1)
{
	FormatStream buf;
	buf.stream(a1);
	return buf.to_string();
}

template <class T1, class T2>
inline // static
std::string String::Format(const T1& a1, const T2& a2)
{
	FormatStream buf;
	buf.stream(a1);
	buf.stream(a2);
	return buf.to_string();
}

template <class T1, class T2, class T3>
inline // static
std::string String::Format(const T1& a1, const T2& a2, const T3& a3)
{
	FormatStream buf;
	buf.stream(a1);
	buf.stream(a2);
	buf.stream(a3);
	return buf.to_string();
}

template <class T1, class T2, class T3, class T4>
inline // static
std::string String::Format(const T1& a1, const T2& a2, const T3& a3, const T4& a4)
{
	FormatStream buf;
	buf.stream(a1);
	buf.stream(a2);
	buf.stream(a3);
	buf.stream(a4);
	return buf.to_string();
}

template <class T1, class T2, class T3, class T4, class T5>
inline // static
std::string String::Format(const T1& a1, const T2& a2, const T3& a3,
						   const T4& a4, const T5& a5)
{
	FormatStream buf;
	buf.stream(a1);
	buf.stream(a2);
	buf.stream(a3);
	buf.stream(a4);
	buf.stream(a5);
	return buf.to_string();
}

template <class T1, class T2, class T3, class T4, class T5, class T6>
inline // static
std::string String::Format(const T1& a1, const T2& a2, const T3& a3,
						   const T4& a4, const T5& a5, const T6& a6)
{
	FormatStream buf;
	buf.stream(a1);
	buf.stream(a2);
	buf.stream(a3);
	buf.stream(a4);
	buf.stream(a5);
	buf.stream(a6);
	return buf.to_string();
}

template <class T1, class T2, class T3, class T4,
class T5, class T6, class T7>
inline // static
std::string String::Format(const T1& a1, const T2& a2, const T3& a3, const T4& a4,
						   const T5& a5, const T6& a6, const T7& a7)
{
	FormatStream buf;
	buf.stream(a1);
	buf.stream(a2);
	buf.stream(a3);
	buf.stream(a4);
	buf.stream(a5);
	buf.stream(a6);
	buf.stream(a7);
	return buf.to_string();
}

template <class T1, class T2, class T3, class T4,
class T5, class T6, class T7, class T8>
inline // static
std::string String::Format(const T1& a1, const T2& a2, const T3& a3, const T4& a4,
						   const T5& a5, const T6& a6, const T7& a7, const T8& a8)
{
	FormatStream buf;
	buf.stream(a1);
	buf.stream(a2);
	buf.stream(a3);
	buf.stream(a4);
	buf.stream(a5);
	buf.stream(a6);
	buf.stream(a7);
	buf.stream(a8);
	return buf.to_string();
}

//////////////////////////////////////////////////////////////////////////
// data to string conversion
template <typename T>
inline // static
std::string String::ToHexString(T n, bool bupper/* = false*/)
{
	std::string s;
	size_t size = sizeof(T) * 2;
	s.resize(size);
	std::string::size_type charPos = size;

	const int radix = 1 << 4;
	int mask = radix - 1;
	char base = bupper ? 'A' : 'a';

	do
	{
		int d = n & mask;
		s[--charPos] = d < 10 ? '0' + d : base + (d - 10);
		n >>= 4;
	}while (0 != n);

	return std::string(s, charPos, (size - charPos));
}

template <typename T>
inline // static
std::string String::ToOctalString(T n)
{
	std::string s;
	size_t size = sizeof(T) * 8;
	s.resize(size);
	std::string::size_type charPos = size;

	const int radix = 1 << 3;
	int mask = radix - 1;

	do
	{
		s[--charPos] = '0' + static_cast<int>(n & mask);
		n >>= 3;
	}while (0 != n);

	return std::string(s, charPos, (size - charPos));
}

template <typename T>
inline // static
std::string String::ToBinaryString(T n)
{
	std::string s;
	size_t size = sizeof(T) * 8;
	s.resize(size);
	std::string::size_type charPos = size;

	do
	{
		s[--charPos] = (n & 1) + '0';
		n >>= 1;
	}while (0 != n);

	return std::string(s, charPos, (size - charPos));
}

template <typename T>
inline // static
T String::ToData(const std::string& strval, size_t* endindex/* = 0*/, unsigned int base/* = 10*/)
{
	//STATIC_ASSERT(Util::IsIntegral<T>::value || Util::IsFloat<T>::value);

	//
	// Assume nothing will be there to convert for now
	//
	if (endindex)
	{
		*endindex = 0;
	}

	std::string::const_pointer iter = strval.c_str();

	//
	// Skip leading whitespace
	//
	while (*iter && isspace(static_cast<unsigned char>(*iter)))
	{
		++iter;
	}

	//
	// Check for sign
	//
	int sign = 1;
	if ('+' == *iter)
	{
		++iter;
	}
	else if ('-' == *iter)
	{
		sign = -1;
		++iter;
	}

	//
	// Check that base is valid
	//
	if (base == 0)
	{
		if ('0' == *iter)
		{
			base = 8;
			++iter;

			//
			// We have at least this zero
			//
			if (endindex)
			{
				*endindex = iter - strval.c_str();
			}

			if ('x' == *iter || 'X' == *iter)
			{
				base = 16;
				++iter;
			}
		}
		else
		{
			base = 10;
		}
	}
	else if (base < 2 || base > 16)
	{
		errno = EINVAL;
		return 0;
	}

	//
	// Check that we have something left to parse
	//
	if (*iter == '\0')
	{
		//
		// We did not read any new digit so we don't update endptr
		//
		return 0;
	}

	int exp = 0;
	double result = 0;
	unsigned value;
	while (*iter && isxdigit(*iter) 
		&& (value = isdigit(*iter) ? *iter - '0' : toupper(*iter) - 'A' + 10) < base)
	{
		result = result * base + value;
		++iter;
	}

	if (10 == base)
	{
		if ('.' == *iter)
		{
			++iter;
			for (int i = -1; isdigit(*iter); --i, ++iter)
			{
				result = result *base + (*iter - '0');
				exp = i;
			}
		}

		if ('E' == toupper(*iter))
		{
			++iter;
			std::string strexp;
			strexp.reserve(strval.size() - (iter - strval.c_str()));
			if ('+' == *iter || '-' == *iter)
			{
				strexp += *iter;
				++iter;
			}

			while (isdigit(*iter))
			{
				strexp += *iter;
				++iter;
			}

			exp += ToLong(strexp, 0, 10);
		}
	}	

	if (endindex)
	{
		*endindex = iter - strval.c_str();
	}

	//if (typeid(float) == typeid(T) || typeid(double) == typeid(T))
	if (UtilInternal::IsFloat<T>::value)
	{
		result *= ::pow(10.0, exp);
		return static_cast<T>(sign * result);
	}
	else if (UtilInternal::IsIntegral<T>::value)
	{
		result = result * ::pow(1.0 * base, exp) + 0.5;

		if (result <= Util::NumericLimits::Max<T>() && result >= Util::NumericLimits::Min<T>())
		{
			Util::Int64 ret64 = static_cast<Util::Int64>(sign * result);
			return static_cast<T>(ret64);
		}
		else // overflow
		{		
			errno = ERANGE;
			return sign == -1 ? Util::NumericLimits::Min<T>() : Util::NumericLimits::Max<T>();
		}
	}
}

template <typename T>
inline // static
std::string String::ToString(const T& val)
{
	return Format(val);
}

//
// Swap lhs and rhs's value
//
inline // static
void String::Swap(unsigned char* lhs, unsigned char* rhs)
{
	//return std::swap(*lhs, *rhs);

	unsigned char tmp = *lhs;
	*lhs = *rhs;
	*rhs = tmp;
}

//
// Reverse elements in [begin, end)
//
inline // static
void String::ReverseBuffer(unsigned char* begin, unsigned char* end)
{
	//return std::reverse(begin, end);
#if 0
	for (; begin != end && begin != --end; ++begin)
	{
		//Swap(begin, end);
		std::swap(*begin, *end);
	}

#else

	while (begin < --end)
	{
		//Swap(begin++, end);
		std::swap(*begin, *end);
	}
#endif
}

template <typename OutIt>
inline // static
void String::HexDump(const void* ptr, size_t size, OutIt out, size_t linelength/* = 16*/)
{
	size_t offset = 0;
	std::string line;
	while (offset < size) 
	{
		offset += HexDumpLine(ptr, offset, size, line, linelength);
		*out++ = line.c_str();
		//*out++ = line;
	}
}

template <typename OutIt>
inline // static
void String::BinDump(const void* ptr, size_t size, OutIt out, size_t linelength/* = 8*/)
{
	size_t offset = 0;
	std::string line;
	while (offset < size) 
	{
		offset += BinDumpLine(ptr, offset, size, line, linelength);
		*out++ = line.c_str();
		//*out++ = line;
	}
}

//
// Global Format utility
// 
UTIL_API std::string Format( const char* format, ...);

// Formats an int value as "%02d".
UTIL_API std::string FormatIntWidth2(int value);

// Formats an int value as "%X".
UTIL_API std::string FormatHexInt(int value);

// Formats a byte as "%02X".
UTIL_API std::string FormatByte(unsigned char value);

// Converts the buffer in a stringstream to an std::string, converting NUL
// bytes to "\\0" along the way.
UTIL_API std::string StringStreamToString(::std::stringstream* ss);

// Converts a Unicode code point to a narrow string in UTF-8 encoding.
// code_point parameter is of type unsigned int because wchar_t may not be
// wide enough to contain a code point.
// If the code_point is not a valid Unicode code point
// (i.e. outside of Unicode range U+0 to U+10FFFF) it will be converted
// to "(Invalid Unicode 0xXXXXXXXX)".
UTIL_API std::string CodePointToUtf8(unsigned int code_point);

// Converts a wide string to a narrow string in UTF-8 encoding.
// The wide string is assumed to have the following encoding:
//   UTF-16 if sizeof(wchar_t) == 2 (on Windows, Cygwin, Symbian OS)
//   UTF-32 if sizeof(wchar_t) == 4 (on Linux)
// Parameter str points to a null-terminated wide string.
// Parameter num_chars may additionally limit the number
// of wchar_t characters processed. -1 is used when the entire string
// should be processed.
// If the string contains code points that are not valid Unicode code points
// (i.e. outside of Unicode range U+0 to U+10FFFF) they will be output
// as '(Invalid Unicode 0xXXXXXXXX)'. If the string is in UTF16 encoding
// and contains invalid UTF-16 surrogate pairs, values in those pairs
// will be encoded as individual Unicode characters from Basic Normal Plane.
UTIL_API std::string WideStringToUtf8(const wchar_t* str, int num_chars);

// Converts a wide C string to an std::string using the UTF-8 encoding.
// NULL will be converted to "(null)".
UTIL_API std::string ShowWideCString(const wchar_t * wide_c_str);

// Compares two wide C strings.  Returns true iff they have the same
// content.
//
// Unlike wcscmp(), this function can handle NULL argument(s).  A NULL
// C string is considered different to any non-NULL C string,
// including the empty string.
UTIL_API bool WideCStringEquals(const wchar_t * lhs, const wchar_t * rhs);

// Compares two C strings.  Returns true iff they have the same content.
//
// Unlike strcmp(), this function can handle NULL argument(s).  A NULL
// C string is considered different to any non-NULL C string,
// including the empty string.
UTIL_API bool CStringEquals(const char * lhs, const char * rhs);

// Compares two C strings, ignoring case.  Returns true iff they have
// the same content.
//
// Unlike strcasecmp(), this function can handle NULL argument(s).  A
// NULL C string is considered different to any non-NULL C string,
// including the empty string.
UTIL_API bool CaseInsensitiveCStringEquals(const char * lhs, const char * rhs);

// Compares two wide C strings, ignoring case.  Returns true iff they
// have the same content.
//
// Unlike wcscasecmp(), this function can handle NULL argument(s).
// A NULL C string is considered different to any non-NULL wide C string,
// including the empty string.
// NB: The implementations on different platforms slightly differ.
// On windows, this method uses _wcsicmp which compares according to LC_CTYPE
// environment variable. On GNU platform this method uses wcscasecmp
// which compares according to LC_CTYPE category of the current locale.
// On MacOS X, it uses towlower, which also uses LC_CTYPE category of the
// current locale.
UTIL_API bool CaseInsensitiveWideCStringEquals(const wchar_t* lhs, const wchar_t* rhs);

// Returns true iff str ends with the given suffix, ignoring case.
// Any string is considered to end with an empty suffix.
UTIL_API bool EndsWithCaseInsensitive(const std::string& str, const std::string& suffix);

UTIL_API std::string Double2String(double value, int precision);
UTIL_API double String2Double(const std::string& str);


//
// Utilities for char.
//
// isspace(int ch) and friends accept an unsigned char or EOF.  char
// may be signed, depending on the compiler (or compiler flags).
// Therefore we need to cast a char to unsigned char before calling
// isspace(), etc.

UTIL_API bool IsAlpha(char);
UTIL_API bool IsDigit(char);

//inline bool IsAlpha(char ch) 
//{
//	return 0 != isalpha(static_cast<unsigned char>(ch));
//}

inline bool IsAlNum(char ch)
{
	return 0 != isalnum(static_cast<unsigned char>(ch));
}

//inline bool IsDigit(char ch) 
//{
//	return 0 != isdigit(static_cast<unsigned char>(ch));
//}

inline bool IsLower(char ch) 
{
	return 0 != islower(static_cast<unsigned char>(ch));
}

inline bool IsSpace(char ch)
{
	return 0 != isspace(static_cast<unsigned char>(ch));
}

inline bool IsUpper(char ch)
{
	return 0 != isupper(static_cast<unsigned char>(ch));
}

inline bool IsXDigit(char ch)
{
	return 0 != isxdigit(static_cast<unsigned char>(ch));
}

inline bool IsXDigit(wchar_t ch)
{
	const unsigned char low_byte = static_cast<unsigned char>(ch);
	return ch == low_byte && 0 != isxdigit(low_byte);
}

inline char ToLower(char ch)
{
	return static_cast<char>(tolower(static_cast<unsigned char>(ch)));
}

inline char ToUpper(char ch) 
{
	return static_cast<char>(toupper(static_cast<unsigned char>(ch)));
}

inline int DigitValue(char ch)
{
	unsigned char uc(static_cast<unsigned char>(ch));
	return isdigit(uc) ? uc - '0' : -1;
}

inline int XDigitValue(char ch)
{
	unsigned char uc(static_cast<unsigned char>(ch));
	return isxdigit(uc)
		? (isdigit(uc) ? uc - '0' : toupper(uc) - 'A' + 10) 
		: -1;
}

#if 0
// ======================================================================
//
// extern functions
//

// ----------------------------------------------------------------------
// ascii_isalnum()
//    Check if an ASCII character is alphanumeric.  We can't use ctype's
//    isalnum() because it is affected by locale.  This function is applied
//    to identifiers in the protocol buffer language, not to natural-language
//    strings, so locale should not be taken into account.
// ascii_isdigit()
//    Like above, but only accepts digits.
// ----------------------------------------------------------------------
extern UTIL_API inline bool ascii_isalnum(char c) 
{
	return ('a' <= c && c <= 'z')
		|| ('A' <= c && c <= 'Z') 
		|| ('0' <= c && c <= '9');
}

extern UTIL_API inline bool ascii_isdigit(char c)
{
	return ('0' <= c && c <= '9');
}

// ----------------------------------------------------------------------
// HasPrefixString()
//    Check if a string begins with a given prefix.
// StripPrefixString()
//    Given a string and a putative prefix, returns the string minus the
//    prefix string if the prefix matches, otherwise the original
//    string.
// ----------------------------------------------------------------------
extern UTIL_API inline bool HasPrefixString(const std::string& str, const std::string& prefix)
{
	return str.size() >= prefix.size() &&
		str.compare(0, prefix.size(), prefix) == 0;
}

extern UTIL_API inline std::string StripPrefixString(const std::string& str, const std::string& prefix)
{
	if (HasPrefixString(str, prefix))
	{
		return str.substr(prefix.size());
	} 
	else
	{
		return str;
	}
}

// ----------------------------------------------------------------------
// HasSuffixString()
//    Return true if str ends in suffix.
// StripSuffixString()
//    Given a string and a putative suffix, returns the string minus the
//    suffix string if the suffix matches, otherwise the original
//    string.
// ----------------------------------------------------------------------
extern UTIL_API inline bool HasSuffixString(const std::string& str, const std::string& suffix) 
{
	return str.size() >= suffix.size() &&
		str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

extern UTIL_API inline std::string StripSuffixString(const std::string& str, const std::string& suffix) 
{
	if (HasSuffixString(str, suffix)) 
	{
		return str.substr(0, str.size() - suffix.size());
	} 
	else
	{
		return str;
	}
}

// ----------------------------------------------------------------------
// StripString
//    Replaces any occurrence of the character 'remove' (or the characters
//    in 'remove') with the character 'replacewith'.
//    Good for keeping html characters or protocol characters (\t) out
//    of places where they might cause a problem.
// ----------------------------------------------------------------------
extern UTIL_API void StripString(std::string* s, const char* remove, char replacewith);

// ----------------------------------------------------------------------
// LowerString()
// UpperString()
//    Convert the characters in "s" to lowercase or uppercase.  ASCII-only:
//    these functions intentionally ignore locale because they are applied to
//    identifiers used in the Protocol Buffer language, not to natural-language
//    strings.
// ----------------------------------------------------------------------
extern UTIL_API inline void LowerString(std::string *s)
{
	std::string::iterator end = s->end();
	for (std::string::iterator i = s->begin(); i != end; ++i)
	{
		// tolower() changes based on locale.  We don't want this!
		if ('A' <= *i && *i <= 'Z') *i += 'a' - 'A';
	}
}

extern UTIL_API inline void UpperString(std::string *s)
{
	std::string::iterator end = s->end();
	for (std::string::iterator i = s->begin(); i != end; ++i)
	{
		// toupper() changes based on locale.  We don't want this!
		if ('a' <= *i && *i <= 'z') *i += 'A' - 'a';
	}
}

// ----------------------------------------------------------------------
// StringReplace()
//    Give me a string and two patterns "old" and "new", and I replace
//    the first instance of "old" in the string with "new", if it
//    exists.  RETURN a new string, regardless of whether the replacement
//    happened or not.
// ----------------------------------------------------------------------

extern UTIL_API std::string StringReplace(const std::string& s, const std::string& oldsub,
								 const std::string& newsub, bool replace_all);

// ----------------------------------------------------------------------
// SplitStringUsing()
//    Split a string using a character delimiter. Append the components
//    to 'result'.  If there are consecutive delimiters, this function skips
//    over all of them.
// ----------------------------------------------------------------------
extern UTIL_API void SplitStringUsing(const std::string& full, const char* delim, 
							 std::vector<std::string>* res);

// Split a string using one or more byte delimiters, presented
// as a nul-terminated c string. Append the components to 'result'.
// If there are consecutive delimiters, this function will return
// corresponding empty strings.  If you want to drop the empty
// strings, try SplitStringUsing().
//
// If "full" is the empty string, yields an empty string as the only value.
// ----------------------------------------------------------------------
extern UTIL_API void SplitStringAllowEmpty(const std::string& full,
								  const char* delim,
								  std::vector<std::string>* result);

// ----------------------------------------------------------------------
// JoinStrings()
//    These methods concatenate a std::vector of strings into a C++ string, using
//    the C-string "delim" as a separator between components. There are two
//    flavors of the function, one flavor returns the concatenated string,
//    another takes a pointer to the target string. In the latter case the
//    target string is cleared and overwritten.
// ----------------------------------------------------------------------
extern UTIL_API void JoinStrings(const std::vector<std::string>& components,
						const char* delim, std::string* result);

inline std::string JoinStrings(const std::vector<std::string>& components,
							   const char* delim) 
{
	std::string result;
	JoinStrings(components, delim, &result);
	return result;
}

// ----------------------------------------------------------------------
// UnescapeCEscapeSequences()
//    Copies "source" to "dest", rewriting C-style escape sequences
//    -- '\n', '\r', '\\', '\ooo', etc -- to their ASCII
//    equivalents.  "dest" must be sufficiently large to hold all
//    the characters in the rewritten string (i.e. at least as large
//    as strlen(source) + 1 should be safe, since the replacements
//    are always shorter than the original escaped sequences).  It's
//    safe for source and dest to be the same.  RETURNS the length
//    of dest.
//
//    It allows hex sequences \xhh, or generally \xhhhhh with an
//    arbitrary number of hex digits, but all of them together must
//    specify a value of a single byte (e.g. \x0045 is equivalent
//    to \x45, and \x1234 is erroneous).
//
//    It also allows escape sequences of the form \uhhhh (exactly four
//    hex digits, upper or lower case) or \Uhhhhhhhh (exactly eight
//    hex digits, upper or lower case) to specify a Unicode code
//    point. The dest array will contain the UTF8-encoded version of
//    that code-point (e.g., if source contains \u2019, then dest will
//    contain the three bytes 0xE2, 0x80, and 0x99).
//
//    Errors: In the first form of the call, errors are reported with
//    LOG(ERROR). The same is true for the second form of the call if
//    the pointer to the string vector is NULL; otherwise, error
//    messages are stored in the vector. In either case, the effect on
//    the dest array is not defined, but rest of the source will be
//    processed.
//    ----------------------------------------------------------------------

extern UTIL_API int UnescapeCEscapeSequences(const char* source, char* dest);
extern UTIL_API int UnescapeCEscapeSequences(const char* source, char* dest, std::vector<std::string> *errors);

// ----------------------------------------------------------------------
// UnescapeCEscapeString()
//    This does the same thing as UnescapeCEscapeSequences, but creates
//    a new string. The caller does not need to worry about allocating
//    a dest buffer. This should be used for non performance critical
//    tasks such as printing debug messages. It is safe for src and dest
//    to be the same.
//
//    The second call stores its errors in a supplied string vector.
//    If the string vector pointer is NULL, it reports the errors with LOG().
//
//    In the first and second calls, the length of dest is returned. In the
//    the third call, the new string is returned.
// ----------------------------------------------------------------------

extern UTIL_API int UnescapeCEscapeString(const std::string& src, std::string* dest);
extern UTIL_API int UnescapeCEscapeString(const std::string& src, std::string* dest, std::vector<std::string> *errors);
extern UTIL_API std::string UnescapeCEscapeString(const std::string& src);

// ----------------------------------------------------------------------
// CEscapeString()
//    Copies 'src' to 'dest', escaping dangerous characters using
//    C-style escape sequences. This is very useful for preparing query
//    flags. 'src' and 'dest' should not overlap.
//    Returns the number of bytes written to 'dest' (not including the \0)
//    or -1 if there was insufficient space.
//
//    Currently only \n, \r, \t, ", ', \ and !isprint() chars are escaped.
// ----------------------------------------------------------------------
extern UTIL_API int CEscapeString(const char* src, int src_len,
						 char* dest, int dest_len);

// ----------------------------------------------------------------------
// CEscape()
//    More convenient form of CEscapeString: returns result as a "string".
//    This version is slower than CEscapeString() because it does more
//    allocation.  However, it is much more convenient to use in
//    non-speed-critical code like logging messages etc.
// ----------------------------------------------------------------------
extern UTIL_API std::string CEscape(const std::string& src);

// Like CEscape() but does not escape bytes with the upper bit set.
extern UTIL_API std::string Utf8SafeCEscape(const std::string& src);

// Like CEscape() but uses hex (\x) escapes instead of octals.
extern UTIL_API std::string CHexEscape(const std::string& src);

// ----------------------------------------------------------------------
// strto32()
// strtou32()
// strto64()
// strtou64()
//    Architecture-neutral plug compatible replacements for strtol() and
//    strtoul().  Long's have different lengths on ILP-32 and LP-64
//    platforms, so using these is safer, from the point of view of
//    overflow behavior, than using the standard libc functions.
// ----------------------------------------------------------------------
extern UTIL_API int32 strto32_adaptor(const char *nptr, char **endptr, int base);
extern UTIL_API uint32 strtou32_adaptor(const char *nptr, char **endptr, int base);

inline int32 strto32(const char *nptr, char **endptr, int base)
{
	if (sizeof(int32) == sizeof(long))
		return strtol(nptr, endptr, base);
	else
		return strto32_adaptor(nptr, endptr, base);
}

inline uint32 strtou32(const char *nptr, char **endptr, int base)
{
	if (sizeof(uint32) == sizeof(unsigned long))
		return strtoul(nptr, endptr, base);
	else
		return strtou32_adaptor(nptr, endptr, base);
}

// For now, long long is 64-bit on all the platforms we care about, so these
// functions can simply pass the call to strto[u]ll.
inline int64 strto64(const char *nptr, char **endptr, int base)
{
	COMPILE_ASSERT(sizeof(int64) == sizeof(long long), sizeof_int64_is_not_sizeof_long_long);
	return strtoll(nptr, endptr, base);
}

inline uint64 strtou64(const char *nptr, char **endptr, int base)
{
	COMPILE_ASSERT(sizeof(uint64) == sizeof(unsigned long long), sizeof_uint64_is_not_sizeof_long_long);
	return strtoull(nptr, endptr, base);
}

// ----------------------------------------------------------------------
// FastIntToBuffer()
// FastHexToBuffer()
// FastHex64ToBuffer()
// FastHex32ToBuffer()
// FastTimeToBuffer()
//    These are intended for speed.  FastIntToBuffer() assumes the
//    integer is non-negative.  FastHexToBuffer() puts output in
//    hex rather than decimal.  FastTimeToBuffer() puts the output
//    into RFC822 format.
//
//    FastHex64ToBuffer() puts a 64-bit unsigned value in hex-format,
//    padded to exactly 16 bytes (plus one byte for '\0')
//
//    FastHex32ToBuffer() puts a 32-bit unsigned value in hex-format,
//    padded to exactly 8 bytes (plus one byte for '\0')
//
//       All functions take the output buffer as an arg.
//    They all return a pointer to the beginning of the output,
//    which may not be the beginning of the input buffer.
// ----------------------------------------------------------------------

// Suggested buffer size for FastToBuffer functions.  Also works with
// DoubleToBuffer() and FloatToBuffer().
static const int kFastToBufferSize = 32;

extern UTIL_API char* FastInt32ToBuffer(int32 i, char* buffer);
extern UTIL_API char* FastInt64ToBuffer(int64 i, char* buffer);
static char* FastUInt32ToBuffer(uint32 i, char* buffer);  // inline below
static char* FastUInt64ToBuffer(uint64 i, char* buffer);  // inline below
extern UTIL_API char* FastHexToBuffer(int i, char* buffer);
extern UTIL_API char* FastHex64ToBuffer(uint64 i, char* buffer);
extern UTIL_API char* FastHex32ToBuffer(uint32 i, char* buffer);

// at least 22 bytes long
inline char* FastIntToBuffer(int i, char* buffer)
{
	return (sizeof(i) == 4 ?
		FastInt32ToBuffer(i, buffer) : FastInt64ToBuffer(i, buffer));
}
inline char* FastUIntToBuffer(unsigned int i, char* buffer)
{
	return (sizeof(i) == 4 ?
		FastUInt32ToBuffer(i, buffer) : FastUInt64ToBuffer(i, buffer));
}
inline char* FastLongToBuffer(long i, char* buffer)
{
	return (sizeof(i) == 4 ?
		FastInt32ToBuffer(i, buffer) : FastInt64ToBuffer(i, buffer));
}
inline char* FastULongToBuffer(unsigned long i, char* buffer)
{
	return (sizeof(i) == 4 ?
		FastUInt32ToBuffer(i, buffer) : FastUInt64ToBuffer(i, buffer));
}

// ----------------------------------------------------------------------
// FastInt32ToBufferLeft()
// FastUInt32ToBufferLeft()
// FastInt64ToBufferLeft()
// FastUInt64ToBufferLeft()
//
// Like the Fast*ToBuffer() functions above, these are intended for speed.
// Unlike the Fast*ToBuffer() functions, however, these functions write
// their output to the beginning of the buffer (hence the name, as the
// output is left-aligned).  The caller is responsible for ensuring that
// the buffer has enough space to hold the output.
//
// Returns a pointer to the end of the string (i.e. the null character
// terminating the string).
// ----------------------------------------------------------------------

extern UTIL_API char* FastInt32ToBufferLeft(int32 i, char* buffer);
extern UTIL_API char* FastUInt32ToBufferLeft(uint32 i, char* buffer);
extern UTIL_API char* FastInt64ToBufferLeft(int64 i, char* buffer);
extern UTIL_API char* FastUInt64ToBufferLeft(uint64 i, char* buffer);

// Just define these in terms of the above.
inline char* FastUInt32ToBuffer(uint32 i, char* buffer)
{
	FastUInt32ToBufferLeft(i, buffer);
	return buffer;
}
inline char* FastUInt64ToBuffer(uint64 i, char* buffer)
{
	FastUInt64ToBufferLeft(i, buffer);
	return buffer;
}

// ----------------------------------------------------------------------
// SimpleItoa()
//    Description: converts an integer to a string.
//
//    Return value: string
// ----------------------------------------------------------------------
extern UTIL_API std::string SimpleItoa(int i);
extern UTIL_API std::string SimpleItoa(unsigned int i);
extern UTIL_API std::string SimpleItoa(long i);
extern UTIL_API std::string SimpleItoa(unsigned long i);
extern UTIL_API std::string SimpleItoa(long long i);
extern UTIL_API std::string SimpleItoa(unsigned long long i);

// ----------------------------------------------------------------------
// SimpleDtoa()
// SimpleFtoa()
// DoubleToBuffer()
// FloatToBuffer()
//    Description: converts a double or float to a string which, if
//    passed to NoLocaleStrtod(), will produce the exact same original double
//    (except in case of NaN; all NaNs are considered the same value).
//    We try to keep the string short but it's not guaranteed to be as
//    short as possible.
//
//    DoubleToBuffer() and FloatToBuffer() write the text to the given
//    buffer and return it.  The buffer must be at least
//    kDoubleToBufferSize bytes for doubles and kFloatToBufferSize
//    bytes for floats.  kFastToBufferSize is also guaranteed to be large
//    enough to hold either.
//
//    Return value: string
// ----------------------------------------------------------------------
extern UTIL_API std::string SimpleDtoa(double value);
extern UTIL_API std::string SimpleFtoa(float value);

extern UTIL_API char* DoubleToBuffer(double i, char* buffer);
extern UTIL_API char* FloatToBuffer(float i, char* buffer);

// In practice, doubles should never need more than 24 bytes and floats
// should never need more than 14 (including null terminators), but we
// overestimate to be safe.
static const int kDoubleToBufferSize = 32;
static const int kFloatToBufferSize = 24;

// ----------------------------------------------------------------------
// NoLocaleStrtod()
//   Exactly like strtod(), except it always behaves as if in the "C"
//   locale (i.e. decimal points must be '.'s).
// ----------------------------------------------------------------------

extern UTIL_API double NoLocaleStrtod(const char* text, char** endptr);

// BSD-style safe and consistent string copy functions.
// Copies |src| to |dst|, where |dst_size| is the total allocated size of |dst|.
// Copies at most |dst_size|-1 characters, and always NULL terminates |dst|, as
// long as |dst_size| is not 0.  Returns the length of |src| in characters.
// If the return value is >= dst_size, then the output was truncated.
// NOTE: All sizes are in number of characters, NOT in bytes.
UTIL_API size_t strlcpy(char* dst, const char* src, size_t dst_size);
UTIL_API size_t wcslcpy(wchar_t* dst, const wchar_t* src, size_t dst_size);

// ASCII-specific tolower.  The standard library's tolower is locale sensitive,
// so we don't want to use it here.
template <class Char> inline Char ToLowerASCII(Char c)
{
	return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

// ASCII-specific toupper.  The standard library's toupper is locale sensitive,
// so we don't want to use it here.
template <class Char> inline Char ToUpperASCII(Char c)
{
	return (c >= 'a' && c <= 'z') ? (c + ('A' - 'a')) : c;
}

// Function objects to aid in comparing/searching strings.

template<typename Char> struct CaseInsensitiveCompare
{
public:
	bool operator()(Char x, Char y) const 
	{
		// TODO(darin): Do we really want to do locale sensitive comparisons here?
		// See http://crbug.com/24917
		return tolower(x) == tolower(y);
	}
};

template<typename Char> struct CaseInsensitiveCompareASCII
{
public:
	bool operator()(Char x, Char y) const
	{
		return ToLowerASCII(x) == ToLowerASCII(y);
	}
};

template <typename STR>
bool StartsWith(const STR& str, const STR& search, bool case_sensitive = true)
{
	if (case_sensitive)
	{
		return str.compare(0, search.length(), search) == 0;
	}
	else 
	{
		if (search.size() > str.size())
		{
			return false;
		}
		return std::equal(search.begin(), search.end(), str.begin(),
			CaseInsensitiveCompare<typename STR::value_type>());
	}
}

template <typename STR>
bool EndsWith(const STR& str, const STR& search, bool case_sensitive = true)
{
	typename STR::size_type str_length = str.length();
	typename STR::size_type search_length = search.length();
	if (search_length > str_length)
	{
		return false;
	}
	if (case_sensitive)
	{
		return str.compare(str_length - search_length, search_length, search) == 0;
	} 
	else
	{
		return std::equal(search.begin(), search.end(),
			str.begin() + (str_length - search_length),
			CaseInsensitiveCompare<typename STR::value_type>());
	}
}

// Removes characters in remove_chars from anywhere in input.  Returns true if
// any characters were removed.
// NOTE: Safe to use the same variable for both input and output.
template<typename STR>
bool RemoveChars(const STR& input,
				  const typename STR::value_type remove_chars[],
				  STR* output) 
{
	bool removed = false;
	size_t found;

	*output = input;

	found = output->find_first_of(remove_chars);
	while (found != STR::npos)
	{
		removed = true;
		output->replace(found, 1, STR());
		found = output->find_first_of(remove_chars, found);
	}

	return removed;
}

// Removes characters in trim_chars from the beginning and end of input.
// NOTE: Safe to use the same variable for both input and output.
enum TrimPositions {
	TRIM_NONE     = 0,
	TRIM_LEADING  = 1 << 0,
	TRIM_TRAILING = 1 << 1,
	TRIM_ALL      = TRIM_LEADING | TRIM_TRAILING,
};

template<typename STR>
TrimPositions TrimString(const STR& input,
						  const typename STR::value_type trim_chars[],
						  TrimPositions positions,
						  STR* output) 
{
	// Find the edges of leading/trailing whitespace as desired.
	const typename STR::size_type last_char = input.length() - 1;
	const typename STR::size_type first_good_char = (positions & TRIM_LEADING) ?
		input.find_first_not_of(trim_chars) : 0;
	const typename STR::size_type last_good_char = (positions & TRIM_TRAILING) ?
		input.find_last_not_of(trim_chars) : last_char;

	// When the string was all whitespace, report that we stripped off whitespace
	// from whichever position the caller was interested in.  For empty input, we
	// stripped no whitespace, but we still need to clear |output|.
	if (input.empty() ||
		(first_good_char == STR::npos) || (last_good_char == STR::npos))
	{
			bool input_was_empty = input.empty();  // in case output == &input
			output->clear();
			return input_was_empty ? TRIM_NONE : positions;
	}

	// Trim the whitespace.
	*output =
		input.substr(first_good_char, last_good_char - first_good_char + 1);

	// Return where we trimmed from.
	return static_cast<TrimPositions>(
		((first_good_char == 0) ? TRIM_NONE : TRIM_LEADING) |
		((last_good_char == last_char) ? TRIM_NONE : TRIM_TRAILING));
}

UTIL_API extern const wchar_t kWhitespaceWide[];
UTIL_API extern const char kWhitespaceASCII[];

// Removes characters in trim_chars from the beginning and end of input.
// NOTE: Safe to use the same variable for both input and output.
bool TrimString(const std::wstring& input,
				const wchar_t trim_chars[],
				std::wstring* output);
bool TrimString(const std::string& input,
				const char trim_chars[],
				std::string* output);

// Returns true if it's a whitespace character.
inline bool IsWhitespace(wchar_t c)
{
	return NULL != wcschr(kWhitespaceWide, c);
}

// Searches  for CR or LF characters.  Removes all contiguous whitespace
// strings that contain them.  This is useful when trying to deal with text
// copied from terminals.
// Returns |text|, with the following three transformations:
// (1) Leading and trailing whitespace is trimmed.
// (2) If |trim_sequences_with_line_breaks| is true, any other whitespace
//     sequences containing a CR or LF are trimmed.
// (3) All other whitespace sequences are converted to single spaces.
template<typename STR>
STR CollapseWhitespace(const STR& text,
						bool trim_sequences_with_line_breaks)
{
	STR result;
	result.resize(text.size());

	// Set flags to pretend we're already in a trimmed whitespace sequence, so we
	// will trim any leading whitespace.
	bool in_whitespace = true;
	bool already_trimmed = true;

	int chars_written = 0;
	for (typename STR::const_iterator i(text.begin()); i != text.end(); ++i)
	{
		if (IsWhitespace(*i)) 
		{
			if (!in_whitespace)
			{
				// Reduce all whitespace sequences to a single space.
				in_whitespace = true;
				result[chars_written++] = L' ';
			}
			if (trim_sequences_with_line_breaks && !already_trimmed &&
				((*i == '\n') || (*i == '\r')))
			{
					// Whitespace sequences containing CR or LF are eliminated entirely.
					already_trimmed = true;
					--chars_written;
			}
		}
		else
		{
			// Non-whitespace chracters are copied straight across.
			in_whitespace = false;
			already_trimmed = false;
			result[chars_written++] = *i;
		}
	}

	if (in_whitespace && !already_trimmed)
	{
		// Any trailing whitespace is eliminated.
		--chars_written;
	}

	result.resize(chars_written);
	return result;
}

UTIL_API TrimPositions TrimWhitespace(const std::wstring& input,
							 TrimPositions positions,
							 std::wstring* output);
UTIL_API TrimPositions TrimWhitespaceASCII(const std::string& input,
								  TrimPositions positions,
								  std::string* output);

// Deprecated. This function is only for backward compatibility and calls
// TrimWhitespaceASCII().
UTIL_API TrimPositions TrimWhitespace(const std::string& input,
							 TrimPositions positions,
							 std::string* output);

// Converts the elements of the given string.  This version uses a pointer to
// clearly differentiate it from the non-pointer variant.
template <class str> inline void StringToLowerASCII(str* s)
{
	for (typename str::iterator i = s->begin(); i != s->end(); ++i)
		*i = Util::ToLowerASCII(*i);
}

template <class str> inline str StringToLowerASCII(const str& s)
{
	// for std::string and std::wstring
	str output(s);
	StringToLowerASCII(&output);
	return output;
}

// Converts the elements of the given string.  This version uses a pointer to
// clearly differentiate it from the non-pointer variant.
template <class str> inline void StringToUpperASCII(str* s)
{
	for (typename str::iterator i = s->begin(); i != s->end(); ++i)
		*i = Util::ToUpperASCII(*i);
}

template <class str> inline str StringToUpperASCII(const str& s)
{
	// for std::string and std::wstring
	str output(s);
	StringToUpperASCII(&output);
	return output;
}

UTIL_API bool LowerCaseEqualsASCII(const std::wstring& input,
				const char* dst);
UTIL_API bool LowerCaseEqualsASCII(const std::string& input,
				const char* dst);

// Determines the type of ASCII character, independent of locale (the C
// library versions will change based on locale).
template <typename Char>
inline bool IsAsciiWhitespace(Char c)
{
	return c == ' ' || c == '\r' || c == '\n' || c == '\t';
}
template <typename Char>
inline bool IsAsciiAlpha(Char c)
{
	return ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
}
template <typename Char>
inline bool IsAsciiDigit(Char c)
{
	return c >= '0' && c <= '9';
}

template <typename Char>
inline bool IsHexDigit(Char c)
{
	return (c >= '0' && c <= '9') ||
		(c >= 'A' && c <= 'F') ||
		(c >= 'a' && c <= 'f');
}

template <typename Char>
inline Char HexDigitToInt(Char c)
{
	//UTIL_DCHECK(IsHexDigit(c));
	if (IsHexDigit(c))
	{
		if (c >= '0' && c <= '9')
			return c - '0';
		if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;
		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;
	}
	
	return 0;
}

enum DataUnits {
	DATA_UNITS_BYTE = 0,
	DATA_UNITS_KIBIBYTE,
	DATA_UNITS_MEBIBYTE,
	DATA_UNITS_GIBIBYTE,
};

// Return the unit type that is appropriate for displaying the amount of bytes
// passed in.
UTIL_API DataUnits GetByteDisplayUnits(int64 bytes);

// Return a byte string in human-readable format, displayed in units appropriate
// specified by 'units', with an optional unit suffix.
// Ex: FormatBytes(512, DATA_UNITS_KIBIBYTE, true) => "0.5 KB"
// Ex: FormatBytes(10*1024, DATA_UNITS_MEBIBYTE, false) => "0.1"
UTIL_API std::string FormatBytes(int64 bytes, DataUnits units, bool show_units);

// As above, but with "/s" units.
// Ex: FormatSpeed(512, DATA_UNITS_KIBIBYTE, true) => "0.5 KB/s"
// Ex: FormatSpeed(10*1024, DATA_UNITS_MEBIBYTE, false) => "0.1"
UTIL_API std::string FormatSpeed(int64 bytes, DataUnits units, bool show_units);

// Return a number formated with separators in the user's locale way.
// Ex: FormatNumber(1234567) => 1,234,567
UTIL_API std::string FormatNumber(int64 number);

template<class StringType>
void ReplaceSubstringsAfterOffset(StringType* str,
									typename StringType::size_type start_offset,
									const StringType& find_this,
									const StringType& replace_with,
									bool replace_all)
{
	if ((start_offset == StringType::npos) || (start_offset >= str->length()))
		return;

	UTIL_DCHECK(!find_this.empty());
	for (typename StringType::size_type offs(str->find(find_this, start_offset));
		offs != StringType::npos; 
		offs = str->find(find_this, offs))
	{
		str->replace(offs, find_this.length(), replace_with);
		offs += replace_with.length();

		if (!replace_all)
		{
			break;
		}
	}
}

template<typename STR>
static size_t Tokenize(const STR& str,
						const STR& delimiters,
						std::vector<STR>* tokens) 
{
	tokens->clear();

	typename STR::size_type start = str.find_first_not_of(delimiters);
	while (start != STR::npos)
	{
		typename STR::size_type end = str.find_first_of(delimiters, start + 1);
		if (end == STR::npos)
		{
			tokens->push_back(str.substr(start));
			break;
		}
		else
		{
			tokens->push_back(str.substr(start, end - start));
			start = str.find_first_not_of(delimiters, end + 1);
		}
	}

	return tokens->size();
}

// This is mpcomplete's pattern for saving a string copy when dealing with
// a function that writes results into a wchar_t[] and wanting the result to
// end up in a std::wstring.  It ensures that the std::wstring's internal
// buffer has enough room to store the characters to be written into it, and
// sets its .length() attribute to the right value.
//
// The reserve() call allocates the memory required to hold the string
// plus a terminating null.  This is done because resize() isn't
// guaranteed to reserve space for the null.  The resize() call is
// simply the only way to change the string's 'length' member.
//
// XXX-performance: the call to wide.resize() takes linear time, since it fills
// the string's buffer with nulls.  I call it to change the length of the
// string (needed because writing directly to the buffer doesn't do this).
// Perhaps there's a constant-time way to change the string's length.
template <class string_type>
inline typename string_type::value_type* WriteInto(string_type* str,
												   size_t length_with_null)
{
	str->reserve(length_with_null);
	str->resize(length_with_null - 1);
	return &((*str)[0]);
}

template<typename STR>
static size_t TokenizeT(const STR& str,
						const STR& delimiters,
						std::vector<STR>* tokens) 
{
	tokens->clear();

	typename STR::size_type start = str.find_first_not_of(delimiters);
	while (start != STR::npos)
	{
		typename STR::size_type end = str.find_first_of(delimiters, start + 1);
		if (end == STR::npos)
		{
			tokens->push_back(str.substr(start));
			break;
		}
		else 
		{
			tokens->push_back(str.substr(start, end - start));
			start = str.find_first_not_of(delimiters, end + 1);
		}
	}

	return tokens->size();
}

// If the size of |input| is more than |max_len|, this function returns true and
// |input| is shortened into |output| by removing chars in the middle (they are
// replaced with up to 3 dots, as size permits).
// Ex: ElideString(L"Hello", 10, &str) puts Hello in str and returns false.
// ElideString(L"Hello my name is Tom", 10, &str) puts "Hell...Tom" in str and
// returns true.
UTIL_API bool ElideString(const std::wstring& input, int max_len, std::wstring* output);

template <typename CHAR>
static bool IsWildcard(char character)
{
	return character == CHAR('*') || character == CHAR('?');
}

template <typename CHAR>
struct NextChar
{
	CHAR operator()(const CHAR** p, const CHAR* end)
	{
		if (end - *p < 1)
		{
			return CHAR();
		}

		return *(*p)++;
	}
};

// Move the strings pointers to the point where they start to differ.
template <typename CHAR, typename NEXT>
static void EatSameChars(const CHAR** pattern, const CHAR* pattern_end,
						 const CHAR** string, const CHAR* string_end,
						 NEXT next)
{
	const CHAR* escape = NULL;
	while (*pattern != pattern_end && *string != string_end)
	{
		if (!escape && IsWildcard(**pattern))
		{
			// We don't want to match wildcard here, except if it's escaped.
			return;
		}

		// Check if the escapement char is found. If so, skip it and move to the
		// next character.
		if (!escape && **pattern == '\\')
		{
			escape = *pattern;
			next(pattern, pattern_end);
			continue;
		}

		// Check if the chars match, if so, increment the ptrs.
		const CHAR* pattern_next = *pattern;
		const CHAR* string_next = *string;
		CHAR pattern_char = next(&pattern_next, pattern_end);
		if (pattern_char == next(&string_next, string_end))
		{
			*pattern = pattern_next;
			*string = string_next;
		}
		else
		{
			// Uh ho, it did not match, we are done. If the last char was an
			// escapement, that means that it was an error to advance the ptr here,
			// let's put it back where it was. This also mean that the MatchPattern
			// function will return false because if we can't match an escape char
			// here, then no one will.
			if (escape) 
			{
				*pattern = escape;
			}

			return;
		}

		escape = NULL;
	}
}

template <typename CHAR, typename NEXT>
static void EatWildcard(const CHAR** pattern, const CHAR* end, NEXT next)
{
	while (*pattern != end)
	{
		if (!IsWildcard(**pattern))
		{
			return;
		}
		next(pattern, end);
	}
}

template <typename CHAR, typename NEXT>
static bool MatchPattern(const CHAR* eval, const CHAR* eval_end,
						  const CHAR* pattern, const CHAR* pattern_end,
						  int depth,
						  NEXT next = NextChar<CHAR>())
{
	const int kMaxDepth = 16;
	if (depth > kMaxDepth)
		return false;

	// Eat all the matching chars.
	EatSameChars(&pattern, pattern_end, &eval, eval_end, next);

	// If the string is empty, then the pattern must be empty too, or contains
	// only wildcards.
	if (eval == eval_end) 
	{
		EatWildcard(&pattern, pattern_end, next);
		return pattern == pattern_end;
	}

	// Pattern is empty but not string, this is not a match.
	if (pattern == pattern_end)
		return false;

	// If this is a question mark, then we need to compare the rest with
	// the current string or the string with one character eaten.
	const CHAR* next_pattern = pattern;
	next(&next_pattern, pattern_end);
	if (pattern[0] == '?')
	{
		if (MatchPattern(eval, eval_end, next_pattern, pattern_end, depth + 1, next))
		{
			return true;
		}
		const CHAR* next_eval = eval;
		next(&next_eval, eval_end);
		if (MatchPattern(next_eval, eval_end, next_pattern, pattern_end, depth + 1, next))
		{
			return true;
		}
	}

	// This is a *, try to match all the possible substrings with the remainder
	// of the pattern.
	if (pattern[0] == '*')
	{
		// Collapse duplicate wild cards (********** into *) so that the
		// method does not recurse unnecessarily. http://crbug.com/52839
		EatWildcard(&next_pattern, pattern_end, next);

		while (eval != eval_end)
		{
			if (MatchPattern(eval, eval_end, next_pattern, pattern_end, depth + 1, next))
			{
				return true;
			}
			eval++;
		}

		// We reached the end of the string, let see if the pattern contains only
		// wildcards.
		if (eval == eval_end)
		{
			EatWildcard(&pattern, pattern_end, next);
			if (pattern != pattern_end)
			{
				return false;
			}
			return true;
		}
	}

	return false;
}

#endif

UTIL_END

#endif
