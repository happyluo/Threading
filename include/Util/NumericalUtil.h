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
#include <Config.h>

#define BITMASK(b)      (1 << ((b) % CHAR_BIT))
#define BITSLOT(b)      ((b) / CHAR_BIT)
#define BITSET(a, b)    ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b)  ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b)   ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb)   ((nb + CHAR_BIT - 1) / CHAR_BIT)

THREADING_BEGIN

//
// Max
// 
class NumericLimits
{
public:
    template<typename T>
    static inline T Max()
    {
        STATIC_ASSERT(Threading::IsIntegral<T>::value);

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
        STATIC_ASSERT(Threading::IsIntegral<T>::value);

        size_t bitsperword = sizeof (T) * 8;

        return T(1 << (bitsperword - 1)) < 0 
            ? T(1) << (bitsperword - 1)
            : 0;
    }

    template<>
    static inline float Min<float>()
    {
        int sign      = 0;    //1 << 31
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
        Int64 sign      = 0;    //1 << 61
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
    std::vector<T>    m_table;
    const size_t m_mask;
    size_t m_shift;

};

static inline bool IsPowOf2(unsigned num) 
{
    return !(num & (num - 1));
}

static inline unsigned NextPowOf2(unsigned num) 
{
    if (IsPowOf2(num))
    {
        return num;
    }

    //
    // assume the highest nonzero bit of 'num' is n, following operation set bits in [n-1, 0] to 1.
    // 
    num |= num >> 1;    // set n-1 bit
    num |= num >> 2;    // set n-2 to n-3 bits.
    num |= num >> 4;    // set n-4 to n-7 bits.
    num |= num >> 8;    // set n-8 to n-15 bits.
    num |= num >> 16;    // set n-16 to n-31 bits.

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

THREADING_END

#endif