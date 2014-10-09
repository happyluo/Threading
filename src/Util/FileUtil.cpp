// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************


#include <Util/DisableWarnings.h>
#include <Util/FileUtil.h>
#include <Util/StringUtil.h>
#include <Unicoder/Unicode.h>
#include <Util/Exception.h>
#include <Logging/Logger.h>
#include <climits>
#include <string.h>

#ifdef _WIN32
#  include <process.h>
#  include <io.h>
#else
#  include <unistd.h>
#endif

using namespace std;

//THREADING_BEGIN

//
// Determine if path is an absolute path
//
bool
Threading::IsAbsolutePath(const string& path)
{
    size_t i = 0;
    size_t size = path.size();

    // Skip whitespace
    while (i < size && isspace(static_cast<unsigned char>(path[i])))
    {
        ++i;
    }

#ifdef _WIN32
    // We need at least 3 non whitespace character to have
    // and absolute path
    if (i + 3 > size)
    {
        return false;
    }

    // Check for X:\ path ('\' may have been converted to '/')
    if ((path[i] >= 'A' && path[i] <= 'Z') || (path[i] >= 'a' && path[i] <= 'z'))
    {
        return path[i + 1] == ':' && (path[i + 2] == '\\' || path[i + 2] == '/');
    }

    // Check for UNC path
    return (path[i] == '\\' && path[i + 1] == '\\') || path[i] == '/';
#else
    if (i >= size)
    {
        return false;
    }

    return path[i] == '/';
#endif
}

//
// Determine if a directory exists.
//
bool
Threading::DirectoryExists(const string& path)
{
    Threading::structstat st;
    if (Threading::stat(path, &st) != 0 || !S_ISDIR(st.st_mode))
    {
        return false;
    }
    return true;
}

//
// Replace "/" by "\"
//
inline string
Threading::FixDirSeparator(const string& path)
{
    string result = path;
    size_t pos = 0;
    while ((pos = result.find('/', pos)) != string::npos)
    {
        result[pos] = '\\';
        pos++;
    }

    return result;
}

bool Threading::Exists(const string& name)
{
    return access(name.c_str(), 0/*F_OK*/) == 0;
}

bool Threading::ReadFileToString(const string& name, string* output)
{
    char buffer[1024];
    FILE* file = fopen(name.c_str(), "rb");
    if (file == NULL) return false;

    while (true) 
    {
        size_t n = fread(buffer, 1, sizeof(buffer), file);
        if (n <= 0) break;
        output->append(buffer, n);
    }

    int error = ferror(file);
    if (fclose(file) != 0) return false;
    return error == 0;
}

void Threading::ReadFileToStringOrDie(const string& name, string* output)
{
    UTIL_CHECK(ReadFileToString(name, output)) << "Could not read: " << name;
}

void Threading::WriteStringToFileOrDie(const string& contents, const string& name)
{
    FILE* file = fopen(name.c_str(), "wb");
    UTIL_CHECK(file != NULL)
        << "fopen(" << name << ", \"wb\"): " << strerror(errno);
    UTIL_CHECK_EQ(fwrite(contents.data(), 1, contents.size(), file),
        contents.size())
        << "fwrite(" << name << "): " << strerror(errno);
    UTIL_CHECK(fclose(file) == 0)
        << "fclose(" << name << "): " << strerror(errno);
}

//
// Determine if a regular file exists.
//
bool
Threading::FileExists(const string& path)
{
    Threading::structstat st;
    if (Threading::stat(path, &st) != 0 || !S_ISREG(st.st_mode))
    {
        return false;
    }
    return true;
}

#ifdef _WIN32

//
// Stat
//
int
Threading::stat(const string& path, structstat* buffer)
{
    return _wstat(Threading::StringToWstring(path).c_str(), buffer);
}

int
Threading::remove(const string& path)
{
    return ::_wremove(Threading::StringToWstring(path).c_str());
}

int
Threading::rename(const string& from, const string& to)
{
    return ::_wrename(Threading::StringToWstring(from).c_str(), Threading::StringToWstring(to).c_str());
}

int
Threading::rmdir(const string& path)
{
    return ::_wrmdir(Threading::StringToWstring(path).c_str());
}

int
Threading::mkdir(const string& path, int)
{
    return ::_wmkdir(Threading::StringToWstring(path).c_str());
}

FILE*
Threading::fopen(const string& path, const string& mode)
{
    return ::_wfopen(Threading::StringToWstring(path).c_str(), Threading::StringToWstring(mode).c_str());
}

int
Threading::open(const string& path, int flags)
{
    if (flags & _O_CREAT)
    {
        return ::_wopen(Threading::StringToWstring(path).c_str(), flags, _S_IREAD | _S_IWRITE);
    }
    else
    {
        return ::_wopen(Threading::StringToWstring(path).c_str(), flags);
    }
}

#ifndef OS_WINRT
int
Threading::getcwd(string& cwd)
{
    wchar_t cwdbuf[_MAX_PATH];
    if (_wgetcwd(cwdbuf, _MAX_PATH) == NULL)
    {
        return -1;
    }
    cwd = Threading::WstringToString(cwdbuf);
    return 0;
}
#endif

int
Threading::unlink(const string& path)
{
    return _wunlink(Threading::StringToWstring(path).c_str());
}

int
Threading::close(int fd)
{
#ifdef __MINGW32__
        return _close(fd);
#else
        return ::close(fd);
#endif
}

Threading::FileLock::FileLock(const std::string& path) :
    m_fd(INVALID_HANDLE_VALUE),
    m_path(path)
{
#ifndef OS_WINRT
    m_fd = ::CreateFileW(Threading::StringToWstring(path).c_str(), GENERIC_WRITE, 0, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#else
    CREATEFILE2_EXTENDED_PARAMETERS params;
    params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    m_fd = ::CreateFile2(Threading::StringToWstring(path).c_str(), GENERIC_WRITE, 0,
                        OPEN_ALWAYS, &params);
#endif
    m_path = path;

    if (m_fd == INVALID_HANDLE_VALUE)
    {
        throw Threading::FileLockException(__FILE__, __LINE__, GetLastError(), m_path);
    }

#ifdef __MINGW32__
    if (::LockFile(m_fd, 0, 0, 0, 0) == 0)
    {
        throw Threading::FileLockException(__FILE__, __LINE__, GetLastError(), m_path);
    }
#else
    OVERLAPPED overlaped;
    overlaped.Internal = 0;
    overlaped.InternalHigh = 0;
    overlaped.Offset = 0;
    overlaped.OffsetHigh = 0;

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
    overlaped.hEvent = nullptr;
#else
    overlaped.hEvent = 0;
#endif

    if (::LockFileEx(m_fd, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 0, 0, &overlaped) == 0)
    {
        ::CloseHandle(m_fd);
        throw Threading::FileLockException(__FILE__, __LINE__, GetLastError(), m_path);
    }
#endif
    //
    // In Windows implementation we don't write the process pid to the file, as it is 
    // not possible to read the file from other process while it is locked here.
    //
}

Threading::FileLock::~FileLock()
{
    assert(m_fd != INVALID_HANDLE_VALUE);
    CloseHandle(m_fd);
    unlink(m_path);
}

Threading::ifstream::ifstream()
{
}

Threading::ifstream::ifstream(const string& path, ios_base::openmode mode) : 
#ifdef  __MINGW32__
    std::ifstream(path.c_str(), mode)
#else
    std::ifstream(Threading::StringToWstring(path).c_str(), mode)
#endif
{
}

void
Threading::ifstream::open(const string& path, ios_base::openmode mode)
{
#ifdef  __MINGW32__
    std::ifstream::open(path.c_str(), mode);
#else
    std::ifstream::open(Threading::StringToWstring(path).c_str(), mode);
#endif
}


Threading::ofstream::ofstream()
{
}

Threading::ofstream::ofstream(const string& path, ios_base::openmode mode) : 
#ifdef __MINGW32__
    std::ofstream(path.c_str(), mode)
#else
    std::ofstream(Threading::StringToWstring(path).c_str(), mode)
#endif
{
}

void
Threading::ofstream::open(const string& path, ios_base::openmode mode)
{
#ifdef __MINGW32__
    std::ofstream::open(path.c_str(), mode);
#else
    std::ofstream::open(Threading::StringToWstring(path).c_str(), mode);
#endif
}


#else

//
// Stat
//
int
Threading::stat(const string& path, structstat* buffer)
{
    return ::stat(path.c_str(), buffer);
}

int
Threading::remove(const string& path)
{
    return ::remove(path.c_str());
}

int
Threading::rename(const string& from, const string& to)
{
    return ::rename(from.c_str(), to.c_str());
}

int
Threading::rmdir(const string& path)
{
    return ::rmdir(path.c_str());
}

int
Threading::mkdir(const string& path, int perm)
{
    return ::mkdir(path.c_str(), perm);
}

FILE*
Threading::fopen(const string& path, const string& mode)
{
    return ::fopen(path.c_str(), mode.c_str());
}

int
Threading::open(const string& path, int flags)
{
    if (flags & O_CREAT)
    {
        // By default, create with rw-rw-rw- modified by the user's umask (same as fopen).
        return ::open(path.c_str(), flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    }
    else
    {
        return ::open(path.c_str(), flags);
    }
}

int
Threading::getcwd(string& cwd)
{
    char cwdbuf[PATH_MAX];
    if (::getcwd(cwdbuf, PATH_MAX) == NULL)
    {
        return -1;
    }
    cwd = cwdbuf;
    return 0;
}

int
Threading::unlink(const string& path)
{
    return ::unlink(path.c_str());
}

int
Threading::close(int fd)
{
    return ::close(fd);
}

Threading::FileLock::FileLock(const std::string& path) :
    m_fd(-1),
    m_path(path)
{
    m_fd = ::open(path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (m_fd < 0)
    {
        throw Threading::FileLockException(__FILE__, __LINE__, errno, m_path);
    }

    struct ::flock lock;
    lock.l_type = F_WRLCK; // Write lock
    lock.l_whence = SEEK_SET; // Begining of file
    lock.l_start = 0;
    lock.l_len = 0;
    
    //
    // F_SETLK tells fcntl to not block if it cannot 
    // acquire the lock, if the lock cannot be acquired 
    // it returns -1 without wait.
    //
    if (::fcntl(m_fd, F_SETLK, &lock) == -1)
    {
        Threading::FileLockException ex(__FILE__, __LINE__, errno, m_path);
        close(m_fd);
        throw ex;
    }

    //
    // If there is an error after here, we close the fd,
    // to release the lock.
    //
    
    //
    // Now that we have acquire an excluxive write lock,
    // write the process pid there.
    //
    ostringstream os;
    os << getpid();
    
    if (write(m_fd, os.str().c_str(), os.str().size()) == -1)
    {
        Threading::FileLockException ex(__FILE__, __LINE__, errno, m_path);
        close(m_fd);
        throw ex;
    }
}

Threading::FileLock::~FileLock()
{
    assert(m_fd > -1);
    unlink(m_path);
}

Threading::ifstream::ifstream()
{
}

Threading::ifstream::ifstream(const string& path, ios_base::openmode mode) : std::ifstream(path.c_str(), mode)
{
}

void
Threading::ifstream::open(const string& path, ios_base::openmode mode)
{
    std::ifstream::open(path.c_str(), mode);
}

Threading::ofstream::ofstream()
{
}

Threading::ofstream::ofstream(const string& path, ios_base::openmode mode) : std::ofstream(path.c_str(), mode)
{
}

void
Threading::ofstream::open(const string& path, ios_base::openmode mode)
{
    std::ofstream::open(path.c_str(), mode);
}

#endif

long 
Threading::ifstream::length()
{
    if (is_open())
    {
        ifstream::pos_type curpos = tellg();
        // get length of file:
        seekg(0, ios::end);
        ifstream::pos_type flen = tellg() - curpos;
        seekg(curpos);

        return static_cast<long>(flen);
    }

    return -1;
}

std::string 
Threading::ifstream::load()
{
    long flength = length();
    if (0 != flength)
    {
        string result;
        result.resize(flength);

        // read data as a block:
        read(&result[0], flength);
        
        assert(flength == gcount() || eof());
        result.resize(gcount());

        return result;
    }

    return "";
}

void 
Threading::ifstream::reset()
{
    if (is_open())
    {
        close();
        clear();
    }
}

long Threading::length(const std::string& path)
{
    Threading::structstat st;
    if (0 == Threading::stat(path, &st) && S_ISREG(st.st_mode))
    {
        return st.st_size;
    }
    return -1;
}

#ifdef _WIN32
long Threading::length(int fd)
{
    return ::filelength(fd);
}

std::string Threading::fload(int fd)
{
    long flength = length(fd);
    if (flength <= 0)
    {
        return "";
    }

    string result;
    result.resize(flength);

    if (flength != read(fd, &result[0], flength))
    {
        return "";
    }

    return Threading::TranslatingCR2LF(result);
}

#endif

// Get the file size, so we can pre-allocate the string. HUGE speed impact.
long Threading::length(FILE* file)
{
    if (NULL == file)
    {
        return -1;
    }

#ifdef _WIN32
    return length(fileno(file));
#else
    int curpos = fseek(file, 0, SEEK_CUR);
    fseek(file, 0, SEEK_END);
    long flength = ftell(file) - curpos;
    fseek(file, curpos, SEEK_SET);
    return flength;
#endif
}

std::string Threading::fload(const std::string& path)
{
    FILE* file = fopen(path.c_str(), "rb");
    if (NULL == file)
    {
        return "";
    }

    long flength = length(file);
    if (flength <= 0)
    {
        return "";
    }

    string content;
    content.resize(flength);

    if (1 != fread(&content[0], flength, 1, file))
    {
        fclose(file);
        return "";
    }

    fclose(file);

    return Threading::TranslatingCR2LF(content);
}

std::string Threading::fload(FILE* file)
{
    const size_t file_size = length(file);
    char* const buffer = new char[file_size];

    size_t bytes_last_read = 0;  // # of bytes read in the last fread()
    size_t bytes_read = 0;       // # of bytes read so far

    // Keeps reading the file until we cannot read further or the
    // pre-determined file size is reached.
    do 
    {
        bytes_last_read = fread(buffer + bytes_read, 1, file_size - bytes_read, file);
        bytes_read += bytes_last_read;
    } while (bytes_last_read > 0 && bytes_read < file_size);

    const std::string content(buffer, bytes_read);
    delete[] buffer;

    return Threading::TranslatingCR2LF(content);
}

void  Threading::redirection(const std::string& filename, 
                                const Threading::StringConverterPtr& converter, 
                                FILE* oldfile/* = stdout | stderr*/)
{
    if ("" != filename)
    {
#ifdef _WIN32
        FILE* file = _wfreopen(Threading::StringToWstring(
            NativeToUTF8(converter, filename)).c_str(), L"a", oldfile);
#else
        FILE* file = freopen(filename.c_str(), "a", oldfile);
#endif
        if (0 == file)
        {
            Threading::FileException ex(__FILE__, __LINE__, Threading::GetSystemErrno(), filename);
            throw ex;
        }
    }
}

std::vector<std::vector<std::string> > 
Threading::LoadCSVFile(const string& csvfile, const string& separator)
{
    Threading::ifstream in(csvfile);
    if (!in)
    {
        throw Threading::FileException(__FILE__, __LINE__, Threading::GetSystemErrno(), csvfile);
    }

    std::vector<std::vector<std::string> > retvec;
    string line;
    bool firstLine = true;
    while (getline(in, line))
    {
        //
        // Skip UTF8 BOM if present.
        //
        if (firstLine)
        {
            const unsigned char UTF8_BOM[3] = {0xEF, 0xBB, 0xBF}; 
            if (line.size() >= 3 &&
                static_cast<const unsigned char>(line[0]) == UTF8_BOM[0] &&
                static_cast<const unsigned char>(line[1]) == UTF8_BOM[1] && 
                static_cast<const unsigned char>(line[2]) == UTF8_BOM[2])
            {
                line = line.substr(3);
            }
            firstLine = false;
        }
        
        if (line.empty() || '#' == line[0] || 
            ('[' == line[0] && ']' == line[line.size() - 1]))
        {
            continue;
        }

        vector<string> colms;
        Threading::SplitString(line, separator, colms, true);
        retvec.push_back(colms);
    }

    return retvec;
}

//THREADING_END