// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_STRING_UTIL_H
#define UTIL_STRING_UTIL_H

#include <string>
#include <Config.h>
//#include <Util/StaticAssert.h>
#include <Util/ErrorToString.h>

THREADING_BEGIN


#ifdef _WIN32
// Creates a UTF-16 wide string from the given ANSI string, allocating
// memory using new. The caller is responsible for deleting the return
// value using delete[]. Returns the wide string, or NULL if the
// input is NULL.
THREADING_API LPCWSTR AnsiToUTF16(const char* ansi);

// Creates an ANSI string from the given wide string, allocating
// memory using new. The caller is responsible for deleting the return
// value using delete[]. Returns the ANSI string, or NULL if the
// input is NULL.
THREADING_API const char* UTF16ToAnsi(LPCWSTR utf16_str);
#endif

//
// Add escape sequences (like "\n", or "\0xxx") to make a string
// readable in ASCII.
//
THREADING_API std::string EscapeString(const std::string&, const std::string&);

//
// Remove escape sequences added by escapeString. Throws IllegalArgumentException
// for an invalid input string.
//
THREADING_API std::string UnescapeString(const std::string&, std::string::size_type, std::string::size_type);

//
// Split a string using the given delimiters. Considers single and double quotes;
// returns false for unbalanced quote, true otherwise.
//
THREADING_API bool SplitString(const std::string& str, const std::string& delim, std::vector<std::string>& result, bool keepblank = false);

//
// Join a list of strings using the given delimiter. 
//
THREADING_API std::string JoinString(const std::vector<std::string>& values, const std::string& delimiter);

//
// Trim white space
//
THREADING_API std::string Trim(const std::string& src);

//
// If a single or double quotation mark is found at the start
// position, then the position of the matching closing quote is
// returned. If no quotation mark is found at the start position, then
// 0 is returned. If no matching closing quote is found, then
// std::string::npos is returned.
//
THREADING_API std::string::size_type CheckQuote(const std::string&, std::string::size_type = 0);

THREADING_API std::string::size_type ExistQuote(const std::string&, std::string::size_type = 0);

//
// Match `s' against the pattern `pat'. A * in the pattern acts
// as a wildcard: it matches any non-empty sequence of characters
// other than a period (`.'). We match by hand here because
// it's portable across platforms (whereas regex() isn't).
//
THREADING_API bool Match(const std::string& s, const std::string& pat, bool = false);

//
// Translating both the two-character sequence #xD #xA and any #xD that is not followed by #xA to 
// a single #xA character.
//    LF  (Line feed, '\n', 0x0A, 10 in decimal)  
//    CR (Carriage return, '\r', 0x0D, 13 in decimal) 
//
THREADING_API std::string TranslatingCR2LF(const std::string& src);

//
// Functions to convert to lower/upper case. These functions accept
// UTF8 string/characters but ignore non ASCII characters. Unlike, the
// C methods, these methods are not local dependent.
//
THREADING_API std::string ToLower(const std::string&);
THREADING_API std::string ToUpper(const std::string&);

THREADING_API unsigned long Hash(const std::string&);

//
// Remove all whitespace from a string
//
THREADING_API std::string RemoveWhitespace(const std::string&);

//////////////////////////////////////////////////////////////////////////
/// string & data convert
//
// Portable strtoll/_strtoi64
//
THREADING_API Threading::Int64 ToInt64(const char* s, char** endptr, int base);

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
THREADING_API  bool ToInt64(const std::string& s,  Threading::Int64& result);

THREADING_API unsigned long ToULong(const std::string& strval, size_t* endindex = 0, unsigned int base = 10);

THREADING_API long ToLong(const std::string& strval, size_t* endindex = 0, unsigned int base = 10);

THREADING_API double ToDouble(const std::string& strval, size_t* endindex = 0, int precision = 6);

template <typename T>
inline
T ToData(const std::string& strval, size_t* endindex = 0, unsigned int base = 10);

THREADING_API std::string ToString(unsigned long n);

THREADING_API std::string ToString(long n);

//
// Determines if a string is a number of not.
//
THREADING_API  bool IsNumber(const std::string& s, bool* isdecimal);

//
// Skip leading none digit character, get the first number in string.
//
THREADING_API  int GetIntInString(const char* s, char** endptr, int base);

THREADING_API const Threading::Byte* FindStringInBuffer(Threading::Byte* pBuff, size_t iBuffSize, const std::string& strSearch);

THREADING_API std::string BytesToString(const Threading::Byte* src, size_t size);
THREADING_API std::string BytesToString(const Threading::ByteSeq& bytes);
THREADING_API Threading::ByteSeq StringToBytes(const std::string&);

//
// Hex-dump at most 16 bytes starting at offset from a memory area of size
// bytes.  Return the number of bytes actually dumped.
//
THREADING_API size_t HexDumpLine(const void* ptr, size_t offset, size_t size, std::string& line, size_t linelength = 16);

template <typename OutIt>
static inline
void HexDump(const void* ptr, size_t size, OutIt out, size_t linelength/* = 16*/);

THREADING_API std::string HexDump(const void* ptr, size_t size, size_t linelength = 16);

THREADING_API std::string HexStringToBuffer(const std::string &hexString, std::string &buffer, const std::string& delimiter = ",");

THREADING_API size_t BinDumpLine(const void* ptr, size_t offset, size_t size, std::string& line, size_t linelength = 8);

template <typename OutIt>
static inline
void BinDump(const void* ptr, size_t size, OutIt out, size_t linelength/* = 8*/);

THREADING_API std::string BinDump(const void* ptr, size_t size, size_t linelength = 8);

// If *pstr starts with the given prefix, modifies *pstr to be right
// past the prefix and returns true; otherwise leaves *pstr unchanged
// and returns false.  None of pstr, *pstr, and prefix can be NULL.
THREADING_API bool SkipPrefix(const char* prefix, const char** pstr);

#ifdef HAS_STD_WSTREAM

//
// Wide stream input operator.
//
std::wistream& operator >>(std::wistream& is, std::string& strret);

//
// Wide stream output operator.
//
std::wostream& operator <<(std::wostream& os, const std::string& strsrc);

#endif // HAS_STD_WSTREAM

//////////////////////////////////////////////////////////////////////////
// data to string conversion
template <typename T>
inline
std::string ToHexString(T n, bool bupper/* = false*/)
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
inline
std::string ToOctalString(T n)
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
inline
std::string ToBinaryString(T n)
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

//
// Swap lhs and rhs's value
//
inline
void Swap(unsigned char* lhs, unsigned char* rhs)
{
    //return std::swap(*lhs, *rhs);

    unsigned char tmp = *lhs;
    *lhs = *rhs;
    *rhs = tmp;
}

//
// Reverse elements in [begin, end)
//
inline
void ReverseBuffer(unsigned char* begin, unsigned char* end)
{
    //return std::reverse(begin, end);
    while (begin < --end)
    {
        Swap(begin++, end);
    }
}

template <typename OutIt>
inline
void HexDump(const void* ptr, size_t size, OutIt out, size_t linelength)
{
    size_t offset = 0;
    std::string line;
    while (offset < size) 
    {
        offset += HexDumpLine(ptr, offset, size, line, linelength);
        *out++ = line.c_str();
    }
}

template <typename OutIt>
inline 
void BinDump(const void* ptr, size_t size, OutIt out, size_t linelength)
{
    size_t offset = 0;
    std::string line;
    while (offset < size) 
    {
        offset += BinDumpLine(ptr, offset, size, line, linelength);
        *out++ = line.c_str();
    }
}

//
// Global Format utility
// 
THREADING_API std::string Format( const char* format, ...);

// Formats an int value as "%02d".
THREADING_API std::string FormatIntWidth2(int value);

// Formats an int value as "%X".
THREADING_API std::string FormatHexInt(int value);

// Formats a byte as "%02X".
THREADING_API std::string FormatByte(unsigned char value);

// Converts the buffer in a stringstream to an std::string, converting NUL
// bytes to "\\0" along the way.
THREADING_API std::string StringStreamToString(::std::stringstream* ss);

// Converts a Unicode code point to a narrow string in UTF-8 encoding.
// code_point parameter is of type unsigned int because wchar_t may not be
// wide enough to contain a code point.
// If the code_point is not a valid Unicode code point
// (i.e. outside of Unicode range U+0 to U+10FFFF) it will be converted
// to "(Invalid Unicode 0xXXXXXXXX)".
THREADING_API std::string CodePointToUtf8(unsigned int code_point);

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
THREADING_API std::string WideStringToUtf8(const wchar_t* str, int num_chars);

// Converts a wide C string to an std::string using the UTF-8 encoding.
// NULL will be converted to "(null)".
THREADING_API std::string ShowWideCString(const wchar_t * wide_c_str);

// Compares two wide C strings.  Returns true iff they have the same
// content.
//
// Unlike wcscmp(), this function can handle NULL argument(s).  A NULL
// C string is considered different to any non-NULL C string,
// including the empty string.
THREADING_API bool WideCStringEquals(const wchar_t * lhs, const wchar_t * rhs);

// Compares two C strings.  Returns true iff they have the same content.
//
// Unlike strcmp(), this function can handle NULL argument(s).  A NULL
// C string is considered different to any non-NULL C string,
// including the empty string.
THREADING_API bool CStringEquals(const char * lhs, const char * rhs);

// Compares two C strings, ignoring case.  Returns true iff they have
// the same content.
//
// Unlike strcasecmp(), this function can handle NULL argument(s).  A
// NULL C string is considered different to any non-NULL C string,
// including the empty string.
THREADING_API bool CaseInsensitiveCStringEquals(const char * lhs, const char * rhs);

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
THREADING_API bool CaseInsensitiveWideCStringEquals(const wchar_t* lhs, const wchar_t* rhs);

// Returns true iff str ends with the given suffix, ignoring case.
// Any string is considered to end with an empty suffix.
THREADING_API bool EndsWithCaseInsensitive(const std::string& str, const std::string& suffix);

THREADING_API std::string Double2String(double value, int precision);
THREADING_API double String2Double(const std::string& str);


//
// Utilities for char.
//

//THREADING_API bool IsAlpha(char);
//THREADING_API bool IsDigit(char);

inline bool IsAlpha(char ch) 
{
    return 0 != isalpha(static_cast<unsigned char>(ch));
}

inline bool IsAlNum(char ch)
{
    return 0 != isalnum(static_cast<unsigned char>(ch));
}

inline bool IsDigit(char ch) 
{
    return 0 != isdigit(static_cast<unsigned char>(ch));
}

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

THREADING_END

#endif
