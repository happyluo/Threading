// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************


#ifndef UTIL_FILE_UTIL_INTERNAL_H
#define UTIL_FILE_UTIL_INTERNAL_H

#include <Util/Config.h>
#include <Util/Shared.h>
#include <Unicoder/StringConverter.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <fstream>
#include <vector>

namespace UtilInternal
{

//
// Detemine if path is an absolute path.
//
UTIL_API bool IsAbsolutePath(const std::string& path);

//
// Determine if a file exists.
//
UTIL_API bool FileExists(const std::string& path);

//
// Determine if a directory exists.
//
UTIL_API bool DirectoryExists(const std::string& path);

//
// Replace "/" by "\"
//
UTIL_API std::string FixDirSeparator(const std::string& path);

// Check if the file exists.
UTIL_API  bool Exists(const std::string& name);

// Read an entire file to a std::string.  Return true if successful, false
// otherwise.
UTIL_API  bool ReadFileToString(const std::string& name, std::string* output);

// Same as above, but crash on failure.
UTIL_API  void ReadFileToStringOrDie(const std::string& name, std::string* output);

// Create a file and write a std::string to it.
UTIL_API  void WriteStringToFileOrDie(const std::string& contents,
								   const std::string& name);

#ifdef _WIN32

#if defined(__MINGW32__)
typedef struct _stat structstat;
#else
typedef struct _stat64i32 structstat;
#endif

#ifdef _MSC_VER
#   define O_RDONLY _O_RDONLY
#   define O_BINARY _O_BINARY

#   define S_ISDIR(mode) ((mode) & _S_IFDIR)
#   define S_ISREG(mode) ((mode) & _S_IFREG)
#endif

#else

typedef struct stat structstat;
#   define O_BINARY 0

#endif

//
// OS stat
//
UTIL_API int stat(const std::string& path, structstat* buffer);
UTIL_API int remove(const std::string& path);
UTIL_API int rename(const std::string& from, const std::string& to);
UTIL_API int rmdir(const std::string& path);

UTIL_API int mkdir(const std::string& path, int);
UTIL_API FILE* fopen(const std::string& path, const std::string& mode);
UTIL_API int open(const std::string& path, int flags);

#ifndef OS_WINRT
UTIL_API int getcwd(std::string& cwd);		// current work directory.
#endif

UTIL_API int unlink(const std::string& path);
UTIL_API int close(int fd);

// get file length.
UTIL_API long length(const std::string& path);	
UTIL_API long length(int fd);		// file handle
UTIL_API long length(FILE* file);

// load file contents.
UTIL_API std::string fload(const std::string& path);
UTIL_API std::string fload(int fd);
UTIL_API std::string fload(FILE* file);

UTIL_API void redirection(const std::string& filename, 
						  const Util::StringConverterPtr& converter, 
						  FILE* oldfile = stdout/*stderr*/);

UTIL_API std::vector<std::vector<std::string> > LoadCSVFile(const std::string& csvfile, const std::string& separator);

//
// This class is used to implement process file locking. This class
// is not intended to do file locking within the same process.
//
class UTIL_API FileLock : public Util::Shared, public Util::noncopyable
{
public:
    //
    // The constructor opens the given file (eventually creating it)
    // and acquires a lock on the file or throws FileLockException if
    // the file couldn't be locked.
    //
    // If the lock can be acquired, the process pid is written to the
    // file.
    //
    FileLock(const std::string& path);
    
    //
    // The destructor releases the lock and removes the file.
    //
    virtual ~FileLock();
    
private:
    
#ifdef _WIN32
    HANDLE m_fd;
#else
    int m_fd;
#endif
    std::string m_path;
};

typedef Util::SharedPtr<FileLock> FileLockPtr;

class UTIL_API ifstream : public std::ifstream
{
public:

    ifstream();
    ifstream(const std::string&, std::ios_base::openmode mode = std::ios_base::in);
    void open(const std::string&, std::ios_base::openmode mode = std::ios_base::in);
	long length();
	std::string load();
	void reset();

#ifdef __SUNPRO_CC
    using std::ifstream::open;
#endif

private:

    // Hide const char* definitions since they shouldn't be used.
    ifstream(const char*);
    void open(const char*, std::ios_base::openmode mode = std::ios_base::in);
};

class UTIL_API ofstream : public std::ofstream
{
public:

    ofstream();
    ofstream(const std::string&, std::ios_base::openmode mode = std::ios_base::out);
    void open(const std::string&, std::ios_base::openmode mode = std::ios_base::out);

#ifdef __SUNPRO_CC
    using std::ofstream::open;
#endif

private:

    // Hide const char* definitions since they shouldn't be used.
    ofstream(const char*);
    void open(const char*, std::ios_base::openmode mode = std::ios_base::out);
};

}
#endif
