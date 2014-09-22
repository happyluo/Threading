// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_NUMERICAL_UTIL_H
#define UTIL_NUMERICAL_UTIL_H

#include <vector>
#include <cfloat>
#include <limits.h>        // for CHAR_BIT 
#include <Util/Config.h>

#ifdef  _MSC_VER
#	pragma warning(push, 3)
#	pragma warning(disable: 4307)
#endif  // _MSC_VER 


#define BITMASK(b)		(1 << ((b) % CHAR_BIT))
#define BITSLOT(b)		((b) / CHAR_BIT)
#define BITSET(a, b)	((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b)	((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b)	((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb)	((nb + CHAR_BIT - 1) / CHAR_BIT)

UTIL_BEGIN

//
// Max
// 
class NumericLimits
{
public:
	template<typename T>
	static inline T Max()
	{
		STATIC_ASSERT(UtilInternal::IsIntegral<T>::value);

		size_t bitsperword = sizeof (T) * 8;

		return static_cast<T>(1 << (bitsperword - 1)) < 0
			? ~(static_cast<T>(1) << (bitsperword - 1))
			: ~static_cast<T>(0);
	}

	template<>
	static inline float Max<float>()
	{
		int sign      = 0; 
		int mantissa  = 0xfe << 23;
		int exponent  = 0x7fffff;

		int ret(sign | mantissa | exponent);
		float fret = *reinterpret_cast<float*>(&ret);
		return *reinterpret_cast<float*>(&ret);
		//return FLT_MAX;
	}

	template<>
	static inline double Max<double>()
	{
		Int64 sign      = 0; 
		Int64 mantissa  = (Int64)0x7fe << 52;
		Int64 exponent  = 0xfffffffffffff;

		Int64 ret(sign | mantissa | exponent);
		double fret = *reinterpret_cast<double*>(&ret);
		return *reinterpret_cast<double*>(&ret);
		//return DBL_MAX; 
	}

	//
	// Min
	// 
	template<typename T>
	static inline T Min()
	{
		STATIC_ASSERT(UtilInternal::IsIntegral<T>::value);

		size_t bitsperword = sizeof (T) * 8;

		return T(1 << (bitsperword - 1)) < 0 
			? T(1) << (bitsperword - 1)
			: 0;
	}

	template<>
	static inline float Min<float>()
	{
		int sign      = 0;	//1 << 31
		int mantissa  = 0x1 << 23;
		int exponent  = 0;

		int ret(sign | mantissa | exponent);
		float fret = *reinterpret_cast<float*>(&ret);
		return *reinterpret_cast<float*>(&ret);
		//return FLT_MIN;
	}

	template<>
	static inline double Min<double>()
	{
		Int64 sign      = 0;	//1 << 61
		Int64 mantissa  = (Int64)0x1 << 52;
		Int64 exponent  = 0;

		Int64 ret(sign | mantissa | exponent);
		double fret = *reinterpret_cast<double*>(&ret);
		return *reinterpret_cast<double*>(&ret);
		//return DBL_MIN; 
	}
};

inline bool IsFinite(const double& number) 
{  
#if defined(_WIN32)  
	return _finite(number) != 0;  
#else
	return finite(number) != 0; 
#endif  
}  

inline bool IsNaN(double value)
{
	// NaN is never equal to anything, even itself.
	return value != value;
}

//
// greatest common divisor
//
static int GCD(int lhs, int rhs)
{
	while (0 != lhs)
	{
		if (rhs >= lhs)
		{
			rhs -= lhs;
		}
		else 
		{
			std::swap(lhs, rhs);
		}
	}
	return rhs;
}

// greatest common divisor
static int ISOGCD(int lhs, int rhs)
{	
	if (lhs == 0) return rhs;
	if (rhs == 0) return lhs;

	while (lhs != rhs) 
	{
		if (lhs > rhs)
		{
			lhs -= rhs;
		}
		else 
		{
			rhs -= lhs;
		}
	}
	return lhs;
}

//
// LookupTable
//
template<typename T, size_t bitsperword = sizeof(T) * 8>
class LookupTable
{
public:
	LookupTable(size_t table_size) : 
		m_mask(bitsperword - 1)
	{
		m_shift = BitCount(m_mask);
		m_table.swap(std::vector<T>((table_size >> m_shift) + 1, 0));
	}

	void Set(unsigned long num)
	{
		assert(m_table.size() > num >> m_shift);
		m_table[num >> m_shift] |= T(1) << (num & m_mask);
	}

	void Clear(unsigned long num)
	{
		assert(m_table.size() > num >> m_shift);
		m_table[num >> m_shift] &= ~(T(1) << (num & m_mask));
	}

	bool Test(unsigned long num)
	{
		assert(m_table.size() > num >> m_shift);
		return m_table[num >> m_shift] & T(1) << (num & m_mask);
	}

	void Zero()
	{
		m_table.swap(std::vector<T>(m_table.size(), 0));
	}

private:
	std::vector<T>	m_table;
	const size_t m_mask;
	size_t m_shift;

};

static inline unsigned 
BitCount(unsigned long num)
{
	//
	// 1. By RightShift
	//
	unsigned ucount = 0;

	//do 
	//{
	//	ucount += num & 1 ? 1 : 0;
	//} while (num >>= 1);

	//////////////////////////////////////////////////////////////////////////

	//
	// 2. By Subtraction
	//

	while (num)
	{
		++ucount;
		num &= num - 1;		// each time of this operation clear the lowest nonzero bit.
	}

	//for (ucount = 0; num; ucount++)
	//{
	//	num &= num - 1;		// clear the least significant bit set
	//}

	return ucount;

	
}

static inline bool 
IsPowOf2(unsigned num) 
{
	return !(num & (num - 1));
}

///
/// 一个 32bit 整数，取不小于它的 2 的整数次幂 
/// 
static inline unsigned
NextPowOf2(unsigned num) 
{
	if (IsPowOf2(num))
	{
		return num;
	}

	//
	// assume the highest nonzero bit of 'num' is n, following operation set bits in [n-1, 0] to 1.
	// 
	num |= num >> 1;	// set n-1 bit
	num |= num >> 2;	// set n-2 to n-3 bits.
	num |= num >> 4;	// set n-4 to n-7 bits.
	num |= num >> 8;	// set n-8 to n-15 bits.
	num |= num >> 16;	// set n-16 to n-31 bits.

	return num + 1;
}

// Returns the integer i such as 2^i <= n < 2^(i+1)
inline int Log2Floor(uint32 n)
{
	if (n == 0)
	{
		return -1;
	}
	int log = 0;
	uint32 value = n;
	for (int i = 4; i >= 0; --i)
	{
		int shift = (1 << i);
		uint32 x = value >> shift;
		if (x != 0)
		{
			value = x;
			log += shift;
		}
	}
	
	//UTIL_DCHECK_EQ(value, 1u);
	assert(value == 1u);
	return log;
}

// Returns the integer i such as 2^(i-1) < n <= 2^i
inline int Log2Ceiling(uint32 n)
{
	if (n == 0)
	{
		return -1;
	} 
	else 
	{
		// Log2Floor returns -1 for 0, so the following works correctly for n=1.
		return 1 + Log2Floor(n - 1);
	}
}

union Cast { double m_double; long m_long; };

static inline long DoubleToLong(double num)
{
	// 
	// 6755399441055744 --> 0x18 00 00 00 00 00 00 (与浮点数1.1的尾数对应,在二进制表示中小数点前的 1 是不需要记录的)
	// 4503599627370496 --> 0x10 00 00 00 00 00 00	(与浮点数1.0的尾数对应)
	// 
	// 在进行浮点数的加法时，num 为了与 6755399441055744 对阶(小阶向大阶看齐), 尾数必须右移,
	// 最终尾数全部移入与 long 类型对应的低四字节(double 是64位的)，在union将double对应的内存解释为long时会得到正确的值
	//
	volatile union Cast cast;
	cast.m_double = num + 6755399441055744.0;		
	return cast.m_long;
}

//
//	对于单精度（single precision）：单精度浮点数位长：32位
//	
//	（1）	IEEE 754 标准规定：第1位为符号位，1 代表负，0代表正
//	（2）	接下来用8位来表示指数部分。
//	（3）	接下来的23位用来表示有效数位
//	
//	0       0 0 0 0 0 0 0 0      0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0
//	-       ---------------      -----------------------------------------------
//	S         指数（8位）              有效数位 （23 位）
//
static inline long FloatToLong(float num)
{
	int a         = *(int*)(&num);
	int sign      = (a >> 31); 
	int mantissa  = (a & ((1 << 23) - 1)) | (1 << 23);
	int exponent  = ((a & 0x7fffffff) >> 23) - 127;
	int r         = ((unsigned int)(mantissa) << 8) >> (31 - exponent);
	return ((r ^ (sign)) - sign ) &~ (exponent >> 31);    	
}

UTIL_END

#ifdef _MSC_VER
#	pragma warning(default: 4307)
#	pragma warning(pop)
#endif

#endif