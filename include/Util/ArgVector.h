// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_ARGVECTOR_H
#define UTIL_ARGVECTOR_H

#include <string>
#include <Util/Config.h>
#include <Unicoder/StringConverter.h>


namespace Util
{

class UTIL_API ArgVector
{
public:

    ArgVector(int argc, char* const argv[]);
    ArgVector(const StringSeq&);
    ArgVector(const ArgVector&);
    ArgVector& operator =(const ArgVector&);
    ~ArgVector();

    int m_argc;
    char** m_argv;

private:

    StringSeq m_args;
    void SetupArgcArgv();
};

}

#endif
