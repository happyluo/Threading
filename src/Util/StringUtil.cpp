// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <cstring>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <iomanip>
#include <errno.h>
#include <float.h>    // FLT_DIG and DBL_DIG
#include <limits>
#include <limits.h>
#include <stdio.h>
#include <iterator>
#include <bitset>

#ifdef __MINGW32__
//#if defined(__MINGW32__) || (defined(_MSC_VER) && (_MSC_VER < 1300))
#	include <limits.h>
#endif

#if defined(__hpux)
#	include <inttypes.h>
#endif

#include <Util/StringUtil.h>
#include <Util/StringConverter.h>
#include <Build/UndefSysMacros.h>
#include <Logging/Logger.h>
#include <Util/ScopedArray.h>
#include <Build/UsefulMacros.h>

//#ifdef OS_WINRT
//#	include <Util/ScopedArray.h>
//#endif

//#if _WIN32
//#	define vsnprintf _vsnprintf
//#endif

using namespace std;

UTIL_BEGIN

String::String()
{
}

String::String(const String& other) : std::string(other)
{
}

String::String(const String& src, String::size_type i, String::size_type n) : std::string(src, i, n)
{
}

String::String(const char* src, String::size_type n) : std::string(src, n)
{
}

String::String(const char* src) : std::string(src)
{
}

String::String(String::size_type n, char c) : std::string(n, c)
{
}

String::String(const std::string& src) : std::string(src)
{
}

String::~String()
{
}

String String::ToUTF8(const std::string& internalCode) const
{
	StringConverterPtr stringConverter = "local" == internalCode ?
#	if defined(_WIN32) && !defined(ICONV_ON_WINDOWS)
		new WindowsStringConverter() : new WindowsStringConverter(internalCode);
#	else
		new IconvStringConverter<char>() : new IconvStringConverter<char>(internalCode.c_str());
#	endif

	return Util::NativeToUTF8(stringConverter, *this);
}

String String::ToUTF8(const String& src, const std::string& internalCode)
{
	StringConverterPtr stringConverter = "local" == internalCode ?
#	if defined(_WIN32) && !defined(ICONV_ON_WINDOWS)
		new WindowsStringConverter() : new WindowsStringConverter(internalCode);
#	else
		new IconvStringConverter<char>() : new IconvStringConverter<char>(internalCode.c_str());
#	endif

	return Util::NativeToUTF8(stringConverter, src);
}

std::wstring String::ToWString(const std::string& internalCode) const
{
	StringConverterPtr stringConverter = "local" == internalCode ?
#	if defined(_WIN32) && !defined(ICONV_ON_WINDOWS)
		new WindowsStringConverter() : new WindowsStringConverter(internalCode);
#	else
		new IconvStringConverter<char>() : new IconvStringConverter<char>(internalCode.c_str());
#	endif

	return Util::StringToWstring(Util::NativeToUTF8(stringConverter, *this));
}

std::wstring String::ToWString(const String& src, const std::string& internalCode)
{
	StringConverterPtr stringConverter = "local" == internalCode ?
#	if defined(_WIN32) && !defined(ICONV_ON_WINDOWS)
		new WindowsStringConverter() : new WindowsStringConverter(internalCode);
#	else
		new IconvStringConverter<char>() : new IconvStringConverter<char>(internalCode.c_str());
#	endif

	return Util::StringToWstring(Util::NativeToUTF8(stringConverter, src));
}

#ifdef _WIN32
// Creates a UTF-16 wide string from the given ANSI string, allocating
// memory using new. The caller is responsible for deleting the return
// value using delete[]. Returns the wide string, or NULL if the
// input is NULL.
LPCWSTR String::AnsiToUTF16(const char* ansi)
{
	if (!ansi) 
	{
		return NULL;
	}
	const int length = strlen(ansi);
	const int unicode_length =
		MultiByteToWideChar(CP_ACP, 0, ansi, length,
		NULL, 0);
	WCHAR* unicode = new WCHAR[unicode_length + 1];
	MultiByteToWideChar(CP_ACP, 0, ansi, length,
		unicode, unicode_length);
	unicode[unicode_length] = 0;
	return unicode;
}

// Creates an ANSI string from the given wide string, allocating
// memory using new. The caller is responsible for deleting the return
// value using delete[]. Returns the ANSI string, or NULL if the
// input is NULL.
const char* String::UTF16ToAnsi(LPCWSTR utf16_str)
{
	if (!utf16_str) 
	{
		return NULL;
	}

	const int ansi_length =
		WideCharToMultiByte(CP_ACP, 0, utf16_str, -1,
		NULL, 0, NULL, NULL);
	char* ansi = new char[ansi_length + 1];
	WideCharToMultiByte(CP_ACP, 0, utf16_str, -1,
		ansi, ansi_length, NULL, NULL);
	ansi[ansi_length] = 0;
	return ansi;
}

#endif  // _WIN32

//
// Message formatting.
//
std::string String::ComposeArgv(const std::string& fmt, int argc, const std::string* const* argv)
{
	std::string::size_type result_size = fmt.size();

	// Guesstimate the final string size.
	for (int i = 0; i < argc; ++i)
	{
		result_size += argv[i]->size();
	}

	std::string result;
	result.reserve(result_size);

	const char* const pfmt = fmt.c_str();
	const char* start = pfmt;

	while (const char* const stop = std::strchr(start, '%'))
	{
		if (stop[1] == '%')
		{
			result.append(start, stop - start + 1);
			start = stop + 2;
		}
		else
		{
			const int index = Util::DigitValue(stop[1]) - 1;

			if (index >= 0 && index < argc)
			{
				result.append(start, stop - start);
				result += *argv[index];
				start = stop + 2;
			}
			else
			{
				const char* const next = stop + 1;

				// Copy invalid substitutions literally to the output.
				result.append(start, next - start);

				STDERR_LOG(LOGLEVEL_WARNING) << "invalid substitution \"" 
					<< result.c_str() + result.size() - (next - stop)
					<< "\" in fmt string \"" << pfmt << "\"";

				start = next;
			}
		}
	}

	result.append(start, pfmt + fmt.size() - start);

	return result;
}

//
// Util::string::FormatStream
//
String::FormatStream::FormatStream() : m_stream()
{
}

String::FormatStream::~FormatStream()
{
}

std::string String::FormatStream::to_string() const
{
#ifdef HAS_STD_WSTREAM
	const std::wstring wstr = m_stream.str();
	//return Util::WstringToString(wstr);

	StringConverterPtr stringConverter = 
#	if defined(_WIN32) && !defined(ICONV_ON_WINDOWS)
		new WindowsStringConverter();
#	else
		new IconvStringConverter<char>();
#	endif

	return Util::UTF8ToNative(stringConverter, Util::WstringToString(wstr));

#else
	return m_stream.str();

#endif
}


#ifdef HAS_STD_WSTREAM

std::wistream& operator >>(std::wistream& is, Util::String& strret)
{
	std::wstring wstr;
	is >> wstr;

	StringConverterPtr stringConverter = 
#	if defined(_WIN32) && !defined(ICONV_ON_WINDOWS)
		new WindowsStringConverter();
#	else
		new IconvStringConverter<char>();
#	endif

	strret = Util::UTF8ToNative(stringConverter, Util::WstringToString(wstr));

	return is;
}

std::wostream& operator <<(std::wostream& os, const Util::String& strsrc)
{
	StringConverterPtr stringConverter = 
#	if defined(_WIN32) && !defined(ICONV_ON_WINDOWS)
		new WindowsStringConverter();
#	else
		new IconvStringConverter<char>();
#	endif

	// This won't work if the String contains NUL characters.  Unfortunately,
	// std::wostream::write() ignores Format flags, so we cannot use that.
	// The only option would be to create a temporary std::wstring.  However,
	// even then GCC's libstdc++-v3 prints only the characters up to the first
	// NUL.  Given this, there doesn't seem much of a point in allowing NUL in
	// formatted output.  The semantics would be unclear anyway: what's the
	// screen width of a NUL?
	os << Util::StringToWstring(Util::NativeToUTF8(stringConverter, strsrc));

	return os;
}

#endif


namespace
{
//
// Write the byte b as an escape sequence if it isn't a printable ASCII
// character and append the escape sequence to s. Additional characters
// that should be escaped can be passed in special. If b is any of these
// characters, b is preceded by a backslash in s.
//
void
EncodeChar(string::value_type b, string& s, const string& special)
{
	switch(b)
	{
	case '\\': 
		{
			s.append("\\\\");
			break;
		}

	case '\'': 
		{
			s.append("\\'");
			break;
		}

	case '"': 
		{
			s.append("\\\"");
			break;
		}

	case '\b': 
		{
			s.append("\\b");
			break;
		}

	case '\f': 
		{
			s.append("\\f");
			break;
		}

	case '\n': 
		{
			s.append("\\n");
			break;
		}

	case '\r': 
		{
			s.append("\\r");
			break;
		}

	case '\t': 
		{
			s.append("\\t");
			break;
		}

	default: 
		{
			unsigned char i = static_cast<unsigned char>(b);
			if (!(i >= 32 && i <= 126))
			{
				s.push_back('\\');
				string octal = Util::String::ToOctalString(i);
				//
				// Add leading zeroes so that we avoid problems during
				// decoding. For example, consider the escaped string
				// \0013 (i.e., a character with value 1 followed by the
				// character '3'). If the leading zeroes were omitted, the
				// result would be incorrectly interpreted as a single
				// character with value 11.
				//
				for (string::size_type j = octal.size(); j < 3; j++)
				{
					s.push_back('0');
				}
				s.append(octal);
			}
			else if (special.find(b) != string::npos)
			{
				s.push_back('\\');
				s.push_back(b);
			}
			else
			{
				s.push_back(b);
			}
			break;
		}
	}
}

}

//
// Add escape sequences (such as "\n", or "\007") to make a string
// readable in ASCII. Any characters that appear in special are
// prefixed with a backslash in the returned string.
//
string
String::EscapeString(const string& s, const string& special)
{
	for (string::size_type i = 0; i < special.size(); ++i)
	{
		if (static_cast<unsigned char>(special[i]) < 32 || static_cast<unsigned char>(special[i]) > 126)
		{
			throw IllegalArgumentException(__FILE__, __LINE__, "special characters must be in ASCII range 32-126");
		}
	}

	string result;
	for (string::size_type i = 0; i < s.size(); ++i)
	{
		EncodeChar(s[i], result, special);
	}

	return result;
}

namespace
{

char
CheckChar(const string& s, string::size_type pos)
{
	unsigned char c = static_cast<unsigned char>(s[pos]);
	if (!(c >= 32 && c <= 126))
	{
		ostringstream ostr;
		if (pos > 0)
		{
			ostr << "character after `" << s.substr(0, pos) << "'";
		}
		else
		{
			ostr << "first character";
		}
		ostr << " is not a printable ASCII character (ordinal " << (int)c << ")";
		throw IllegalArgumentException(__FILE__, __LINE__, ostr.str());
	}
	return c;
}

//
// Decode the character or escape sequence starting at start and return it.
// end marks the one-past-the-end position of the substring to be scanned.
// nextStart is set to the index of the first character following the decoded
// character or escape sequence.
//
char
DecodeChar(const string& s, string::size_type start, string::size_type end, string::size_type& nextStart)
{
	assert(start < end);
	assert(end <= s.size());

	char c;

	if (s[start] != '\\')
	{
		c = CheckChar(s, start++);
	}
	else
	{
		if (start + 1 == end)
		{
			throw IllegalArgumentException(__FILE__, __LINE__, "trailing backslash");
		}
		switch(s[++start])
		{
		case '\\': 
		case '\'': 
		case '"': 
			{
				c = s[start++];
				break;
			}
		case 'b': 
			{
				++start;
				c = '\b';
				break;
			}
		case 'f': 
			{
				++start;
				c = '\f';
				break;
			}
		case 'n': 
			{
				++start;
				c = '\n';
				break;
			}
		case 'r': 
			{
				++start;
				c = '\r';
				break;
			}
		case 't': 
			{
				++start;
				c = '\t';
				break;
			}
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			{
				int val = 0;
				for (int j = 0; j < 3 && start < end; ++j)
				{
					int charVal = s[start++] - '0';
					if (charVal < 0 || charVal > 7)
					{
						--start;
						break;
					}
					val = val * 8 + charVal;
				}
				if (val > 255)
				{
					ostringstream ostr;
					ostr << "octal value \\" << oct << val << dec << " (" << val << ") is out of range";
					throw IllegalArgumentException(__FILE__, __LINE__, ostr.str());
				}
				c = (char)val;
				break;
			}
		default:
			{
				c = CheckChar(s, start++);
				break;
			}
		}
	}
	nextStart = start;
	return c;
}

//
// Remove escape sequences from s and append the result to sb.
// Return true if successful, false otherwise.
//
void
DecodeString(const string& s, string::size_type start, string::size_type end, string& sb)
{
	while (start < end)
	{
		sb.push_back(DecodeChar(s, start, end, start));
	}
}

}

//
// Remove escape sequences added by escapeString.
//
string
String::UnescapeString(const string& s, string::size_type start, string::size_type end)
{
	assert(start <= end && end <= s.size());

	string result;
	result.reserve(end - start);
	result.clear();
	DecodeString(s, start, end, result);
	return result;
}

bool
String::SplitString(const string& str, const string& delim, vector<string>& result, bool keepblank)
{
	string::size_type pos = 0;
	string::size_type length = str.length();
	string elt;

	char quoteChar = '\0';
	while (pos < length)
	{
		if (quoteChar == '\0' && (str[pos] == '"' || str[pos] == '\''))
		{
			quoteChar = str[pos++];
			continue; // Skip the quote
		}
		else if (quoteChar == '\0' && str[pos] == '\\' && pos + 1 < length && 
			(str[pos + 1] == '\'' || str[pos + 1] == '"'))
		{
			++pos;
		}
		else if (quoteChar != '\0' && str[pos] == '\\' && pos + 1 < length && str[pos + 1] == quoteChar)
		{
			++pos;
		}
		else if (quoteChar != '\0' && str[pos] == quoteChar)
		{
			++pos;
			quoteChar = '\0';
			continue; // Skip the end quote
		}
		else if (delim.find(str[pos]) != string::npos)
		{
			if (quoteChar == '\0')
			{
				++pos;
				if (elt.length() > 0 || keepblank)
				{
					result.push_back(elt);
					elt = "";
				}
				continue;
			}
		}

		if (pos < length)
		{
			elt += str[pos++];
		}
	}

	if (elt.length() > 0 || keepblank)
	{
		result.push_back(elt);
	}
	if (quoteChar != '\0')
	{
		return false; // Unmatched quote.
	}
	return true;
}

string
String::JoinString(const std::vector<std::string>& values, const std::string& delimiter)
{
	ostringstream out;
	for (unsigned int i = 0; i < values.size(); i++)
	{
		if (i != 0)
		{
			out << delimiter;
		}
		out << values[i];
	}
	return out.str();
}

//
// Trim white space (" \t\r\n")
//
string
String::Trim(const string& s)
{
	static const string delim = " \t\r\n";
	string::size_type beg = s.find_first_not_of(delim);
	if (beg == string::npos)
	{
		return "";
	}
	else
	{
		return s.substr(beg, s.find_last_not_of(delim) - beg + 1);
	}
}

//
// If a single or double quotation mark is found at the start position,
// then the position of the matching closing quote is returned. If no
// quotation mark is found at the start position, then 0 is returned.
// If no matching closing quote is found, then -1 is returned.
//
string::size_type
String::CheckQuote(const string& s, string::size_type start)
{
	string::value_type quoteChar = s[start];
	if (quoteChar == '"' || quoteChar == '\'')
	{
		start++;
		string::size_type pos;
		while (start < s.size() && (pos = s.find(quoteChar, start)) != string::npos)
		{
			if (s[pos - 1] != '\\')
			{
				return pos;
			}
			start = pos + 1;
		}
		return string::npos; // Unmatched quote.
	}
	return 0; // Not quoted.
}

string::size_type
String::ExistQuote(const string& s, string::size_type start)
{
	string::size_type pos;
	while (start < s.size() && (pos = s.find_first_of("\"\'", start)) != string::npos)
	{
		if (s[pos - 1] != '\\')
		{
			return pos;
		}
		start = pos + 1;
	}

	return string::npos; // No quote.
}

//
// Match `s' against the pattern `pat'. A * in the pattern acts
// as a wildcard: it matches any non-empty sequence of characters.
// We match by hand here because it's portable across platforms 
// (whereas regex() isn't). Only one * per pattern is supported.
//
bool
String::Match(const string& s, const string& pat, bool emptyMatch)
{
	assert(!s.empty());
	assert(!pat.empty());

	//
	// If pattern does not contain a wildcard just compare strings.
	//
	string::size_type beginIndex = pat.find('*');
	if (beginIndex == string::npos)
	{
		return s == pat;
	}

	//
	// Make sure start of the strings match
	//
	if (beginIndex > s.length() || s.substr(0, beginIndex) != pat.substr(0, beginIndex))
	{
		return false;
	}

	//
	// Make sure there is something present in the middle to match the
	// wildcard. If emptyMatch is true, allow a match of "".
	//
	string::size_type endLength = pat.length() - beginIndex - 1;
	if (endLength > s.length())
	{
		return false;
	}
	string::size_type endIndex = s.length() - endLength;
	if (endIndex < beginIndex || (!emptyMatch && endIndex == beginIndex))
	{
		return false;
	}

	//
	// Make sure end of the strings match
	//
	if (s.substr(endIndex, s.length()) != pat.substr(beginIndex + 1, pat.length()))
	{
		return false;
	}

	return true;
}

std::string 
String::TranslatingCR2LF(const std::string& src)
{
	// Translating both the two-character sequence #xD #xA and any #xD that is not followed by #xA to 
	// a single #xA character.

	// Process the buffer in place to normalize new lines. (See comment above.)
	// Copies from the 'pread' to 'pwrite' pointer, where pread can advance faster if
	// a newline-carriage return is hit.
	//
	// Wikipedia:
	// Systems based on ASCII or a compatible character set use either LF  (Line feed, '\n', 0x0A, 10 in decimal) or 
	// CR (Carriage return, '\r', 0x0D, 13 in decimal) individually, or CR followed by LF (CR+LF, 0x0D 0x0A)...
	//		* LF:    Multics, Unix and Unix-like systems (GNU/Linux, AIX, Xenix, Mac OS X, FreeBSD, etc.), BeOS, Amiga, RISC OS, and others
	//		* CR+LF: DEC RT-11 and most other early non-Unix, non-IBM OSes, CP/M, MP/M, DOS, OS/2, Microsoft Windows, Symbian OS
	//		* CR:    Commodore 8-bit machines, Apple II family, Mac OS up to version 9 and OS-9

	std::string result(src);

	const char *pread = &result[0];
	char *pwrite = &result[0];
	const char *pend = &result[0] + result.size();

	const char CR = 0x0d;
	const char LF = 0x0a;

	while (pread != pend)
	{
		assert(pwrite <= pread);

		if (CR == *pread)
		{
			*pwrite++ = LF;
			++pread;
			if (LF == *pread)
			{
				++pread;
			}
		}
		else
		{
			*pwrite++ = *pread++;
		}
	}

	result.resize(pwrite - &result[0]);

	return result;
}

//std::string 
//String::ToHexString(unsigned long n, bool bupper)
//{
//	string s;
//	size_t size = sizeof(unsigned long) * 2;
//	s.resize(size);
//	string::size_type charPos = size;
//	
//	const int radix = 1 << 4;
//	int mask = radix - 1;
//	char base = bupper ? 'A' : 'a';
//
//	do
//	{
//		int d = n & mask;
//		s[--charPos] = d < 10 ? '0' + d : base + (d - 10);
//		n >>= 4;
//	}while (0 != n);
//
//	return string(s, charPos, (size - charPos));
//}

//std::string 
//String::ToOctalString(unsigned long n)
//{
//	string s;
//	size_t size = sizeof(unsigned long) * 8;
//	s.resize(size);
//	string::size_type charPos = size;
//	const int radix = 1 << 3;
//	int mask = radix - 1;
//	do
//	{
//		s[--charPos] = '0' + static_cast<int>(n & mask);
//		n >>= 3;
//	}while (0 != n);
//
//	return string(s, charPos, (size - charPos));
//}

//std::string 
//String::ToBinaryString(unsigned long n)
//{
//	string s;
//	size_t size = sizeof(unsigned long) * 8;
//	s.resize(size);
//	string::size_type charPos = size;
//
//	do
//	{
//		//s[--charPos] = n & 1 ? '1' : '0';
//		s[--charPos] = (n & 1) + '0';
//		n >>= 1;
//	}while (0 != n);
//
//	return string(s, charPos, (size - charPos));
//}

std::string
String::ToLower(const std::string& s)
{
	string result;
	result.reserve(s.size());
	for (unsigned int i = 0; i < s.length(); ++i)
	{
		if (isascii(s[i]))
		{
			result += tolower(static_cast<unsigned char>(s[i]));
		}
		else
		{
			result += s[i];
		}
	}
	return result;
}

std::string
String::ToUpper(const std::string& s)
{
	string result;
	result.reserve(s.size());
	for (unsigned int i = 0; i < s.length(); ++i)
	{
		if (isascii(s[i]))
		{
			result += toupper(static_cast<unsigned char>(s[i]));
		}
		else
		{
			result += s[i];
		}
	}
	return result;
}

unsigned long 
String::Hash(const std::string& s)
{
	unsigned long hashCode = s.length();
	size_t step = (s.length() >> 5) + 1;
	for (size_t i = s.length(); i >= step; i -= step)
	{
		hashCode = hashCode ^ ((hashCode << 5) + ( hashCode >> 2) + (unsigned long)s[i-1]);
	}

	return hashCode;
}

string
String::RemoveWhitespace(const std::string& s)
{
	string result;
	for (unsigned int i = 0; i < s.length(); ++ i)
	{
		if (!isspace(static_cast<unsigned char>(s[i])))
		{
			result += s[i];
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////
/// String To Int64
static const string allDigits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

//
// Table to convert ASCII digits/letters into their value (100 for unused slots)
//
static const char digitVal[] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,				// '0' - '9'
	100, 100, 100, 100, 100, 100, 100,			// punctuation
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,		// 'A' - 'J'
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29,     // 'K' - 'T'
	30, 31, 32, 33, 34, 35						// 'U' - 'Z'
};

#ifdef __MINGW32__
//#if defined(__MINGW32__) || (defined(_MSC_VER) && (_MSC_VER < 1300))

namespace UtilInternal
{
//
// The MINGW runtime does not include _strtoi64, so we provide our own implementation
//

static Util::Int64 strToInt64Impl(const char* s, char** endptr, int base)
{
	//
	// Assume nothing will be there to convert for now
	//
	if (endptr)
	{
		*endptr = const_cast<char *>(s);
	}

	//
	// Skip leading whitespace
	//
	while (*s && isspace(static_cast<unsigned char>(*s)))
	{
		++s;
	}

	//
	// Check for sign
	//
	int sign = 1;
	if (*s == '+')
	{
		++s;
	}
	else if (*s == '-')
	{
		sign = -1;
		++s;
	}

	//
	// Check that base is valid
	//
	if (base == 0)
	{
		if (*s == '0')
		{
			base = 8;
			++s;

			//
			// We have at least this zero
			//
			if (endptr)
			{
				*endptr = const_cast<char *>(s);
			}

			if (*s == 'x' || *s == 'X')
			{
				base = 16;
				++s;
			}
		}
		else
		{
			base = 10;
		}
	}
	else if (base < 2 || base > 36)
	{
		errno = EINVAL;
		return 0;
	}

	//
	// Check that we have something left to parse
	//
	if (*s == '\0')
	{
		//
		// We did not read any new digit so we don't update endptr
		//
		return 0;
	}

	Int64 result = 0;
	bool overflow = false;
	bool digitFound = false;
	const string validDigits(allDigits.begin(), allDigits.begin() + base);
	while (*s && validDigits.find_first_of(toupper(static_cast<unsigned char>(*s))) != validDigits.npos)
	{   
		digitFound = true;
		if (!overflow)
		{
			int digit = digitVal[toupper(static_cast<unsigned char>(*s)) - '0'];
			assert(digit != 100);
			if (result < _I64_MAX / base)
			{
				result *= base;
				result += digit;
			}
			else if ((digit <= _I64_MAX % base) || (sign == -1 && digit == _I64_MAX % base + 1))
			{
				result *= base;
				result += digit;
			}
			else
			{
				overflow = true;
				result = sign == -1 ? _I64_MIN : _I64_MAX;
			}
		}
		++s;
	}

	if (overflow)
	{
		errno = ERANGE;
	}
	else
	{
		result *= sign;
	}

	if (digitFound && endptr != 0)
	{
		*endptr = const_cast<char *>(s);
	}

	return result;
}
}

#endif


Util::Int64 
String::ToInt64(const char* s, char** endptr, int base)
{
#if defined(_WIN32)
#   ifdef __MINGW32__
	return strToInt64Impl(s, endptr, base);
#   else
	return _strtoi64(s, endptr, base);
#   endif
#elif defined(UTIL_64)
	return strtol(s, endptr, base);
#elif defined(__hpux)
	return __strtoll(s, endptr, base);
#else
	return strtoll(s, endptr, base);
#endif
}

bool 
String::ToInt64(const string& s,  Util::Int64& result)
{
	const char* start = s.c_str();
	char* end = 0;
	errno = 0;
	result = ToInt64(start, &end, 0);
	return (errno == 0 && start != end);
}

unsigned long 
String::ToULong(const std::string& strval, size_t* endindex, unsigned int base)
{
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
	else if (base < 2 || base > 16)	//else if (2 != base && 8 != base && 10 != base && 16 != base)
	{
		errno = EINVAL;
		return 0;
	}

	int exp = 0;
	unsigned long result = 0;
	unsigned value;
	while (isxdigit(*iter) 
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
			string strexp;
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

	result = static_cast<unsigned long>(result * ::pow(1.0 * base, exp) + 0.5);

	return sign * result;
}

long 
String::ToLong(const std::string& strval, size_t* endindex, unsigned int base)
{
	return static_cast<long>(ToULong(strval, endindex, base));
}

double 
String::ToDouble(const std::string& strval, size_t* endindex, int precision)
{
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

	int exp = 0;
	double result = 0;	
	while (isdigit(*iter))
	{
		result = result * 10 + *iter - '0';
		++iter;
	}

	if ('.' == *iter)
	{
		++iter;
		for (int i = -1; precision-- && isdigit(*iter); --i, ++iter)
		{
			result = result * 10 + (*iter - '0');
			exp = i;
		}
	}

	if ('E' == toupper(*iter))
	{
		++iter;
		string strexp;
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

	if (endindex)
	{
		*endindex = iter - strval.c_str();
	}

	result *= ::pow(10.0, exp);

	return sign * result;

	//////////////////////////////////////////////////////////////////////////
	//double integerpart = 0;
	//double decimalpart = 0;
	//std::string::const_pointer dot = 0;
	//
	//for (; *iter; ++iter)
	//{
	//	if (isdigit(*iter))
	//	{
	//		if (!dot)
	//		{
	//			integerpart = integerpart * 10 + *iter - '0';
	//		}
	//		else if (0 != precision)
	//		{
	//			--precision;
	//			decimalpart = decimalpart + (*iter - '0') * ::pow(10., dot - iter);
	//		}
	//	}
	//	else if (!dot && '.' == *iter)
	//	{
	//		dot = iter;
	//	}
	//	else if ('E' == toupper(*iter))
	//	{
	//		return sign * (integerpart + decimalpart) * ::pow(10., ToLong(strval.substr(iter - strval.c_str() + 1)));
	//	}
	//	else
	//	{
	//		break;
	//	}
	//}

	//if (endindex)
	//{
	//	*endindex = iter - strval.c_str();
	//}

	//return sign * (integerpart + decimalpart);
}

std::string 
String::ToString(unsigned long n)
{
	string s;
	size_t size = sizeof(unsigned long) * 8;
	s.resize(size);
	string::size_type charPos = size;

	do
	{
		s[--charPos] = n % 10 + '0';
		n /= 10;
	}while (0 != n);

	return string(s, charPos, (size - charPos));
}

std::string 
String::ToString(long n)
{
	string s;
	size_t size = sizeof(unsigned long) * 8;
	s.resize(size);
	string::size_type charPos = size;

	bool negative = false;
	if (n < 0)
	{
		n = -n;
		negative = true;
	}

	do
	{
		s[--charPos] = n % 10 + '0';
		n /= 10;
	}while (0 != n);

	if (negative)
	{
		s[--charPos] = '-';
	}

	return string(s, charPos, (size - charPos));
}

/// 将双精度浮点型数转换为字符串，转换结果中不包括十进制小数点和符号位
/// @param[in]		data				待转换的双精度浮点数。
/// @param[in]		precision			转换的字符串中包含小数点后几位。
/// @param[in]		data				小数点在返回的串中的位置(decimal point position)
/// @param[in]		sign				转换的数的符号。
std::string
doubleConvert(double data, int precision, int& decpos, bool& negative)
{
	decpos = 0;
	negative = false;

	string strret;
	size_t size = sizeof(double) * 8;
	strret.resize(size);
	string::size_type charPos = size;

	negative = false;
	if (data < 0)
	{
		data = -data;
		negative = true;
	}

	double intprt;
	data = modf(data, &intprt);

	if (0 != intprt)
	{
		while (0 != intprt)
		{
			double decmprt = modf(intprt / 10, &intprt);
			strret[--charPos] = static_cast<int>((decmprt + .03) * 10) + '0';
			++decpos;
		}
		strret.erase(0, charPos).resize(size);
	}
	else
	{
		while (data * 10 < 1)
		{
			data *= 10;
			--decpos;
		}
	}

	int strretlen = precision + decpos;

	if (strretlen < 0)
	{
		return "\0";
	}

	charPos = decpos > 0 ? decpos : 0;

	while (static_cast<int>(charPos) <= strretlen)		// 为了保证正确舍入，需多插入一个尾数
	{
		data *= 10;
		data = modf(data, &intprt);
		strret[charPos++] = static_cast<int>(intprt) + '0';
	}

	// 
	// 以下执行舍入操作
	// 
	strret[--charPos] += 5;		// 取最后一个尾数，检测其是否大于 5 ， 执行舍入
	while (precision > 0 && strret[charPos] > '9')
	{
		strret[charPos] = '0';
		if (charPos > 0)
		{
			++strret[--charPos];
		}
		else
		{
			//strret.insert(strret.begin(), '1');
			++decpos;
			strret[charPos] = '1';

			++strretlen;
			break;
		}
	}

	strret.erase(strretlen);

	return strret;
}

std::string 
String::ToString(double data, int precision)
{
#if 1
	//double intprt;
	//data = modf(data, &intprt);

	//if (0 != intprt)
	//{
	//	while (0 != intprt)
	//	{
	//		double decmprt = modf(intprt / 10, &intprt);
	//		++precision;
	//	}
	//}
	//string ret;
	//ret.resize(precision);
	//gcvt(data, precision, &ret[0]);	// ecvt(), fcvt()

	//return ret;

	int decpos = 0;
	bool isneg = false;
	string ret = doubleConvert(data, precision, decpos, isneg);


	if (decpos < 0)
	{
		ret.insert(0, -decpos, '0');
		ret.insert(0, 1, '.');
	}
	else
	{
		ret.insert(decpos, 1, '.');
	}

	if (isneg)
	{
		ret.insert(0, 1, '-');
	}

	return ret;

#else

	string strret;
	size_t size = sizeof(double) * 8;
	strret.reserve(size);
	//string::size_type charPos = size;

	if (data < 0)
	{
		data = -data;
		strret.push_back('-');
	}

	unsigned long integerpart;// = static_cast<unsigned long>(DoubleToLong(data));
	double intprt;
	double decmprt = modf(data, &intprt);
	integerpart = static_cast<unsigned long>(intprt);

	data -= integerpart;
	if (data < 0)
	{
		integerpart -= 1;
		data += 1;
	}

	strret += (ToString(integerpart) + '.');

	for (int i = 0; i < precision; ++i)
	{
		data *= 10;
	}

	unsigned long decimalpart = static_cast<unsigned long>(DoubleToLong(data));
	//data -= decimalpart;
	//if (data < 0)
	//{
	//	decimalpart -= 1;
	//	data += 1;
	//}
	strret += ToString(decimalpart);

	size_t len = strret.size();
	while ('0' == strret[--len])
	{
		strret[len] = '\0';
	}

	strret.erase(++len);

	return strret;

#endif
}

std::string 
String::ToExpString(double data, int precision)
{
	int decpos = 0;
	bool isneg = false;
	string ret = doubleConvert(data, precision, decpos, isneg);

	if (0 != decpos)
	{
		ret.insert(1, 1, '.');
		ret.insert(ret.end(), 1, 'E');
		ret += ToString(decpos - 1);
	}

	if (isneg)
	{
		ret.insert(0, 1, '-');
	}

	return ret;
}

bool 
String::IsNumber(const std::string& s, bool* isdecimal)
{
	const char* src(s.c_str());

	if ('-' == *src || '+' == *src)
	{
		++src;
	}

	if (!IsDigit(*src))
	{
		return false;
	}

	++src;

	while (IsDigit(*src))
	{
		++src;
	}

	isdecimal ? *isdecimal = false : 0/*do nothing*/;

	if ('.' == *src)
	{
		++src;
		if (!IsDigit(*src))
		{
			return false;
		}

		while (IsDigit(*src))
		{
			++src;
		}
		isdecimal ? *isdecimal = true : 0/*do nothing*/;
	}
	if ('e' == *src || 'E' == *src)
	{
		++src;
		if ('+' == *src || '-' == *src) 
		{
			++src;
		}

		if (!IsDigit(*src))
		{
			return false;
		}

		while (IsDigit(*src))
		{
			++src;
		}
		isdecimal ? *isdecimal = true : 0/*do nothing*/;
	}

	return 0 == *src;
}

int 
String::GetIntInString(const char* s, char** endptr, int base)
{
	//
	// Assume nothing will be there to convert for now
	//
	if (endptr)
	{
		*endptr = const_cast<char *>(s);
	}

	//
	// Skip leading none digit character
	//
	while (*s && '+' != *s && '-' != *s
		&& !isdigit(static_cast<unsigned char>(*s)))
	{
		++s;
	}

	//
	// Check for sign
	//
	int sign = 1;
	if (*s == '+')
	{
		++s;
	}
	else if (*s == '-')
	{
		sign = -1;
		++s;
	}

	//
	// Check that base is valid
	//
	if (base == 0)
	{
		if (*s == '0')
		{
			base = 8;
			++s;

			//
			// We have at least this zero
			//
			if (endptr)
			{
				*endptr = const_cast<char *>(s);
			}

			if (*s == 'x' || *s == 'X')
			{
				base = 16;
				++s;
			}
		}
		else
		{
			base = 10;
		}
	}
	else if (base < 2 || base > 36)
	{
		errno = EINVAL;
		return 0;
	}

	//
	// Check that we have something left to parse
	//
	if (*s == '\0')
	{
		//
		// We did not read any new digit so we don't update endptr
		//
		return 0;
	}

	int result = 0;
	bool overflow = false;
	bool digitFound = false;
	const string validDigits(allDigits.begin(), allDigits.begin() + base);
	while (*s && validDigits.find_first_of(toupper(static_cast<unsigned char>(*s))) != validDigits.npos)
	{   
		digitFound = true;
		if (!overflow)
		{
			int digit = digitVal[toupper(static_cast<unsigned char>(*s)) - '0'];
			assert(digit != 100);
			if (result < _I32_MAX / base)
			{
				result *= base;
				result += digit;
			}
			else if ((digit <= _I32_MAX % base) || (sign == -1 && digit == _I32_MAX % base + 1))
			{
				result *= base;
				result += digit;
			}
			else
			{
				overflow = true;
				result = sign == -1 ? _I32_MIN : _I32_MAX;
			}
		}
		++s;
	}

	if (overflow)
	{
		errno = ERANGE;
	}
	else
	{
		result *= sign;
	}

	if (digitFound && endptr != 0)
	{
		*endptr = const_cast<char *>(s);
	}

	return result;
}

const Util::Byte* 
String::FindStringInBuffer(Util::Byte* buffer, size_t buffsize, const std::string& strtosearch)
{
	if (buffsize < strtosearch.length()
		|| "" == strtosearch)
	{
		return NULL;
	}

	for (size_t i = 0; i < buffsize; ++i)
	{
		size_t curindex = i;
		const char *search = strtosearch.c_str();

		while (*search && curindex < buffsize && *search == buffer[curindex])
		{
			++curindex;
			++search;
		}

		if (!*search)
		{
			return buffer + i;
		}
	}

	return NULL;

	//
	// wrong!!!!! BUG manufacturing!!!!!!!!!!!
	// test FindStringInBuffer("ssstring", strlen("ssstring"), "sstring"); return NULL.
	//
	//for (size_t i = 0; i < buffsize; /* ... */)
	//{
	//	int matchlen = 0;

	//	const char *strTem = strtosearch.c_str();

	//	while (i < buffsize && '\0' != strTem)
	//	{
	//		if (buffer[i] == *strTem)
	//		{
	//			++strTem;
	//			++matchlen;
	//		}
	//		else if (matchlen > 0)
	//		{
	//			break;
	//		}

	//		++i;		// bug manufacturing.
	//	}

	//	if (matchlen == strtosearch.length())
	//	{
	//		return buffer + i - matchlen;
	//	}
	//}

	//return NULL;
}

string 
String::BytesToString(const Byte* src, size_t size)
{
	const Byte* end(src + size);
#if 0
	ostringstream s;
	for (; src != end; ++src)
	{
		s << setw(2) << setfill('0') << hex << static_cast<int>(*src);
	}

	return s.str();

#else

	static const char* toHex = "0123456789abcdef";

	string s;
	s.resize(size * 2);

	for (unsigned int i = 0 ; src != end; ++src, ++i)
	{
		s[i * 2] = toHex[(*src >> 4) & 0xf];
		s[i * 2 + 1] = toHex[*src & 0xf];
	}

	return s;
#endif
}

string
String::BytesToString(const ByteSeq& bytes)
{
	return BytesToString(&*bytes.begin(), bytes.size());

#if 0
	ostringstream s;
	for (ByteSeq::const_iterator p = bytes.begin(); p != bytes.end(); ++p)
	{
		s << setw(2) << setfill('0') << hex << static_cast<int>(*p);
	}

	return s.str();

	//#else

	static const char* toHex = "0123456789abcdef";

	string s;
	s.resize(bytes.size() * 2);

	for (unsigned int i = 0; i < bytes.size(); ++i)
	{
		s[i * 2] = toHex[(bytes[i] >> 4) & 0xf];
		s[i * 2 + 1] = toHex[bytes[i] & 0xf];
	}

	return s;
#endif

}

ByteSeq
String::StringToBytes(const string& str)
{
	ByteSeq bytes;
	bytes.reserve((str.size() + 1) / 2);

	for(unsigned int i = 0; i + 1 < str.size(); i += 2)
	{
		int byte = 0;
#if 0

		istringstream is(str.substr(i, 2));
		is >> hex >> byte;

#else
		for(int j = 0; j < 2; ++j)
		{
			char c = str[i + j];

			if(c >= '0' && c <= '9')
			{
				byte |= c - '0';
			}
			else if(c >= 'a' && c <= 'f')
			{
				byte |= 10 + c - 'a';
			}
			else if(c >= 'A' && c <= 'F')
			{
				byte |= 10 + c - 'A';
			}

			if(j == 0)
			{
				byte <<= 4;
			}
		}
#endif
		bytes.push_back(static_cast<Byte>(byte));
	}

	return bytes;
}

size_t 
String::HexDumpLine(const void* ptr, size_t offset, size_t size, std::string& line, size_t linelength)
{
	// Line layout:
	// 12: address
	// 1: space
	// (1+2)*linelength: hex bytes, each preceded by a space
	// 1: space separating the two halves
	// 4: "   |"
	// 1*linelength: characters
	// 1: "|"
	// Total: 19 + linelength * (1 + 2 + 1)
	line.clear();
	line.reserve(19 + linelength * (1 + 2 + 1));
	const Util::Byte* srcstr = reinterpret_cast<const Util::Byte*>(ptr) + offset;
	size_t length = std::min(size - offset, linelength);

	std::ostringstream out;
	out << std::hex << /*std::showbase << */std::uppercase;
	out << '[';
	out.width(8);
	out.fill('0');
	out << offset << "h]: ";

	for (size_t i = 0; i < length; ++i)
	{
		if (i == linelength / 2)
		{
			out << ' ';
		}

		out.width(2);
		out.fill('0');
		out << /*std::noshowbase << */(srcstr[i] & 0x00ff) << ' ';
	}

	// 3 spaces for each byte we're not printing, one separating the halves
	// if necessary
	out << string(3 * (linelength - length) + (length <= linelength / 2), ' ');
	out << "   |";
	line.append(out.str());

	for (size_t i = 0; i < length; i++) 
	{
		char c = (srcstr[i] >= 32 && srcstr[i] <= 126 ? static_cast<char>(srcstr[i]) : '.');
		//out << c;
		line.append(1, c);
	}
	//line.append(out.str());
	line.append(linelength - length, ' ');
	line.push_back('|');

	//out << string(linelength - length, ' ');
	//out << '|';
	//line.append(out.str());
	assert(line.size() == 19 + linelength * 4); 

	return length;
}

std::string 
String::HexDump(const void* ptr, size_t size, size_t linelength) 
{
	std::ostringstream os;
	HexDump(ptr, size, std::ostream_iterator<const char*>(os, "\n"), linelength);
	//HexDump(ptr, size, std::ostream_iterator<std::string>(os, "\n"), linelength);
	return os.str();
}

std::string 
String::HexStringToBuffer(const string &hexString, string &buffer, const std::string& delimiter)
{
	//
	//For the "C" locale, white-space characters are any of:
	//	' '		(0x20)	space (SPC)
	//	'\t'	(0x09)	horizontal tab (TAB)
	//	'\n'	(0x0a)	newline (LF)
	//	'\v'	(0x0b)	vertical tab (VT)
	//	'\f'	(0x0c)	feed (FF)
	//	'\r'	(0x0d)	carriage return (CR)
	//

	const string IFS(delimiter + " \t\r\n\v\f"); // Internal Field Separator(space is default separator).

	buffer.clear();
	string::const_pointer data = hexString.c_str();
	size_t pos(hexString.find("h]:"));	// skip address info if exist.
	if (string::npos != pos)
	{
		data += pos + 3;
	}

	while ('\0' != data)
	{
		//if (isspace(*data) || (/*ispunct(*data) && */',' == *data))
		if (string::npos != IFS.find(*data))
		{
			++data;
			continue;
		}
		else if (!isxdigit(*data) && string::npos == string("+-").find(*data))
		{
			break;
		}

		int sign = 1;
		if ('+' == *data)
		{
			++data;
		}
		else if ('-' == *data)
		{
			sign = -1;
			++data;
		}

		if ('\0' == data)
		{
			break;
		}

		if ('0' == *data && 'x' == tolower(*(data + 1)))
		{
			data += 2;
		}

#if 1
		string item;
		item.reserve(128);		// for performance
		while (*data && isxdigit(*data))
		{
			item.push_back(*data);
			++data;
		}

		if (0 != item.size() % 2)
		{
			item = '0' + item;
		}

		string::const_iterator citer = item.begin();

		while (item.end() != citer)
		{
			unsigned short ucdata;
			std::stringstream is(string(citer, citer + 2));
			is >> std::hex >> ucdata;
			//unsigned char ucdata;		//error
			//std::stringstream ostringstream(string(citer, citer + 2));
			//ostringstream >> std::hex >> ucdata;
			ucdata = (unsigned short)ToULong(string(citer, citer + 2), 0, 16);
			buffer += (char)(sign * ucdata);

			citer += 2;
		}

		//////////////////////////////////////////////////////////////////////////
#else	// bug manufacture
		//
		//string::const_pointer end = hexString.c_str() + hexString.size();
		//size_t len = end - data <= 2 ? end - data : 2;
		//string strData(data, len);
		//data += len;
		//if ("0x" == ToLower(strData))
		//{
		//	continue;
		//}
		//else
		//{
		//	unsigned char cdata;
		//	//std::stringstream ostringstream(strData);
		//	//ostringstream >> std::hex >> cdata;
		//	cdata = (unsigned char)ToULong(strData, 0, 16);
		//	buffer += (sign * cdata);
		//}
#endif
	}

	return buffer;
}

size_t
String::BinDumpLine(const void* ptr, size_t offset, size_t size, std::string& line, size_t linelength)
{
	// Line layout:
	// 12: address
	// 1: space
	// (1+8)*linelength: binary bytes, each preceded by a space
	// 1: space separating the two halves
	// 4: "   |"
	// 1*linelength: characters
	// 1: "|"
	// Total: 19 + linelength * (1 + 8 + 1)
	line.clear();
	line.reserve(19 + linelength * (1 + 8 + 1));
	const Util::Byte* srcstr = reinterpret_cast<const Util::Byte*>(ptr) + offset;
	size_t length = std::min(size - offset, linelength);

	std::ostringstream out;
	out << std::hex << /*std::showbase << */std::uppercase;
	out << '[';
	out.width(8);
	out.fill('0');
	out << offset << "h]: ";

	for (size_t i = 0; i < length; ++i)
	{
		if (i == linelength / 2)
		{
			out << ' ';
		}

		bitset<8> bits(srcstr[i]);
		out << bits.to_string() << ' ';
	}

	// 9 spaces for each byte we're not printing, one separating the halves
	// if necessary
	out << string(9 * (linelength - length) + (length <= linelength / 2), ' ');
	out << "   |";
	line.append(out.str());

	for (size_t i = 0; i < length; i++) 
	{
		char c = (srcstr[i] >= 32 && srcstr[i] <= 126 ? static_cast<char>(srcstr[i]) : '.');
		//out << c;
		line.append(1, c);
	}
	//line.append(out.str());
	line.append(linelength - length, ' ');
	line.push_back('|');

	//out << string(linelength - length, ' ');
	//out << '|';
	//line.append(out.str());
	assert(line.size() == 19 + linelength * 10); 

	return length;
}

std::string 
String::BinDump(const void* ptr, size_t size, size_t linelength) 
{
	std::ostringstream os;
	BinDump(ptr, size, std::ostream_iterator<const char*>(os, "\n"), linelength);
	//BinDump(ptr, size, std::ostream_iterator<std::string>(os, "\n"), linelength);
	return os.str();
}

// If *pstr starts with the given prefix, modifies *pstr to be right
// past the prefix and returns true; otherwise leaves *pstr unchanged
// and returns false.  None of pstr, *pstr, and prefix can be NULL.
bool 
String::SkipPrefix(const char* prefix, const char** pstr) 
{
	const size_t prefix_len = strlen(prefix);
	if (strncmp(*pstr, prefix, prefix_len) == 0) 
	{
		*pstr += prefix_len;
		return true;
	}
	return false;
}

// Formats an int value as "%02d".
std::string FormatIntWidth2(int value) 
{
	std::stringstream ss;
	ss << std::setfill('0') << std::setw(2) << value;
	return ss.str();
}

// Formats an int value as "%X".
std::string FormatHexInt(int value) 
{
	std::stringstream ss;
	ss << std::hex << std::uppercase << value;
	return ss.str();
}

// Formats a byte as "%02X".
std::string FormatByte(unsigned char value) 
{
	std::stringstream ss;
	ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase
		<< static_cast<unsigned int>(value);
	return ss.str();
}

// Converts the buffer in a stringstream to an std::string, converting NUL
// bytes to "\\0" along the way.
std::string StringStreamToString(::std::stringstream* ss) 
{
	const ::std::string& str = ss->str();
	const char* const start = str.c_str();
	const char* const end = start + str.length();

	std::string result;
	result.reserve(2 * (end - start));
	for (const char* ch = start; ch != end; ++ch) 
	{
		if (*ch == '\0') 
		{
			result += "\\0";  // Replaces NUL with "\\0";
		} 
		else 
		{
			result += *ch;
		}
	}

	return result;
}

// Utility functions for encoding Unicode text (wide strings) in
// UTF-8.

// A Unicode code-point can have upto 21 bits, and is encoded in UTF-8
// like this:
//
// Code-point length   Encoding
//   0 -  7 bits       0xxxxxxx
//   8 - 11 bits       110xxxxx 10xxxxxx
//  12 - 16 bits       1110xxxx 10xxxxxx 10xxxxxx
//  17 - 21 bits       11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

// The maximum code-point a one-byte UTF-8 sequence can represent.
const unsigned int kMaxCodePoint1 = (static_cast<unsigned int>(1) <<  7) - 1;

// The maximum code-point a two-byte UTF-8 sequence can represent.
const unsigned int kMaxCodePoint2 = (static_cast<unsigned int>(1) << (5 + 6)) - 1;

// The maximum code-point a three-byte UTF-8 sequence can represent.
const unsigned int kMaxCodePoint3 = (static_cast<unsigned int>(1) << (4 + 2*6)) - 1;

// The maximum code-point a four-byte UTF-8 sequence can represent.
const unsigned int kMaxCodePoint4 = (static_cast<unsigned int>(1) << (3 + 3*6)) - 1;

// Chops off the n lowest bits from a bit pattern.  Returns the n
// lowest bits.  As a side effect, the original bit pattern will be
// shifted to the right by n bits.
inline unsigned int ChopLowBits(unsigned int* bits, int n) 
{
	const unsigned int low_bits = *bits & ((static_cast<unsigned int>(1) << n) - 1);
	*bits >>= n;
	return low_bits;
}

// Converts a Unicode code point to a narrow string in UTF-8 encoding.
// code_point parameter is of type unsigned int because wchar_t may not be
// wide enough to contain a code point.
// If the code_point is not a valid Unicode code point
// (i.e. outside of Unicode range U+0 to U+10FFFF) it will be converted
// to "(Invalid Unicode 0xXXXXXXXX)".
std::string CodePointToUtf8(unsigned int code_point)
{
	if (code_point > kMaxCodePoint4) 
	{
		return "(Invalid Unicode 0x" + FormatHexInt(code_point) + ")";
	}

	char str[5];  // Big enough for the largest valid code point.
	if (code_point <= kMaxCodePoint1) 
	{
		str[1] = '\0';
		str[0] = static_cast<char>(code_point);                          // 0xxxxxxx
	}
	else if (code_point <= kMaxCodePoint2) 
	{
		str[2] = '\0';
		str[1] = static_cast<char>(0x80 | ChopLowBits(&code_point, 6));  // 10xxxxxx
		str[0] = static_cast<char>(0xC0 | code_point);                   // 110xxxxx
	}
	else if (code_point <= kMaxCodePoint3)
	{
		str[3] = '\0';
		str[2] = static_cast<char>(0x80 | ChopLowBits(&code_point, 6));  // 10xxxxxx
		str[1] = static_cast<char>(0x80 | ChopLowBits(&code_point, 6));  // 10xxxxxx
		str[0] = static_cast<char>(0xE0 | code_point);                   // 1110xxxx
	}
	else // code_point <= kMaxCodePoint4
	{  
		str[4] = '\0';
		str[3] = static_cast<char>(0x80 | ChopLowBits(&code_point, 6));  // 10xxxxxx
		str[2] = static_cast<char>(0x80 | ChopLowBits(&code_point, 6));  // 10xxxxxx
		str[1] = static_cast<char>(0x80 | ChopLowBits(&code_point, 6));  // 10xxxxxx
		str[0] = static_cast<char>(0xF0 | code_point);                   // 11110xxx
	}
	return str;
}

// The following two functions only make sense if the the system
// uses UTF-16 for wide string encoding. All supported systems
// with 16 bit wchar_t (Windows, Cygwin, Symbian OS) do use UTF-16.

// Determines if the arguments constitute UTF-16 surrogate pair
// and thus should be combined into a single Unicode code point
// using CreateCodePointFromUtf16SurrogatePair.
inline bool IsUtf16SurrogatePair(wchar_t first, wchar_t second) 
{
	return sizeof(wchar_t) == 2 &&
		(first & 0xFC00) == 0xD800 && (second & 0xFC00) == 0xDC00;
}

// Creates a Unicode code point from UTF16 surrogate pair.
inline unsigned int CreateCodePointFromUtf16SurrogatePair(wchar_t first,
														  wchar_t second) 
{
	const unsigned int mask = (1 << 10) - 1;

	return (sizeof(wchar_t) == 2) ?
		(((first & mask) << 10) | (second & mask)) + 0x10000 :
	// This function should not be called when the condition is
	// false, but we provide a sensible default in case it is.
	static_cast<unsigned int>(first);
}

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
std::string WideStringToUtf8(const wchar_t* str, int num_chars)
{
	if (-1 == num_chars)
	{
		num_chars = static_cast<int>(wcslen(str));
	}

	::std::stringstream stream;
	for (int i = 0; i < num_chars; ++i)
	{
		unsigned int unicode_code_point;

		if (str[i] == L'\0') 
		{
			break;
		} 
		else if (i + 1 < num_chars && IsUtf16SurrogatePair(str[i], str[i + 1]))
		{
			unicode_code_point = 
				CreateCodePointFromUtf16SurrogatePair(str[i], str[i + 1]);
			i++;
		} 
		else 
		{
			unicode_code_point = static_cast<unsigned int>(str[i]);
		}

		stream << CodePointToUtf8(unicode_code_point);
	}
	return StringStreamToString(&stream);
}

// Converts a wide C string to an std::string using the UTF-8 encoding.
// NULL will be converted to "(null)".
std::string ShowWideCString(const wchar_t * wide_c_str)
{
	if (wide_c_str == NULL) 
	{
		return "(null)";
	}

	return WideStringToUtf8(wide_c_str, -1);
}

// Compares two wide C strings.  Returns true iff they have the same
// content.
//
// Unlike wcscmp(), this function can handle NULL argument(s).  A NULL
// C string is considered different to any non-NULL C string,
// including the empty string.
bool WideCStringEquals(const wchar_t * lhs, const wchar_t * rhs) 
{
	if (NULL == lhs) 
	{
		return rhs == NULL;
	}

	if (NULL == rhs) 
	{
		return false;
	}

	return 0 == wcscmp(lhs, rhs);
}

// Compares two C strings.  Returns true iff they have the same content.
//
// Unlike strcmp(), this function can handle NULL argument(s).  A NULL
// C string is considered different to any non-NULL C string,
// including the empty string.
bool CStringEquals(const char * lhs, const char * rhs)
{
	if (NULL == lhs)
	{
		return rhs == NULL;
	}

	if (NULL == rhs) 
	{
		return false;
	}

	return 0 == strcmp(lhs, rhs);
}

// Compares two C strings, ignoring case.  Returns true iff they have
// the same content.
//
// Unlike strcasecmp(), this function can handle NULL argument(s).  A
// NULL C string is considered different to any non-NULL C string,
// including the empty string.
bool CaseInsensitiveCStringEquals(const char * lhs, const char * rhs) 
{
	if (NULL == lhs)
	{
		return rhs == NULL;
	}
	if (NULL == rhs)
	{
		return false;
	}
	return 0 == stricmp(lhs, rhs);
}

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
bool CaseInsensitiveWideCStringEquals(const wchar_t* lhs, const wchar_t* rhs)
{
	if (NULL == lhs)
	{
		return rhs == NULL;
	}

	if (NULL == rhs) 
	{
		return false;
	}

#ifdef _WIN32
	return _wcsicmp(lhs, rhs) == 0;
#elif defined (__linux__) && !defined(__ANDROID__)
	return wcscasecmp(lhs, rhs) == 0;
#else
	// Android, Mac OS X and Cygwin don't define wcscasecmp.
	// Other unknown OSes may not define it either.
	wint_t left, right;
	do {
		left = towlower(*lhs++);
		right = towlower(*rhs++);
	} while (left && left == right);
	return left == right;
#endif  // OS selector
}

// Returns true iff str ends with the given suffix, ignoring case.
// Any string is considered to end with an empty suffix.
bool EndsWithCaseInsensitive(const std::string& str, const std::string& suffix)
{
	const size_t str_len = str.length();
	const size_t suffix_len = suffix.length();
	return (str_len >= suffix_len) &&
		CaseInsensitiveCStringEquals(str.c_str() + str_len - suffix_len,
		suffix.c_str());
}

//////////////////////////////////////////////////////////////////////////
/// string format
static string 
formatImpl( const char* format, va_list arglist)
{
	//vsnprintf在X64环境下会更改arglist的值，因此需要复制一份
	va_list tmpvarlist = arglist;

	if (NULL == format)
	{
		return "";
	}

	//_vscprintf(format, args) // _vscprintf doesn't count

	// MSVC 8 deprecates vsnprintf(), so we want to suppress warning
	// 4996 (deprecated function) there.
#ifdef _MSC_VER  // We are using MSVC.
# pragma warning(push)          // Saves the current warning state.
# pragma warning(disable:4996)  // Temporarily disables warning 4996.

	const int size = vsnprintf(NULL, 0, format, tmpvarlist);

# pragma warning(pop)           // Restores the warning state.
#else  // We are not using MSVC.
	const int size = vsnprintf(NULL, 0, format, tmpvarlist);
#endif  // _MSC_VER

	if(size < 0)
	{
		return "";
	}

	string str;
	str.resize(size);
	vsprintf((char*)str.c_str(), format, arglist);

	return str;
}

string 
Format( const char* format, ...)
{
	if (NULL == format)
	{
		return "";
	}

	va_list varList;	//变长参数列表首地址
	va_start(varList, format);
	string str = formatImpl(format, varList);
	va_end(varList);

	return str;
}

double 
String2Double(const string& str)
{
	string value(str);
	double result = 0;
	double sign = 1;
	int scale = 0;
	int exponent = 0;
	int expsign = 1;
	int j = 0;
	int jMax = (int) value.length();
	if (jMax > 0)
	{
		if ('+' == value[j])
		{
			j++;
		}
		else if ('-' == value[j])
		{
			sign = -1;
			j++;
		}
		while (j < jMax && IsDigit(value[j]))
		{
			result = result * 10 + (value[j] - '0');
			j++;
		}
		if (j < jMax && value[j] == '.')
		{
			j++;
			while (j < jMax && IsDigit(value[j]))
			{
				result = result*10 + (value[j] - '0');
				scale++;
				j++;
			}
		}
		if (j < jMax && (value[j] == 'E' || value[j] == 'e'))
		{
			j++;
			if ('+' == value[j])
			{
				j++;
			}
			else if ('-' == value[j])
			{
				expsign = -1;
				j++;
			}
			while (j < jMax && IsDigit(value[j]))
			{
				exponent = exponent*10 + (value[j] - '0');
				j++;
			}
			exponent *= expsign;
		}
		result = sign * result * pow(10.0, exponent-scale);
	}
	return result;
}

string 
Double2String(double value, int precision)
{
	string number;
	if (precision < 0)
	{
		precision = 0;
	}
	else if (precision > 16)
	{
		precision = 16;
	}

	// Use absolute value locally
	double localValue = fabs(value);
	double localFraction = (localValue - floor(localValue)) +(5. * pow(10.0, -precision-1));
	if (localFraction >= 1)
	{
		localValue += 1.0;
		localFraction -= 1.0;
	}
	localFraction *= pow(10.0, precision);

	if (value < 0)
	{
		number += "-";
	}

	number += Format("%.0f", floor(localValue));

	// generate fraction, padding with zero if necessary.
	if (precision > 0)
	{
		number += ".";
		string fraction = Format("%.0f", floor(localFraction));
		if (fraction.length() < ((size_t) precision))
		{
			number += string(precision - fraction.length(), '0');
		}
		number += fraction;
	}

	return number;
}

bool
IsAlpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool
IsDigit(char c)
{
	return c >= '0' && c <= '9';
}

#if 0

// ======================================================================
//
// extern functions
//

// These are defined as macros on some platforms.  #undef them so that we can
// redefine them.
#undef isxdigit
#undef isprint

// The definitions of these in ctype.h change based on locale.  Since our
// string manipulation is all in relation to the protocol buffer and C++
// languages, we always want to use the C locale.  So, we re-define these
// exactly as we want them.
inline bool isxdigit(char c) 
{
	return ('0' <= c && c <= '9') 
		|| ('a' <= c && c <= 'f') 
		|| ('A' <= c && c <= 'F');
}

inline bool isprint(char c)
{
	return c >= 0x20 && c <= 0x7E;
}

// ----------------------------------------------------------------------
// StripString
//    Replaces any occurrence of the character 'remove' (or the characters
//    in 'remove') with the character 'replacewith'.
// ----------------------------------------------------------------------
void StripString(string* s, const char* remove, char replacewith)
{
	const char * str_start = s->c_str();
	const char * str = str_start;
	for (str = strpbrk(str, remove);
		str != NULL;
		str = strpbrk(str + 1, remove))
	{
		(*s)[str - str_start] = replacewith;
	}
}

// ----------------------------------------------------------------------
// StringReplace()
//    Replace the "old" pattern with the "new" pattern in a string,
//    and append the result to "res".  If replace_all is false,
//    it only replaces the first instance of "old."
// ----------------------------------------------------------------------

void StringReplace(const string& s, const string& oldsub,
				   const string& newsub, bool replace_all,
				   string* res)
{
	if (oldsub.empty()) 
	{
		res->append(s);  // if empty, append the given string.
		return;
	}

	string::size_type start_pos = 0;
	string::size_type pos;
	do {
		pos = s.find(oldsub, start_pos);
		if (pos == string::npos) 
		{
			break;
		}
		res->append(s, start_pos, pos - start_pos);
		res->append(newsub);
		start_pos = pos + oldsub.size();  // start searching again after the "old"
	} while (replace_all);
	res->append(s, start_pos, s.length() - start_pos);
}

// ----------------------------------------------------------------------
// StringReplace()
//    Give me a string and two patterns "old" and "new", and I replace
//    the first instance of "old" in the string with "new", if it
//    exists.  If "global" is true; call this repeatedly until it
//    fails.  RETURN a new string, regardless of whether the replacement
//    happened or not.
// ----------------------------------------------------------------------

string StringReplace(const string& s, const string& oldsub,
					 const string& newsub, bool replace_all) 
{
	string ret;
	StringReplace(s, oldsub, newsub, replace_all, &ret);
	return ret;
}

// ----------------------------------------------------------------------
// SplitStringUsing()
//    Split a string using a character delimiter. Append the components
//    to 'result'.
//
// Note: For multi-character delimiters, this routine will split on *ANY* of
// the characters in the string, not the entire string as a single delimiter.
// ----------------------------------------------------------------------
template <typename ITR>
static inline
void SplitStringToIteratorUsing(const string& full,
								const char* delim,
								ITR& result)
{
	// Optimize the common case where delim is a single character.
	if (delim[0] != '\0' && delim[1] == '\0')
	{
		char c = delim[0];
		const char* p = full.data();
		const char* end = p + full.size();
		while (p != end) 
		{
			if (*p == c)
			{
				++p;
			}
			else
			{
				const char* start = p;
				while (++p != end && *p != c);
				*result++ = string(start, p - start);
			}
		}
		return;
	}

	string::size_type begin_index, end_index;
	begin_index = full.find_first_not_of(delim);
	while (begin_index != string::npos) 
	{
		end_index = full.find_first_of(delim, begin_index);
		if (end_index == string::npos)
		{
			*result++ = full.substr(begin_index);
			return;
		}
		*result++ = full.substr(begin_index, (end_index - begin_index));
		begin_index = full.find_first_not_of(delim, end_index);
	}
}

void SplitStringUsing(const string& full,
					  const char* delim,
					  vector<string>* result) 
{
	back_insert_iterator< vector<string> > it(*result);
	SplitStringToIteratorUsing(full, delim, it);
}

// Split a string using a character delimiter. Append the components
// to 'result'.  If there are consecutive delimiters, this function
// will return corresponding empty strings. The string is split into
// at most the specified number of pieces greedily. This means that the
// last piece may possibly be split further. To split into as many pieces
// as possible, specify 0 as the number of pieces.
//
// If "full" is the empty string, yields an empty string as the only value.
//
// If "pieces" is negative for some reason, it returns the whole string
// ----------------------------------------------------------------------
template <typename StringType, typename ITR>
static inline
void SplitStringToIteratorAllowEmpty(const StringType& full,
									 const char* delim,
									 int pieces,
									 ITR& result)
{
	string::size_type begin_index, end_index;
	begin_index = 0;

	for (int i = 0; (i < pieces-1) || (pieces == 0); i++)
	{
		end_index = full.find_first_of(delim, begin_index);
		if (end_index == string::npos) 
		{
			*result++ = full.substr(begin_index);
			return;
		}
		*result++ = full.substr(begin_index, (end_index - begin_index));
		begin_index = end_index + 1;
	}
	*result++ = full.substr(begin_index);
}

void SplitStringAllowEmpty(const string& full, const char* delim,
						   vector<string>* result)
{
	back_insert_iterator<vector<string> > it(*result);
	SplitStringToIteratorAllowEmpty(full, delim, 0, it);
}

// ----------------------------------------------------------------------
// JoinStrings()
//    This merges a vector of string components with delim inserted
//    as separaters between components.
//
// ----------------------------------------------------------------------
template <class ITERATOR>
static void JoinStringsIterator(const ITERATOR& start,
								const ITERATOR& end,
								const char* delim,
								string* result)
{
	UTIL_CHECK(result != NULL);
	result->clear();
	int delim_length = strlen(delim);

	// Precompute resulting length so we can reserve() memory in one shot.
	int length = 0;
	for (ITERATOR iter = start; iter != end; ++iter)
	{
		if (iter != start)
		{
			length += delim_length;
		}
		length += iter->size();
	}
	result->reserve(length);

	// Now combine everything.
	for (ITERATOR iter = start; iter != end; ++iter)
	{
		if (iter != start)
		{
			result->append(delim, delim_length);
		}
		result->append(iter->data(), iter->size());
	}
}

void JoinStrings(const vector<string>& components,
				 const char* delim,
				 string * result) 
{
	JoinStringsIterator(components.begin(), components.end(), delim, result);
}

// ----------------------------------------------------------------------
// UnescapeCEscapeSequences()
//    This does all the unescaping that C does: \ooo, \r, \n, etc
//    Returns length of resulting string.
//    The implementation of \x parses any positive number of hex digits,
//    but it is an error if the value requires more than 8 bits, and the
//    result is truncated to 8 bits.
//
//    The second call stores its errors in a supplied string vector.
//    If the string vector pointer is NULL, it reports the errors with LOG().
// ----------------------------------------------------------------------

#define IS_OCTAL_DIGIT(c) (((c) >= '0') && ((c) <= '7'))

inline int hex_digit_to_int(char c) 
{
	/* Assume ASCII. */
	assert('0' == 0x30 && 'A' == 0x41 && 'a' == 0x61);
	assert(isxdigit(c));
	int x = static_cast<unsigned char>(c);
	if (x > '9') 
	{
		x += 9;
	}
	return x & 0xf;
}

#define LOG_STRING(LEVEL, VECTOR) UTIL_LOG_IF(LEVEL, false)

int UnescapeCEscapeSequences(const char* source, char* dest)
{
	return UnescapeCEscapeSequences(source, dest, NULL);
}

int UnescapeCEscapeSequences(const char* source, char* dest,
							 vector<string> *errors)
{
	UTIL_DCHECK(errors == NULL) << "Error reporting not implemented.";

	char* d = dest;
	const char* p = source;

	// Small optimization for case where source = dest and there's no escaping
	while ( p == d && *p != '\0' && *p != '\\' )
	{
		p++, d++;
	}

	while (*p != '\0') 
	{
		if (*p != '\\')
		{
			*d++ = *p++;
		}
		else 
		{
			switch ( *++p )
			{                    // skip past the '\\'
			case '\0':
				LOG_STRING(ERROR, errors) << "String cannot end with \\";
				*d = '\0';
				return d - dest;   // we're done with p
			case 'a':  *d++ = '\a';  break;
			case 'b':  *d++ = '\b';  break;
			case 'f':  *d++ = '\f';  break;
			case 'n':  *d++ = '\n';  break;
			case 'r':  *d++ = '\r';  break;
			case 't':  *d++ = '\t';  break;
			case 'v':  *d++ = '\v';  break;
			case '\\': *d++ = '\\';  break;
			case '?':  *d++ = '\?';  break;    // \?  Who knew?
			case '\'': *d++ = '\'';  break;
			case '"':  *d++ = '\"';  break;
			case '0': case '1': case '2': case '3':  // octal digit: 1 to 3 digits
			case '4': case '5': case '6': case '7':
				{
					char ch = *p - '0';
					if ( IS_OCTAL_DIGIT(p[1]) )
						ch = ch * 8 + *++p - '0';
					if ( IS_OCTAL_DIGIT(p[1]) )      // safe (and easy) to do this twice
						ch = ch * 8 + *++p - '0';      // now points at last digit
					*d++ = ch;
					break;
				}
			case 'x': case 'X': 
				{
					if (!isxdigit(p[1]))
					{
						if (p[1] == '\0')
						{
							LOG_STRING(ERROR, errors) << "String cannot end with \\x";
						}
						else
						{
							LOG_STRING(ERROR, errors) <<
								"\\x cannot be followed by non-hex digit: \\" << *p << p[1];
						}
						break;
					}
					unsigned int ch = 0;
					const char *hex_start = p;
					while (isxdigit(p[1])) 
					{
						// arbitrarily many hex digits
						ch = (ch << 4) + hex_digit_to_int(*++p);
					}
					if (ch > 0xFF)
					{
						LOG_STRING(ERROR, errors) << "Value of " <<
							"\\" << string(hex_start, p+1-hex_start) << " exceeds 8 bits";
					}
					*d++ = ch;
					break;
				}
#if 0  // TODO(kenton):  Support \u and \U?  Requires runetochar().
			case 'u': 
				{
					// \uhhhh => convert 4 hex digits to UTF-8
					char32 rune = 0;
					const char *hex_start = p;
					for (int i = 0; i < 4; ++i) 
					{
						if (isxdigit(p[1])) 
						{  
							// Look one char ahead.
							rune = (rune << 4) + hex_digit_to_int(*++p);  // Advance p.
						} 
						else 
						{
							LOG_STRING(ERROR, errors)
								<< "\\u must be followed by 4 hex digits: \\"
								<<  string(hex_start, p+1-hex_start);
							break;
						}
					}
					d += runetochar(d, &rune);
					break;
				}
			case 'U': 
				{
					// \Uhhhhhhhh => convert 8 hex digits to UTF-8
					char32 rune = 0;
					const char *hex_start = p;
					for (int i = 0; i < 8; ++i)
					{
						if (isxdigit(p[1]))
						{  // Look one char ahead.
							// Don't change rune until we're sure this
							// is within the Unicode limit, but do advance p.
							char32 newrune = (rune << 4) + hex_digit_to_int(*++p);
							if (newrune > 0x10FFFF) 
							{
								LOG_STRING(ERROR, errors)
									<< "Value of \\"
									<< string(hex_start, p + 1 - hex_start)
									<< " exceeds Unicode limit (0x10FFFF)";
								break;
							}
							else
							{
								rune = newrune;
							}
						}
						else 
						{
							LOG_STRING(ERROR, errors)
								<< "\\U must be followed by 8 hex digits: \\"
								<<  string(hex_start, p+1-hex_start);
							break;
						}
					}
					d += runetochar(d, &rune);
					break;
				}
#endif
			default:
				LOG_STRING(ERROR, errors) << "Unknown escape sequence: \\" << *p;
			}
			p++;                                 // read past letter we escaped
		}
	}
	*d = '\0';
	return d - dest;
}

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
int UnescapeCEscapeString(const string& src, string* dest)
{
	return UnescapeCEscapeString(src, dest, NULL);
}

int UnescapeCEscapeString(const string& src, string* dest,
						  vector<string> *errors) 
{
	ScopedArray<char> unescaped(new char[src.size() + 1]);
	int len = UnescapeCEscapeSequences(src.c_str(), unescaped.Get(), errors);
	UTIL_CHECK(dest);
	dest->assign(unescaped.Get(), len);
	return len;
}

string UnescapeCEscapeString(const string& src) 
{
	ScopedArray<char> unescaped(new char[src.size() + 1]);
	int len = UnescapeCEscapeSequences(src.c_str(), unescaped.Get(), NULL);
	return string(unescaped.Get(), len);
}

// ----------------------------------------------------------------------
// CEscapeString()
// CHexEscapeString()
//    Copies 'src' to 'dest', escaping dangerous characters using
//    C-style escape sequences. This is very useful for preparing query
//    flags. 'src' and 'dest' should not overlap. The 'Hex' version uses
//    hexadecimal rather than octal sequences.
//    Returns the number of bytes written to 'dest' (not including the \0)
//    or -1 if there was insufficient space.
//
//    Currently only \n, \r, \t, ", ', \ and !isprint() chars are escaped.
// ----------------------------------------------------------------------
static int CEscapeInternal(const char* src, int src_len, char* dest,
					int dest_len, bool use_hex, bool utf8_safe)
{
	const char* src_end = src + src_len;
	int used = 0;
	bool last_hex_escape = false; // true if last output char was \xNN

	for (; src < src_end; src++)
	{
		if (dest_len - used < 2)   // Need space for two letter escape
			return -1;

		bool is_hex_escape = false;
		switch (*src) 
		{
		case '\n': dest[used++] = '\\'; dest[used++] = 'n';  break;
		case '\r': dest[used++] = '\\'; dest[used++] = 'r';  break;
		case '\t': dest[used++] = '\\'; dest[used++] = 't';  break;
		case '\"': dest[used++] = '\\'; dest[used++] = '\"'; break;
		case '\'': dest[used++] = '\\'; dest[used++] = '\''; break;
		case '\\': dest[used++] = '\\'; dest[used++] = '\\'; break;
		default:
			// Note that if we emit \xNN and the src character after that is a hex
			// digit then that digit must be escaped too to prevent it being
			// interpreted as part of the character code by C.
			if ((!utf8_safe || static_cast<uint8>(*src) < 0x80) &&
				(!isprint(*src) || (last_hex_escape && isxdigit(*src))))
			{
				if (dest_len - used < 4) // need space for 4 letter escape
					return -1;
				sprintf(dest + used, (use_hex ? "\\x%02x" : "\\%03o"),
					static_cast<uint8>(*src));
				is_hex_escape = use_hex;
				used += 4;
			}
			else 
			{
				dest[used++] = *src; break;
			}
		}
		last_hex_escape = is_hex_escape;
	}

	if (dest_len - used < 1)   // make sure that there is room for \0
		return -1;

	dest[used] = '\0';   // doesn't count towards return value though
	return used;
}

int CEscapeString(const char* src, int src_len, char* dest, int dest_len)
{
	return CEscapeInternal(src, src_len, dest, dest_len, false, false);
}

// ----------------------------------------------------------------------
// CEscape()
// CHexEscape()
//    Copies 'src' to result, escaping dangerous characters using
//    C-style escape sequences. This is very useful for preparing query
//    flags. 'src' and 'dest' should not overlap. The 'Hex' version
//    hexadecimal rather than octal sequences.
//
//    Currently only \n, \r, \t, ", ', \ and !isprint() chars are escaped.
// ----------------------------------------------------------------------
string CEscape(const string& src)
{
	const int dest_length = src.size() * 4 + 1; // Maximum possible expansion
	ScopedArray<char> dest(new char[dest_length]);
	const int len = CEscapeInternal(src.data(), src.size(),
		dest.Get(), dest_length, false, false);
	UTIL_DCHECK_GE(len, 0);
	return string(dest.Get(), len);
}

string Utf8SafeCEscape(const string& src)
{
	const int dest_length = src.size() * 4 + 1; // Maximum possible expansion
	ScopedArray<char> dest(new char[dest_length]);
	const int len = CEscapeInternal(src.data(), src.size(),
		dest.Get(), dest_length, false, true);
	UTIL_DCHECK_GE(len, 0);
	return string(dest.Get(), len);
}

string CHexEscape(const string& src) 
{
	const int dest_length = src.size() * 4 + 1; // Maximum possible expansion
	ScopedArray<char> dest(new char[dest_length]);
	const int len = CEscapeInternal(src.data(), src.size(),
		dest.Get(), dest_length, true, false);
	UTIL_DCHECK_GE(len, 0);
	return string(dest.Get(), len);
}
// ----------------------------------------------------------------------
// strto32_adaptor()
// strtou32_adaptor()
//    Implementation of strto[u]l replacements that have identical
//    overflow and underflow characteristics for both ILP-32 and LP-64
//    platforms, including errno preservation in error-free calls.
// ----------------------------------------------------------------------

int32 strto32_adaptor(const char *nptr, char **endptr, int base)
{
	const int saved_errno = errno;
	errno = 0;
	const long result = strtol(nptr, endptr, base);
	if (errno == ERANGE && result == LONG_MIN)
	{
		return kint32min;
	}
	else if (errno == ERANGE && result == LONG_MAX) 
	{
		return kint32max;
	} 
	else if (errno == 0 && result < kint32min)
	{
		errno = ERANGE;
		return kint32min;
	}
	else if (errno == 0 && result > kint32max) 
	{
		errno = ERANGE;
		return kint32max;
	}

	if (errno == 0)
	{
		errno = saved_errno;
	}
	return static_cast<int32>(result);
}

uint32 strtou32_adaptor(const char *nptr, char **endptr, int base)
{
	const int saved_errno = errno;
	errno = 0;
	const unsigned long result = strtoul(nptr, endptr, base);
	if (errno == ERANGE && result == ULONG_MAX)
	{
		return kuint32max;
	}
	else if (errno == 0 && result > kuint32max) 
	{
		errno = ERANGE;
		return kuint32max;
	}
	if (errno == 0)
	{
		errno = saved_errno;
	}
	return static_cast<uint32>(result);
}

// ----------------------------------------------------------------------
// FastIntToBuffer()
// FastInt64ToBuffer()
// FastHexToBuffer()
// FastHex64ToBuffer()
// FastHex32ToBuffer()
// ----------------------------------------------------------------------

// Offset into buffer where FastInt64ToBuffer places the end of string
// null character.  Also used by FastInt64ToBufferLeft.
static const int kFastInt64ToBufferOffset = 21;

char *FastInt64ToBuffer(int64 i, char* buffer) 
{
	// We could collapse the positive and negative sections, but that
	// would be slightly slower for positive numbers...
	// 22 bytes is enough to store -2**64, -18446744073709551616.
	char* p = buffer + kFastInt64ToBufferOffset;
	*p-- = '\0';
	if (i >= 0) 
	{
		do {
			*p-- = '0' + i % 10;
			i /= 10;
		} while (i > 0);
		return p + 1;
	}
	else 
	{
		// On different platforms, % and / have different behaviors for
		// negative numbers, so we need to jump through hoops to make sure
		// we don't divide negative numbers.
		if (i > -10) 
		{
			i = -i;
			*p-- = '0' + i;
			*p = '-';
			return p;
		} 
		else 
		{
			// Make sure we aren't at MIN_INT, in which case we can't say i = -i
			i = i + 10;
			i = -i;
			*p-- = '0' + i % 10;
			// Undo what we did a moment ago
			i = i / 10 + 1;
			do {
				*p-- = '0' + i % 10;
				i /= 10;
			} while (i > 0);
			*p = '-';
			return p;
		}
	}
}

// Offset into buffer where FastInt32ToBuffer places the end of string
// null character.  Also used by FastInt32ToBufferLeft
static const int kFastInt32ToBufferOffset = 11;

// Yes, this is a duplicate of FastInt64ToBuffer.  But, we need this for the
// compiler to generate 32 bit arithmetic instructions.  It's much faster, at
// least with 32 bit binaries.
char *FastInt32ToBuffer(int32 i, char* buffer)
{
	// We could collapse the positive and negative sections, but that
	// would be slightly slower for positive numbers...
	// 12 bytes is enough to store -2**32, -4294967296.
	char* p = buffer + kFastInt32ToBufferOffset;
	*p-- = '\0';
	if (i >= 0)
	{
		do {
			*p-- = '0' + i % 10;
			i /= 10;
		} while (i > 0);
		return p + 1;
	}
	else
	{
		// On different platforms, % and / have different behaviors for
		// negative numbers, so we need to jump through hoops to make sure
		// we don't divide negative numbers.
		if (i > -10)
		{
			i = -i;
			*p-- = '0' + i;
			*p = '-';
			return p;
		} 
		else 
		{
			// Make sure we aren't at MIN_INT, in which case we can't say i = -i
			i = i + 10;
			i = -i;
			*p-- = '0' + i % 10;
			// Undo what we did a moment ago
			i = i / 10 + 1;
			do {
				*p-- = '0' + i % 10;
				i /= 10;
			} while (i > 0);
			*p = '-';
			return p;
		}
	}
}

char *FastHexToBuffer(int i, char* buffer) 
{
	UTIL_CHECK(i >= 0) << "FastHexToBuffer() wants non-negative integers, not " << i;

	static const char *hexdigits = "0123456789abcdef";
	char *p = buffer + 21;
	*p-- = '\0';
	do {
		*p-- = hexdigits[i & 15];   // mod by 16
		i >>= 4;                    // divide by 16
	} while (i > 0);
	return p + 1;
}

static char *InternalFastHexToBuffer(uint64 value, char* buffer, int num_byte) 
{
	static const char *hexdigits = "0123456789abcdef";
	buffer[num_byte] = '\0';
	for (int i = num_byte - 1; i >= 0; i--) 
	{
#ifdef _M_X64
		// MSVC x64 platform has a bug optimizing the uint32(value) in the #else
		// block. Given that the uint32 cast was to improve performance on 32-bit
		// platforms, we use 64-bit '&' directly.
		buffer[i] = hexdigits[value & 0xf];
#else
		buffer[i] = hexdigits[uint32(value) & 0xf];
#endif
		value >>= 4;
	}
	return buffer;
}

char *FastHex64ToBuffer(uint64 value, char* buffer) 
{
	return InternalFastHexToBuffer(value, buffer, 16);
}

char *FastHex32ToBuffer(uint32 value, char* buffer)
{
	return InternalFastHexToBuffer(value, buffer, 8);
}

static inline char* PlaceNum(char* p, int num, char prev_sep) 
{
	*p-- = '0' + num % 10;
	*p-- = '0' + num / 10;
	*p-- = prev_sep;
	return p;
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

static const char two_ASCII_digits[100][2] = {
	{'0','0'}, {'0','1'}, {'0','2'}, {'0','3'}, {'0','4'},
	{'0','5'}, {'0','6'}, {'0','7'}, {'0','8'}, {'0','9'},
	{'1','0'}, {'1','1'}, {'1','2'}, {'1','3'}, {'1','4'},
	{'1','5'}, {'1','6'}, {'1','7'}, {'1','8'}, {'1','9'},
	{'2','0'}, {'2','1'}, {'2','2'}, {'2','3'}, {'2','4'},
	{'2','5'}, {'2','6'}, {'2','7'}, {'2','8'}, {'2','9'},
	{'3','0'}, {'3','1'}, {'3','2'}, {'3','3'}, {'3','4'},
	{'3','5'}, {'3','6'}, {'3','7'}, {'3','8'}, {'3','9'},
	{'4','0'}, {'4','1'}, {'4','2'}, {'4','3'}, {'4','4'},
	{'4','5'}, {'4','6'}, {'4','7'}, {'4','8'}, {'4','9'},
	{'5','0'}, {'5','1'}, {'5','2'}, {'5','3'}, {'5','4'},
	{'5','5'}, {'5','6'}, {'5','7'}, {'5','8'}, {'5','9'},
	{'6','0'}, {'6','1'}, {'6','2'}, {'6','3'}, {'6','4'},
	{'6','5'}, {'6','6'}, {'6','7'}, {'6','8'}, {'6','9'},
	{'7','0'}, {'7','1'}, {'7','2'}, {'7','3'}, {'7','4'},
	{'7','5'}, {'7','6'}, {'7','7'}, {'7','8'}, {'7','9'},
	{'8','0'}, {'8','1'}, {'8','2'}, {'8','3'}, {'8','4'},
	{'8','5'}, {'8','6'}, {'8','7'}, {'8','8'}, {'8','9'},
	{'9','0'}, {'9','1'}, {'9','2'}, {'9','3'}, {'9','4'},
	{'9','5'}, {'9','6'}, {'9','7'}, {'9','8'}, {'9','9'}
};

char* FastUInt32ToBufferLeft(uint32 u, char* buffer) 
{
	int digits;
	const char *ASCII_digits = NULL;
	// The idea of this implementation is to trim the number of divides to as few
	// as possible by using multiplication and subtraction rather than mod (%),
	// and by outputting two digits at a time rather than one.
	// The huge-number case is first, in the hopes that the compiler will output
	// that case in one branch-free block of code, and only output conditional
	// branches into it from below.
	if (u >= 1000000000) 
	{ 
		// >= 1,000,000,000
		digits = u / 100000000;  // 100,000,000
		ASCII_digits = two_ASCII_digits[digits];
		buffer[0] = ASCII_digits[0];
		buffer[1] = ASCII_digits[1];
		buffer += 2;
sublt100_000_000:
		u -= digits * 100000000;  // 100,000,000
lt100_000_000:
		digits = u / 1000000;  // 1,000,000
		ASCII_digits = two_ASCII_digits[digits];
		buffer[0] = ASCII_digits[0];
		buffer[1] = ASCII_digits[1];
		buffer += 2;
sublt1_000_000:
		u -= digits * 1000000;  // 1,000,000
lt1_000_000:
		digits = u / 10000;  // 10,000
		ASCII_digits = two_ASCII_digits[digits];
		buffer[0] = ASCII_digits[0];
		buffer[1] = ASCII_digits[1];
		buffer += 2;
sublt10_000:
		u -= digits * 10000;  // 10,000
lt10_000:
		digits = u / 100;
		ASCII_digits = two_ASCII_digits[digits];
		buffer[0] = ASCII_digits[0];
		buffer[1] = ASCII_digits[1];
		buffer += 2;
sublt100:
		u -= digits * 100;
lt100:
		digits = u;
		ASCII_digits = two_ASCII_digits[digits];
		buffer[0] = ASCII_digits[0];
		buffer[1] = ASCII_digits[1];
		buffer += 2;
done:
		*buffer = 0;
		return buffer;
	}

	if (u < 100) 
	{
		digits = u;
		if (u >= 10) goto lt100;
		*buffer++ = '0' + digits;
		goto done;
	}
	if (u  <  10000)
	{   // 10,000
		if (u >= 1000) goto lt10_000;
		digits = u / 100;
		*buffer++ = '0' + digits;
		goto sublt100;
	}
	if (u  <  1000000)
	{   // 1,000,000
		if (u >= 100000) goto lt1_000_000;
		digits = u / 10000;  //    10,000
		*buffer++ = '0' + digits;
		goto sublt10_000;
	}
	if (u  <  100000000)
	{   // 100,000,000
		if (u >= 10000000) goto lt100_000_000;
		digits = u / 1000000;  //   1,000,000
		*buffer++ = '0' + digits;
		goto sublt1_000_000;
	}
	// we already know that u < 1,000,000,000
	digits = u / 100000000;   // 100,000,000
	*buffer++ = '0' + digits;
	goto sublt100_000_000;
}

char* FastInt32ToBufferLeft(int32 i, char* buffer)
{
	uint32 u = i;
	if (i < 0)
	{
		*buffer++ = '-';
		u = -i;
	}
	return FastUInt32ToBufferLeft(u, buffer);
}

char* FastUInt64ToBufferLeft(uint64 u64, char* buffer)
{
	int digits;
	const char *ASCII_digits = NULL;

	uint32 u = static_cast<uint32>(u64);
	if (u == u64) return FastUInt32ToBufferLeft(u, buffer);

	uint64 top_11_digits = u64 / 1000000000;
	buffer = FastUInt64ToBufferLeft(top_11_digits, buffer);
	u = u64 - (top_11_digits * 1000000000);

	digits = u / 10000000;  // 10,000,000
	UTIL_DCHECK_LT(digits, 100);
	ASCII_digits = two_ASCII_digits[digits];
	buffer[0] = ASCII_digits[0];
	buffer[1] = ASCII_digits[1];
	buffer += 2;
	u -= digits * 10000000;  // 10,000,000
	digits = u / 100000;  // 100,000
	ASCII_digits = two_ASCII_digits[digits];
	buffer[0] = ASCII_digits[0];
	buffer[1] = ASCII_digits[1];
	buffer += 2;
	u -= digits * 100000;  // 100,000
	digits = u / 1000;  // 1,000
	ASCII_digits = two_ASCII_digits[digits];
	buffer[0] = ASCII_digits[0];
	buffer[1] = ASCII_digits[1];
	buffer += 2;
	u -= digits * 1000;  // 1,000
	digits = u / 10;
	ASCII_digits = two_ASCII_digits[digits];
	buffer[0] = ASCII_digits[0];
	buffer[1] = ASCII_digits[1];
	buffer += 2;
	u -= digits * 10;
	digits = u;
	*buffer++ = '0' + digits;
	*buffer = 0;
	return buffer;
}

char* FastInt64ToBufferLeft(int64 i, char* buffer)
{
	uint64 u = i;
	if (i < 0) 
	{
		*buffer++ = '-';
		u = -i;
	}
	return FastUInt64ToBufferLeft(u, buffer);
}

// ----------------------------------------------------------------------
// SimpleItoa()
//    Description: converts an integer to a string.
//
//    Return value: string
// ----------------------------------------------------------------------

string SimpleItoa(int i) 
{
	char buffer[kFastToBufferSize];
	return (sizeof(i) == 4) ? 
		FastInt32ToBuffer(i, buffer) : 
		FastInt64ToBuffer(i, buffer);
}

string SimpleItoa(unsigned int i)
{
	char buffer[kFastToBufferSize];
	return string(buffer, (sizeof(i) == 4) ?
		FastUInt32ToBufferLeft(i, buffer) :
		FastUInt64ToBufferLeft(i, buffer));
}

string SimpleItoa(long i)
{
	char buffer[kFastToBufferSize];
	return (sizeof(i) == 4) ?
		FastInt32ToBuffer(i, buffer) :
		FastInt64ToBuffer(i, buffer);
}

string SimpleItoa(unsigned long i) 
{
	char buffer[kFastToBufferSize];
	return string(buffer, (sizeof(i) == 4) ?
		FastUInt32ToBufferLeft(i, buffer) :
		FastUInt64ToBufferLeft(i, buffer));
}

string SimpleItoa(long long i) 
{
	char buffer[kFastToBufferSize];
	return (sizeof(i) == 4) ?
		FastInt32ToBuffer(i, buffer) :
		FastInt64ToBuffer(i, buffer);
}

string SimpleItoa(unsigned long long i)
{
	char buffer[kFastToBufferSize];
	return string(buffer, (sizeof(i) == 4) ?
		FastUInt32ToBufferLeft(i, buffer) :
		FastUInt64ToBufferLeft(i, buffer));
}

// ----------------------------------------------------------------------
// SimpleDtoa()
// SimpleFtoa()
// DoubleToBuffer()
// FloatToBuffer()
//    We want to print the value without losing precision, but we also do
//    not want to print more digits than necessary.  This turns out to be
//    trickier than it sounds.  Numbers like 0.2 cannot be represented
//    exactly in binary.  If we print 0.2 with a very large precision,
//    e.g. "%.50g", we get "0.2000000000000000111022302462515654042363167".
//    On the other hand, if we set the precision too low, we lose
//    significant digits when printing numbers that actually need them.
//    It turns out there is no precision value that does the right thing
//    for all numbers.
//
//    Our strategy is to first try printing with a precision that is never
//    over-precise, then parse the result with strtod() to see if it
//    matches.  If not, we print again with a precision that will always
//    give a precise result, but may use more digits than necessary.
//
//    An arguably better strategy would be to use the algorithm described
//    in "How to Print Floating-Point Numbers Accurately" by Steele &
//    White, e.g. as implemented by David M. Gay's dtoa().  It turns out,
//    however, that the following implementation is about as fast as
//    DMG's code.  Furthermore, DMG's code locks mutexes, which means it
//    will not scale well on multi-core machines.  DMG's code is slightly
//    more accurate (in that it will never use more digits than
//    necessary), but this is probably irrelevant for most users.
//
//    Rob Pike and Ken Thompson also have an implementation of dtoa() in
//    third_party/fmt/fltfmt.cc.  Their implementation is similar to this
//    one in that it makes guesses and then uses strtod() to check them.
//    Their implementation is faster because they use their own code to
//    generate the digits in the first place rather than use snprintf(),
//    thus avoiding format string parsing overhead.  However, this makes
//    it considerably more complicated than the following implementation,
//    and it is embedded in a larger library.  If speed turns out to be
//    an issue, we could re-implement this in terms of their
//    implementation.
// ----------------------------------------------------------------------

string SimpleDtoa(double value) 
{
	char buffer[kDoubleToBufferSize];
	return DoubleToBuffer(value, buffer);
}

string SimpleFtoa(float value) 
{
	char buffer[kFloatToBufferSize];
	return FloatToBuffer(value, buffer);
}

static inline bool IsValidFloatChar(char c)
{
	return ('0' <= c && c <= '9')
		|| c == 'e' || c == 'E'
		|| c == '+' || c == '-';
}

static void DelocalizeRadix(char* buffer)
{
	// Fast check:  if the buffer has a normal decimal point, assume no
	// translation is needed.
	if (strchr(buffer, '.') != NULL) return;

	// Find the first unknown character.
	while (IsValidFloatChar(*buffer)) ++buffer;

	if (*buffer == '\0') 
	{
		// No radix character found.
		return;
	}

	// We are now pointing at the locale-specific radix character.  Replace it
	// with '.'.
	*buffer = '.';
	++buffer;

	if (!IsValidFloatChar(*buffer) && *buffer != '\0')
	{
		// It appears the radix was a multi-byte character.  We need to remove the
		// extra bytes.
		char* target = buffer;
		do { ++buffer; } while (!IsValidFloatChar(*buffer) && *buffer != '\0');
		memmove(target, buffer, strlen(buffer) + 1);
	}
}

char* DoubleToBuffer(double value, char* buffer)
{
	// DBL_DIG is 15 for IEEE-754 doubles, which are used on almost all
	// platforms these days.  Just in case some system exists where DBL_DIG
	// is significantly larger -- and risks overflowing our buffer -- we have
	// this assert.
	COMPILE_ASSERT(DBL_DIG < 20, DBL_DIG_is_too_big);

	if (value == numeric_limits<double>::infinity())
	{
		strcpy(buffer, "inf");
		return buffer;
	}
	else if (value == -numeric_limits<double>::infinity()) 
	{
		strcpy(buffer, "-inf");
		return buffer;
	}
	else if (IsNaN(value))
	{
		strcpy(buffer, "nan");
		return buffer;
	}

	int snprintf_result =
		snprintf(buffer, kDoubleToBufferSize, "%.*g", DBL_DIG, value);

	// The snprintf should never overflow because the buffer is significantly
	// larger than the precision we asked for.
	UTIL_DCHECK(snprintf_result > 0 && snprintf_result < kDoubleToBufferSize);

	// We need to make parsed_value volatile in order to force the compiler to
	// write it out to the stack.  Otherwise, it may keep the value in a
	// register, and if it does that, it may keep it as a long double instead
	// of a double.  This long double may have extra bits that make it compare
	// unequal to "value" even though it would be exactly equal if it were
	// truncated to a double.
	volatile double parsed_value = strtod(buffer, NULL);
	if (parsed_value != value)
	{
		int snprintf_result =
			snprintf(buffer, kDoubleToBufferSize, "%.*g", DBL_DIG + 2, value);

		// Should never overflow; see above.
		UTIL_DCHECK(snprintf_result > 0 && snprintf_result < kDoubleToBufferSize);
	}

	DelocalizeRadix(buffer);
	return buffer;
}

bool safe_strtof(const char* str, float* value)
{
	char* endptr;
	errno = 0;  // errno only gets set on errors
#if defined(_WIN32) || defined (__hpux)  // has no strtof()
	*value = strtod(str, &endptr);
#else
	*value = strtof(str, &endptr);
#endif
	return *str != 0 && *endptr == 0 && errno == 0;
}

char* FloatToBuffer(float value, char* buffer)
{
	// FLT_DIG is 6 for IEEE-754 floats, which are used on almost all
	// platforms these days.  Just in case some system exists where FLT_DIG
	// is significantly larger -- and risks overflowing our buffer -- we have
	// this assert.
	COMPILE_ASSERT(FLT_DIG < 10, FLT_DIG_is_too_big);

	if (value == numeric_limits<double>::infinity())
	{
		strcpy(buffer, "inf");
		return buffer;
	} 
	else if (value == -numeric_limits<double>::infinity())
	{
		strcpy(buffer, "-inf");
		return buffer;
	}
	else if (IsNaN(value)) 
	{
		strcpy(buffer, "nan");
		return buffer;
	}

	int snprintf_result =
		snprintf(buffer, kFloatToBufferSize, "%.*g", FLT_DIG, value);

	// The snprintf should never overflow because the buffer is significantly
	// larger than the precision we asked for.
	UTIL_DCHECK(snprintf_result > 0 && snprintf_result < kFloatToBufferSize);

	float parsed_value;
	if (!safe_strtof(buffer, &parsed_value) || parsed_value != value) 
	{
		int snprintf_result =
			snprintf(buffer, kFloatToBufferSize, "%.*g", FLT_DIG+2, value);

		// Should never overflow; see above.
		UTIL_DCHECK(snprintf_result > 0 && snprintf_result < kFloatToBufferSize);
	}

	DelocalizeRadix(buffer);
	return buffer;
}

// ----------------------------------------------------------------------
// NoLocaleStrtod()
//   This code will make you cry.
// ----------------------------------------------------------------------

// Returns a string identical to *input except that the character pointed to
// by radix_pos (which should be '.') is replaced with the locale-specific
// radix character.
string LocalizeRadix(const char* input, const char* radix_pos)
{
	// Determine the locale-specific radix character by calling sprintf() to
	// print the number 1.5, then stripping off the digits.  As far as I can
	// tell, this is the only portable, thread-safe way to get the C library
	// to divuldge the locale's radix character.  No, localeconv() is NOT
	// thread-safe.
	char temp[16];
	int size = sprintf(temp, "%.1f", 1.5);
	UTIL_CHECK_EQ(temp[0], '1');
	UTIL_CHECK_EQ(temp[size-1], '5');
	UTIL_CHECK_LE(size, 6);

	// Now replace the '.' in the input with it.
	string result;
	result.reserve(strlen(input) + size - 3);
	result.append(input, radix_pos);
	result.append(temp + 1, size - 2);
	result.append(radix_pos + 1);
	return result;
}

double NoLocaleStrtod(const char* text, char** original_endptr) 
{
	// We cannot simply set the locale to "C" temporarily with setlocale()
	// as this is not thread-safe.  Instead, we try to parse in the current
	// locale first.  If parsing stops at a '.' character, then this is a
	// pretty good hint that we're actually in some other locale in which
	// '.' is not the radix character.

	char* temp_endptr;
	double result = strtod(text, &temp_endptr);
	if (original_endptr != NULL) *original_endptr = temp_endptr;
	if (*temp_endptr != '.') return result;

	// Parsing halted on a '.'.  Perhaps we're in a different locale?  Let's
	// try to replace the '.' with a locale-specific radix character and
	// try again.
	string localized = LocalizeRadix(text, temp_endptr);
	const char* localized_cstr = localized.c_str();
	char* localized_endptr;
	result = strtod(localized_cstr, &localized_endptr);
	if ((localized_endptr - localized_cstr) > (temp_endptr - text))
	{
		// This attempt got further, so replacing the decimal must have helped.
		// Update original_endptr to point at the right location.
		if (original_endptr != NULL)
		{
			// size_diff is non-zero if the localized radix has multiple bytes.
			int size_diff = localized.size() - strlen(text);
			// const_cast is necessary to match the strtod() interface.
			*original_endptr = const_cast<char*>(
				text + (localized_endptr - localized_cstr - size_diff));
		}
	}

	return result;
}

// The following code is compatible with the OpenBSD lcpy interface.  See:
//   http://www.gratisoft.us/todd/papers/strlcpy.html
//   ftp://ftp.openbsd.org/pub/OpenBSD/src/lib/libc/string/{wcs,str}lcpy.c

namespace 
{
template <typename CHAR>
size_t lcpyT(CHAR* dst, const CHAR* src, size_t dst_size)
{
	for (size_t i = 0; i < dst_size; ++i)
	{
		if ((dst[i] = src[i]) == 0)  // We hit and copied the terminating NULL.
			return i;
	}

	// We were left off at dst_size.  We over copied 1 byte.  Null terminate.
	if (dst_size != 0)
	{
		dst[dst_size - 1] = 0;
	}

	// Count the rest of the |src|, and return it's length in characters.
	while (src[dst_size]) ++dst_size;
	return dst_size;
}

}  // namespace

size_t Util::strlcpy(char* dst, const char* src, size_t dst_size) 
{
	return lcpyT<char>(dst, src, dst_size);
}
size_t Util::wcslcpy(wchar_t* dst, const wchar_t* src, size_t dst_size) 
{
	return lcpyT<wchar_t>(dst, src, dst_size);
}

#define WHITESPACE_UNICODE \
	0x0009, /** <control-0009> to <control-000D> */ \
	0x000A,                                        \
	0x000B,                                        \
	0x000C,                                        \
	0x000D,                                        \
	0x0020, /** Space */                            \
	0x0085, /** <control-0085> */                   \
	0x00A0, /** No-Break Space */                   \
	0x1680, /** Ogham Space Mark */                 \
	0x180E, /** Mongolian Vowel Separator */        \
	0x2000, /** En Quad to Hair Space */            \
	0x2001,                                        \
	0x2002,                                        \
	0x2003,                                        \
	0x2004,                                        \
	0x2005,                                        \
	0x2006,                                        \
	0x2007,                                        \
	0x2008,                                        \
	0x2009,                                        \
	0x200A,                                        \
	0x200C, /** Zero Width Non-Joiner */            \
	0x2028, /** Line Separator */                   \
	0x2029, /** Paragraph Separator */              \
	0x202F, /** Narrow No-Break Space */            \
	0x205F, /** Medium Mathematical Space */        \
	0x3000, /** Ideographic Space */                \
	0

const wchar_t kWhitespaceWide[] = {
	WHITESPACE_UNICODE
};

const char kWhitespaceASCII[] = {
	0x09,    // <control-0009> to <control-000D>
	0x0A,
	0x0B,
	0x0C,
	0x0D,
	0x20,    // Space
	0
};

bool TrimString(const std::wstring& input,
				const wchar_t trim_chars[],
				std::wstring* output)
{
	return TrimString(input, trim_chars, TRIM_ALL, output) != TRIM_NONE;
}

bool TrimString(const std::string& input,
				const char trim_chars[],
				std::string* output)
{
	return TrimString(input, trim_chars, TRIM_ALL, output) != TRIM_NONE;
}

TrimPositions TrimWhitespace(const std::wstring& input,
							 TrimPositions positions,
							 std::wstring* output)
{
	return TrimString(input, kWhitespaceWide, positions, output);
}

TrimPositions TrimWhitespaceASCII(const std::string& input,
								  TrimPositions positions,
								  std::string* output)
{
	return TrimString(input, kWhitespaceASCII, positions, output);
}

// This function is only for backward-compatibility.
// To be removed when all callers are updated.
TrimPositions TrimWhitespace(const std::string& input,
							 TrimPositions positions,
							 std::string* output)
{
	return TrimWhitespaceASCII(input, positions, output);
}

template<typename Iter>
static inline bool LowerCaseEqualsASCII(Iter src_begin,
										Iter src_end,
										const char* dst) 
{
	for (Iter it = src_begin; it != src_end; ++it, ++dst)
	{
		if (!*dst || Util::ToLowerASCII(*it) != *dst)
			return false;
	}
	return 0 == *dst;
}

bool LowerCaseEqualsASCII(const std::wstring& input,
						  const char* dst)
{
	return LowerCaseEqualsASCII(input.begin(), input.end(), dst);
}

bool LowerCaseEqualsASCII(const std::string& input,
						  const char* dst)
{
	return LowerCaseEqualsASCII(input.begin(), input.end(), dst);
}

DataUnits GetByteDisplayUnits(int64 bytes) 
{
	// The byte thresholds at which we display amounts.  A byte count is displayed
	// in unit U when kUnitThresholds[U] <= bytes < kUnitThresholds[U+1].
	// This must match the DataUnits enum.
	static const int64 kUnitThresholds[] = {
		0,              // DATA_UNITS_BYTE,
		3*1024,         // DATA_UNITS_KIBIBYTE,
		2*1024*1024,    // DATA_UNITS_MEBIBYTE,
		1024*1024*1024  // DATA_UNITS_GIBIBYTE,
	};

	if (bytes < 0)
	{
		NOTREACHED() << "Negative bytes value";
		return DATA_UNITS_BYTE;
	}

	int unit_index = arraysize(kUnitThresholds);
	while (--unit_index > 0) 
	{
		if (bytes >= kUnitThresholds[unit_index])
		{
			break;
		}
	}

	UTIL_DCHECK(unit_index >= DATA_UNITS_BYTE && unit_index <= DATA_UNITS_GIBIBYTE);
	return DataUnits(unit_index);
}

// TODO(mpcomplete): deal with locale
// Byte suffixes.  This must match the DataUnits enum.
static const char* const kByteStrings[] = {
	"B",
	"kB",
	"MB",
	"GB"
};

static const char* const kSpeedStrings[] = {
	"B/s",
	"kB/s",
	"MB/s",
	"GB/s"
};

std::string FormatBytesInternal(int64 bytes,
							 DataUnits units,
							 bool show_units,
							 const char* const* suffix)
{
	if (bytes < 0)
	{
		NOTREACHED() << "Negative bytes value";
		return std::string();
	}

	UTIL_DCHECK(units >= DATA_UNITS_BYTE && units <= DATA_UNITS_GIBIBYTE);

	// Put the quantity in the right units.
	double unit_amount = static_cast<double>(bytes);
	for (int i = 0; i < units; ++i)
	{
		unit_amount /= 1024.0;
	}

	char buf[64];
	if (bytes != 0 && units != DATA_UNITS_BYTE && unit_amount < 100)
	{
		snprintf(buf, arraysize(buf), "%.1lf", unit_amount);
	}
	else
	{
		snprintf(buf, arraysize(buf), "%.0lf", unit_amount);
	}

	std::string ret(buf);
	if (show_units)
	{
		ret += " ";
		ret += suffix[units];
	}

	return ret;
}

std::string FormatBytes(int64 bytes, DataUnits units, bool show_units)
{
	return FormatBytesInternal(bytes, units, show_units, kByteStrings);
}

std::string FormatSpeed(int64 bytes, DataUnits units, bool show_units)
{
	return FormatBytesInternal(bytes, units, show_units, kSpeedStrings);
}

std::string FormatNumber(int64 number)
{
	bool negtive = false;
	uint64 unum = number;
	if (number < 0) 
	{
		negtive = true;
		unum = -number;
	}

	char buffer[kFastInt64ToBufferOffset + 1];
	char *pend = FastUInt64ToBufferLeft(unum, buffer);
	size_t len = pend - buffer;
	if (0 == len)
	{
		return std::string();
	}

	len += (len - 1) / 3 + (negtive ? 1 : 0);

	std::string ret;
	ret.resize(len);
	
	size_t cnt = 0;
	while(0 != len && pend != buffer)
	{
		ret[len--] = *--pend;
		if (++cnt % 3)
		{
			ret[len--] = ',';
		}
	}

	return ret;
}

bool ElideString(const std::wstring& input, int max_len, std::wstring* output)
{
	UTIL_DCHECK(max_len >= 0);
	if (static_cast<int>(input.length()) <= max_len)
	{
		output->assign(input);
		return false;
	}

	switch (max_len)
	{
	case 0:
		output->clear();
		break;
	case 1:
		output->assign(input.substr(0, 1));
		break;
	case 2:
		output->assign(input.substr(0, 2));
		break;
	case 3:
		output->assign(input.substr(0, 1) + L"." +
			input.substr(input.length() - 1));
		break;
	case 4:
		output->assign(input.substr(0, 1) + L".." +
			input.substr(input.length() - 1));
		break;
	default:
		{
			int rstr_len = (max_len - 3) / 2;
			int lstr_len = rstr_len + ((max_len - 3) % 2);
			output->assign(input.substr(0, lstr_len) + L"..." +
				input.substr(input.length() - rstr_len));
			break;
		}
	}

	return true;
}

#endif

UTIL_END
