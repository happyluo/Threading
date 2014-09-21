// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_DISABLEWARNINGS_H
#define UTIL_DISABLEWARNINGS_H

//
// This header file disables various annoying compiler warnings that
// we don't want.
//
// IMPORTANT: Do *not* include this header file in another public header file!
//            Doing this may potentially disable the warnings in the source
//            code of our customers, which would be bad. Only include this
//            header file in Ice *source* files!
//

#if defined(_MSC_VER)
#    define _CRT_SECURE_NO_DEPRECATE 1  // C4996 '<C function>' was declared deprecated/
#    pragma warning( 4 : 4996 ) // C4996 'std::<function>' was declared deprecated
#    pragma warning( 4 : 4800 ) // C4800 forcing value to bool 'true' or 'false' (performance warning)

#    if (_MSC_VER < 1700)
#       pragma warning( 4 : 4355 ) // C4355 'this' : used in base member initializer list
#    endif
#endif

#endif
