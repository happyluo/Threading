// **********************************************************************
//
// Copyright (c) 2003-2012 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_UNICODE_H
#define UTIL_UNICODE_H

#include <Config.h>
#include <Util/Exception.h>

THREADING_BEGIN

enum ConversionFlags
{
    strictConversion = 0,
    lenientConversion
};

THREADING_API std::string WstringToString(const std::wstring&, ConversionFlags = lenientConversion);
THREADING_API std::wstring StringToWstring(const std::string&, ConversionFlags = lenientConversion);

#define WideToUTF8 Threading::WstringToString
#define UTF8ToWide Threading::StringToWstring

typedef unsigned char Byte;

THREADING_API bool
IsLegalUTF8Sequence(const Byte* source, const Byte* end);

enum ConversionErrorType
{
    partialCharacter,
    badEncoding
};

//
// UTFConversionException is raised by WstringToString() or StringToWstring()
// to report a conversion error 
//
class THREADING_API UTFConversionException : public Exception
{
public:
    
    UTFConversionException(const char*, int, ConversionErrorType);
    virtual std::string Name() const;
    virtual void Print(std::ostream&) const;
    virtual UTFConversionException* Clone() const;
    virtual void Throw() const;

    ConversionErrorType ConversionError() const;
private:

    const ConversionErrorType m_conversionError;
    static const char* m_name;    
};

//
// Converts UTF-8 byte-sequences to and from UTF-16 or UTF-32 (with native
// endianness) depending on sizeof(wchar_t).
//
// These are thin wrappers over the UTF8/16/32 converters provided by 
// unicode.org
//

enum ConversionResult
{
    conversionOK,           /* conversion successful */
    sourceExhausted,        /* partial character in source, but hit end */
    targetExhausted,        /* insuff. room in target for conversion */
    sourceIllegal           /* source sequence is illegal/malformed */
};

THREADING_API ConversionResult 
ConvertUTFWstringToUTF8(const wchar_t*& sourceStart, const wchar_t* sourceEnd, 
                        Threading::Byte*& targetStart, Threading::Byte* targetEnd, Threading::ConversionFlags flags);

THREADING_API ConversionResult
ConvertUTF8ToUTFWstring(const Threading::Byte*& sourceStart, const Threading::Byte* sourceEnd, 
                        wchar_t*& targetStart, wchar_t* targetEnd, Threading::ConversionFlags flags);

THREADING_API ConversionResult 
ConvertUTF8ToUTFWstring(const Threading::Byte*& sourceStart, const Threading::Byte* sourceEnd, 
                        std::wstring& target, Threading::ConversionFlags flags);

THREADING_END

#endif
