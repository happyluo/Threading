// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_CONSOLE_COLOR_H
#define UTIL_CONSOLE_COLOR_H

#include <ostream>
#include <Util/Config.h>
#include <Util/StringUtil.h>


//namespace UtilInternal
//{
//
//	extern bool DECLSPEC_IMPORT printStackTraces;
//
//}

UTIL_BEGIN

//
// color console ostream
//
template<class Elem>
class colorostreamT
{
public:
	colorostreamT(std::basic_ostream<Elem, std::char_traits<Elem> >& out, const std::string& colorflag = "auto") : 
		m_ostream(out)
		, m_strcolorflag(colorflag)
	{
		m_oldcolor = GetCurrentTextColor();
	}

	virtual ~colorostreamT()
	{
		RestoreTextColor(m_oldcolor);
	}

	operator std::basic_ostream<Elem, std::char_traits<Elem> >& ()
	{
		return m_ostream;
	}

	std::basic_ostream<Elem, std::char_traits<Elem> >& tostd()
	{
		return operator std::basic_ostream<Elem, std::char_traits<Elem> >&();
	}

	enum Color {
		COLOR_DEFAULT,
		COLOR_RED,
		COLOR_GREEN,
		COLOR_BLUE,
		COLOR_YELLOW
	};

#if OS_WINDOWS && !OS_WINDOWS_MOBILE

	// Returns the character attribute for the given color.
	static WORD GetForegroundColorAttribute(Color color) 
	{
		switch (color) 
		{
		case COLOR_RED:		return FOREGROUND_RED | FOREGROUND_INTENSITY;
		case COLOR_GREEN:	return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		case COLOR_BLUE:	return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		case COLOR_YELLOW:	return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		default:			return 0;
		}
	}

	static WORD GetBackgroundColorAttribute(Color color) 
	{
		switch (color) 
		{
		case COLOR_RED:		return BACKGROUND_RED | BACKGROUND_INTENSITY;
		case COLOR_GREEN:	return BACKGROUND_GREEN | BACKGROUND_INTENSITY;
		case COLOR_BLUE:	return BACKGROUND_BLUE | BACKGROUND_INTENSITY;
		case COLOR_YELLOW:	return BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
		default:			return 0;
		}
	}

#else

	// Returns the ANSI color code for the given color.  COLOR_DEFAULT is
	// an invalid input.
	static const char* GetAnsiColorCode(Color color) 
	{
		switch (color) 
		{
		case COLOR_RED:		return "1";
		case COLOR_GREEN:	return "2";
		case COLOR_YELLOW:	return "3";
		case COLOR_BLUE:	return "4";
		default:			return NULL;
		};
	}

	// Returns the character attribute for the given color.
	static const char*  GetForegroundColorAttribute(Color color) 
	{
		switch (color) 
		{
		case COLOR_RED:		return "31";
		case COLOR_GREEN:	return "32";
		case COLOR_BLUE:	return "33";
		case COLOR_YELLOW:	return "34";
		default:			return NULL;
		}
	}

	static const char*  GetBackgroundColorAttribute(Color color) 
	{
		switch (color) 
		{
		case COLOR_RED:		return "41";
		case COLOR_GREEN:	return "42";
		case COLOR_BLUE:	return "43";
		case COLOR_YELLOW:	return "44";
		default:			return NULL;
		}
	}

#endif  // OS_WINDOWS && !OS_WINDOWS_MOBILE

	// Returns true iff use colors in the output.
	static bool ShouldUseColor(bool stdout_is_tty, const std::string& colorflag);

	// Helpers for printing colored strings to stdout. Note that on Windows, we
	// cannot simply emit special characters and have the terminal change colors.
	// This routine must actually emit the characters rather than return a string
	// that would be colored when printed, as can be done on Linux.
	/*static*/ void ColoredPrintf(Color color, const char* fmt, ...);	

	// Prints a string containing code-encoded text.  The following escape
	// sequences can be used in the string to control the text color:
	//
	//   @@    prints a single '@' character.
	//   @R    changes the color to red.
	//   @G    changes the color to green.
	//   @Y    changes the color to yellow.
	//   @D    changes to the default terminal text color.
	//
	static void PrintColorEncoded(const char* str);

	static unsigned short GetCurrentTextColor();

	static void RestoreTextColor(unsigned short old_color_attrs);

	static void SetForegroundConsoleTextColor(Color color);

	static void SetBackgroundConsoleTextColor(Color color);

private:
	std::string m_strcolorflag;
	unsigned short m_oldcolor;
	std::basic_ostream<Elem, std::char_traits<Elem> >& m_ostream;
};

typedef colorostreamT<char> colorostream;
typedef colorostreamT<wchar_t> wcolorostream;

template<typename Elem, typename T>
inline colorostreamT<Elem>&
operator <<(colorostreamT<Elem>& out, const T& val)
{
	out.tostd() << val;
	return out;
}

template<typename Elem>
inline colorostreamT<Elem>&
operator <<(colorostreamT<Elem>& out, std::ios_base& (*val)(std::ios_base&))
{
	out.tostd() << val;
	return out;
}

template<typename Elem>
inline colorostreamT<Elem>& 
operator <<(colorostreamT<Elem>& out, std::ostream& (*val)(std::ostream&))
{
	out.tostd() << val;
	return out;
}

template<typename Elem>
inline colorostreamT<Elem>&
operator <<(colorostreamT<Elem>& out, const std::exception& ex)
{
	//if (UtilInternal::printStackTraces)
	//{
	//	const ::Util::Exception* exception = dynamic_cast<const ::Util::Exception*>(&ex);
	//	if (exception)
	//	{
	//		out.tostd() << exception->what() << '\n' << exception->StackTrace();
	//		return out;
	//	}
	//}
	out.tostd() << ex.what();
	return out;
}


// We take "yes", "true", "t", and "1" as meaning "yes".  If the
// value is neither one of these nor "auto", we treat it as "no" to
// be conservative.
template<class Elem>
bool colorostreamT<Elem>::ShouldUseColor(bool stdout_is_tty, const std::string& colorflag)
{
	const char* const color = colorflag.c_str();

	if (Util::CaseInsensitiveCStringEquals(color, "auto"))
	{
#if OS_WINDOWS
		// On Windows the TERM variable is usually not set, but the
		// console there does support colors.
		return stdout_is_tty;
#else
		// On non-Windows platforms, we rely on the TERM variable.
		const char* const term = Posix::GetEnv("TERM");
		const bool term_supports_color =
			Util::CStringEquals(term, "xterm") ||
			Util::CStringEquals(term, "xterm-color") ||
			Util::CStringEquals(term, "xterm-256color") ||
			Util::CStringEquals(term, "screen") ||
			Util::CStringEquals(term, "linux") ||
			Util::CStringEquals(term, "cygwin");

		return stdout_is_tty && term_supports_color;
#endif  // OS_WINDOWS
	}

	return Util::CaseInsensitiveCStringEquals(color, "yes") ||
		Util::CaseInsensitiveCStringEquals(color, "true") ||
		Util::CaseInsensitiveCStringEquals(color, "t") ||
		Util::CStringEquals(color, "1");
}

template<class Elem>
void colorostreamT<Elem>::ColoredPrintf(Color color, const char* fmt, ...) 
{
	va_list args;
	va_start(args, fmt);

#if OS_WINDOWS_MOBILE || OS_SYMBIAN || OS_ZOS || OS_IOS
	const bool use_color = false;
#else
	static const bool in_color_mode =
		ShouldUseColor(Posix::IsATTY(Posix::FileNo(stdout)) != 0, m_strcolorflag);
	const bool use_color = in_color_mode && (color != COLOR_DEFAULT);
#endif  // OS_WINDOWS_MOBILE || OS_SYMBIAN || OS_ZOS

	if (!use_color)
	{
		vprintf(fmt, args);
		va_end(args);
		return;
	}

#if OS_WINDOWS && !OS_WINDOWS_MOBILE
	// Gets the current text color.
	const WORD old_color_attrs = GetCurrentTextColor();

	const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	// We need to flush the stream buffers into the console before each
	// SetConsoleTextAttribute call lest it affect the text that is already
	// printed but has not yet reached the console.
	fflush(stdout);
	SetConsoleTextAttribute(stdout_handle,
		GetForegroundColorAttribute(color) | FOREGROUND_INTENSITY);
	vprintf(fmt, args);

	fflush(stdout);
	// Restores the text color.
	SetConsoleTextAttribute(stdout_handle, old_color_attrs);
#else
	printf("\033[0;3%sm", GetAnsiColorCode(color));
	vprintf(fmt, args);
	printf("\033[m");  // Resets the terminal to default.
#endif  // OS_WINDOWS && !OS_WINDOWS_MOBILE
	va_end(args);
}

template<class Elem>
void colorostreamT<Elem>::PrintColorEncoded(const char* str)
{
	colorostreamT<Elem> colorout(std::cout);

	Color color = COLOR_DEFAULT;  // The current color.

	for (;;)
	{
		const char* p = strchr(str, '@');
		if (p == NULL)
		{
			colorout.ColoredPrintf(color, "%s", str);
			return;
		}

		colorout.ColoredPrintf(color, "%s", std::string(str, p).c_str());

		const char ch = p[1];
		str = p + 2;
		if ('@' == ch) {
			colorout.ColoredPrintf(color, "@");
		} else if ('D' == ch) {
			color = COLOR_DEFAULT;
		} else if ('R' == ch) {
			color = COLOR_RED;
		} else if ('G' == ch) {
			color = COLOR_GREEN;
		} else if ('Y' == ch) {
			color = COLOR_YELLOW;
		} else {
			--str;
		}
	}
}

template<class Elem>
unsigned short colorostreamT<Elem>::GetCurrentTextColor()
{
#if OS_WINDOWS && !OS_WINDOWS_MOBILE
	const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	// Gets the current text color.
	CONSOLE_SCREEN_BUFFER_INFO buffer_info;
	GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
	return buffer_info.wAttributes;
#else
	return 0;
#endif
}

template<class Elem>
void colorostreamT<Elem>::RestoreTextColor(unsigned short old_color_attrs)
{
#if OS_WINDOWS && !OS_WINDOWS_MOBILE
	const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	// Restores the text color.
	SetConsoleTextAttribute(stdout_handle, old_color_attrs);
#else
	printf("\033[m");  // Resets the terminal to default.
#endif
}

template<class Elem>
void colorostreamT<Elem>::SetForegroundConsoleTextColor(Color color)
{
#if OS_WINDOWS && !OS_WINDOWS_MOBILE
	const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	// We need to flush the stream buffers into the console before each
	// SetConsoleTextAttribute call lest it affect the text that is already
	// printed but has not yet reached the console.
	fflush(stdout);
	SetConsoleTextAttribute(stdout_handle,
		GetForegroundColorAttribute(color));
#else
	printf("\033[0;%sm", GetForegroundColorAttribute(color));
#endif  // OS_WINDOWS && !OS_WINDOWS_MOBILE
}

template<class Elem>
void colorostreamT<Elem>::SetBackgroundConsoleTextColor(Color color)
{
#if OS_WINDOWS && !OS_WINDOWS_MOBILE
	const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	// We need to flush the stream buffers into the console before each
	// SetConsoleTextAttribute call lest it affect the text that is already
	// printed but has not yet reached the console.
	fflush(stdout);
	SetConsoleTextAttribute(stdout_handle,
		GetBackgroundColorAttribute(color));
#else
	printf("\033[0;%sm", GetBackgroundColorAttribute(color));
#endif  // OS_WINDOWS && !OS_WINDOWS_MOBILE
}


template<class Elem>
inline std::basic_ostream<Elem, std::char_traits<Elem> >& dft(std::basic_ostream<Elem, std::char_traits<Elem> > &out)
{
	out.flush();
	return out;
}

//////////////////////////////////////////////////////////////////////////
/// foreground color
template<class Elem>
inline std::basic_ostream<Elem, std::char_traits<Elem> >& fgred(std::basic_ostream<Elem, std::char_traits<Elem> > &out)
{
	out.flush();
	colorostreamT<Elem>::SetForegroundConsoleTextColor(colorostreamT<Elem>::COLOR_RED);
	return out;
}

template<class Elem>
inline std::basic_ostream<Elem, std::char_traits<Elem> >& fggreen(std::basic_ostream<Elem, std::char_traits<Elem> > &out)
{
	out.flush();
	colorostreamT<Elem>::SetForegroundConsoleTextColor(colorostreamT<Elem>::COLOR_GREEN);
	return out;
}

template<class Elem>
inline std::basic_ostream<Elem, std::char_traits<Elem> >& fgblue(std::basic_ostream<Elem, std::char_traits<Elem> > &out)
{
	out.flush();
	colorostreamT<Elem>::SetForegroundConsoleTextColor(colorostreamT<Elem>::COLOR_BLUE);
	return out;
}

template<class Elem>
inline std::basic_ostream<Elem, std::char_traits<Elem> >& fgyellow(std::basic_ostream<Elem, std::char_traits<Elem> > &out)
{
	out.flush();
	colorostreamT<Elem>::SetForegroundConsoleTextColor(colorostreamT<Elem>::COLOR_YELLOW);
	return out;
}

//////////////////////////////////////////////////////////////////////////
/// background color
template<class Elem>
inline std::basic_ostream<Elem, std::char_traits<Elem> >& bgred(std::basic_ostream<Elem, std::char_traits<Elem> > &out)
{
	out.flush();
	colorostreamT<Elem>::SetBackgroundConsoleTextColor(colorostreamT<Elem>::COLOR_RED);
	return out;
}

template<class Elem>
inline std::basic_ostream<Elem, std::char_traits<Elem> >& bggreen(std::basic_ostream<Elem, std::char_traits<Elem> > &out)
{
	out.flush();
	colorostreamT<Elem>::SetBackgroundConsoleTextColor(colorostreamT<Elem>::COLOR_GREEN);
	return out;
}

template<class Elem>
inline std::basic_ostream<Elem, std::char_traits<Elem> >& bgblue(std::basic_ostream<Elem, std::char_traits<Elem> > &out)
{
	out.flush();
	colorostreamT<Elem>::SetBackgroundConsoleTextColor(colorostreamT<Elem>::COLOR_BLUE);
	return out;
}

template<class Elem>
inline std::basic_ostream<Elem, std::char_traits<Elem> >& bgyellow(std::basic_ostream<Elem, std::char_traits<Elem> > &out)
{
	out.flush();
	colorostreamT<Elem>::SetBackgroundConsoleTextColor(colorostreamT<Elem>::COLOR_YELLOW);
	return out;
}

#ifdef _WIN32

//////////////////////////////////////////////////////////////////////////
/// foreground color
inline std::ostream& fgwhite(std::ostream &out)
{
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);			
	SetConsoleTextAttribute(hStdout, 
							FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);	//R,G,B混合就为白色
	return out;
}

inline std::ostream& fgblack(std::ostream &out)
{
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
	SetConsoleTextAttribute(hStdout, FOREGROUND_INTENSITY);
	return out;
}


inline std::ostream& fgmagenta(std::ostream &out)
{
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
	SetConsoleTextAttribute(hStdout, 
							FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	return out;
}

inline std::ostream& fgcyan(std::ostream &out)
{
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
	SetConsoleTextAttribute(hStdout, 
							FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	return out;
}

inline std::ostream& fggray(std::ostream &out)
{
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
	SetConsoleTextAttribute(hStdout, 
							FOREGROUND_INTENSITY | FOREGROUND_INTENSITY);
	return out;
}

//////////////////////////////////////////////////////////////////////////
/// background color
inline std::ostream& bgwhite(std::ostream &out)
{
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);			//获取标准输出句柄
	SetConsoleTextAttribute(hStdout, 
							BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY);	//R,G,B混合就为白色
	return out;
}

inline std::ostream& bgblack(std::ostream &out)
{
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
	SetConsoleTextAttribute(hStdout, BACKGROUND_INTENSITY);
	return out;
}

inline std::ostream& bgmagenta(std::ostream &out)
{
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
	SetConsoleTextAttribute(hStdout, 
							BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY);
	return out;
}

inline std::ostream& bgcyan(std::ostream &out)
{
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
	SetConsoleTextAttribute(hStdout, 
							BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY);
	return out;
}

inline std::ostream& bggray(std::ostream &out)
{
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
	SetConsoleTextAttribute(hStdout, 
							BACKGROUND_INTENSITY | BACKGROUND_INTENSITY);
	return out;
}

#endif

UTIL_END

#endif