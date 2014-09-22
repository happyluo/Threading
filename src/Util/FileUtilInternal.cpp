// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************


#include <Util/DisableWarnings.h>
#include <Util/FileUtilInternal.h>
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

#if defined(_MSC_VER) || defined(__BORLANDC__)
	// MSVC and C++Builder do not provide a definition of STDERR_FILENO.
	const int kStdOutFileno = 1;
	const int kStdErrFileno = 2;
#else
	const int kStdOutFileno = STDOUT_FILENO;
	const int kStdErrFileno = STDERR_FILENO;
#endif  // _MSC_VER

//
// Determine if path is an absolute path
//
bool
UtilInternal::IsAbsolutePath(const string& path)
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
UtilInternal::DirectoryExists(const string& path)
{
    UtilInternal::structstat st;
    if (UtilInternal::stat(path, &st) != 0 || !S_ISDIR(st.st_mode))
    {
        return false;
    }
    return true;
}

//
// Replace "/" by "\"
//
inline string
UtilInternal::FixDirSeparator(const string& path)
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

bool UtilInternal::Exists(const string& name)
{
	return access(name.c_str(), 0/*F_OK*/) == 0;
}

bool UtilInternal::ReadFileToString(const string& name, string* output)
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

void UtilInternal::ReadFileToStringOrDie(const string& name, string* output)
{
	UTIL_CHECK(ReadFileToString(name, output)) << "Could not read: " << name;
}

void UtilInternal::WriteStringToFileOrDie(const string& contents, const string& name)
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
UtilInternal::FileExists(const string& path)
{
    UtilInternal::structstat st;
    if (UtilInternal::stat(path, &st) != 0 || !S_ISREG(st.st_mode))
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
UtilInternal::stat(const string& path, structstat* buffer)
{
    return _wstat(Util::StringToWstring(path).c_str(), buffer);
}

int
UtilInternal::remove(const string& path)
{
    return ::_wremove(Util::StringToWstring(path).c_str());
}

int
UtilInternal::rename(const string& from, const string& to)
{
    return ::_wrename(Util::StringToWstring(from).c_str(), Util::StringToWstring(to).c_str());
}

int
UtilInternal::rmdir(const string& path)
{
    return ::_wrmdir(Util::StringToWstring(path).c_str());
}

int
UtilInternal::mkdir(const string& path, int)
{
    return ::_wmkdir(Util::StringToWstring(path).c_str());
}

FILE*
UtilInternal::fopen(const string& path, const string& mode)
{
    return ::_wfopen(Util::StringToWstring(path).c_str(), Util::StringToWstring(mode).c_str());
}

int
UtilInternal::open(const string& path, int flags)
{
    if (flags & _O_CREAT)
    {
        return ::_wopen(Util::StringToWstring(path).c_str(), flags, _S_IREAD | _S_IWRITE);
    }
    else
    {
        return ::_wopen(Util::StringToWstring(path).c_str(), flags);
    }
}

#ifndef OS_WINRT
int
UtilInternal::getcwd(string& cwd)
{
    wchar_t cwdbuf[_MAX_PATH];
    if (_wgetcwd(cwdbuf, _MAX_PATH) == NULL)
    {
        return -1;
    }
    cwd = Util::WstringToString(cwdbuf);
    return 0;
}
#endif

int
UtilInternal::unlink(const string& path)
{
    return _wunlink(Util::StringToWstring(path).c_str());
}

int
UtilInternal::close(int fd)
{
#ifdef __MINGW32__
        return _close(fd);
#else
        return ::close(fd);
#endif
}

UtilInternal::FileLock::FileLock(const std::string& path) :
    m_fd(INVALID_HANDLE_VALUE),
    m_path(path)
{
#ifndef OS_WINRT
    m_fd = ::CreateFileW(Util::StringToWstring(path).c_str(), GENERIC_WRITE, 0, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#else
    CREATEFILE2_EXTENDED_PARAMETERS params;
    params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    m_fd = ::CreateFile2(Util::StringToWstring(path).c_str(), GENERIC_WRITE, 0,
                        OPEN_ALWAYS, &params);
#endif
    m_path = path;

    if (m_fd == INVALID_HANDLE_VALUE)
    {
        throw Util::FileLockException(__FILE__, __LINE__, GetLastError(), m_path);
    }

#ifdef __MINGW32__
    if (::LockFile(m_fd, 0, 0, 0, 0) == 0)
    {
        throw Util::FileLockException(__FILE__, __LINE__, GetLastError(), m_path);
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
        throw Util::FileLockException(__FILE__, __LINE__, GetLastError(), m_path);
    }
#endif
    //
    // In Windows implementation we don't write the process pid to the file, as it is 
    // not possible to read the file from other process while it is locked here.
    //
}

UtilInternal::FileLock::~FileLock()
{
    assert(m_fd != INVALID_HANDLE_VALUE);
    CloseHandle(m_fd);
    unlink(m_path);
}

UtilInternal::ifstream::ifstream()
{
}

UtilInternal::ifstream::ifstream(const string& path, ios_base::openmode mode) : 
#ifdef  __MINGW32__
    std::ifstream(path.c_str(), mode)
#else
    std::ifstream(Util::StringToWstring(path).c_str(), mode)
#endif
{
}

void
UtilInternal::ifstream::open(const string& path, ios_base::openmode mode)
{
#ifdef  __MINGW32__
    std::ifstream::open(path.c_str(), mode);
#else
    std::ifstream::open(Util::StringToWstring(path).c_str(), mode);
#endif
}


UtilInternal::ofstream::ofstream()
{
}

UtilInternal::ofstream::ofstream(const string& path, ios_base::openmode mode) : 
#ifdef __MINGW32__
    std::ofstream(path.c_str(), mode)
#else
    std::ofstream(Util::StringToWstring(path).c_str(), mode)
#endif
{
}

void
UtilInternal::ofstream::open(const string& path, ios_base::openmode mode)
{
#ifdef __MINGW32__
    std::ofstream::open(path.c_str(), mode);
#else
    std::ofstream::open(Util::StringToWstring(path).c_str(), mode);
#endif
}


#else

//
// Stat
//
int
UtilInternal::stat(const string& path, structstat* buffer)
{
    return ::stat(path.c_str(), buffer);
}

int
UtilInternal::remove(const string& path)
{
    return ::remove(path.c_str());
}

int
UtilInternal::rename(const string& from, const string& to)
{
    return ::rename(from.c_str(), to.c_str());
}

int
UtilInternal::rmdir(const string& path)
{
    return ::rmdir(path.c_str());
}

int
UtilInternal::mkdir(const string& path, int perm)
{
    return ::mkdir(path.c_str(), perm);
}

FILE*
UtilInternal::fopen(const string& path, const string& mode)
{
    return ::fopen(path.c_str(), mode.c_str());
}

int
UtilInternal::open(const string& path, int flags)
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
UtilInternal::getcwd(string& cwd)
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
UtilInternal::unlink(const string& path)
{
    return ::unlink(path.c_str());
}

int
UtilInternal::close(int fd)
{
    return ::close(fd);
}

UtilInternal::FileLock::FileLock(const std::string& path) :
    m_fd(-1),
    m_path(path)
{
    m_fd = ::open(path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (m_fd < 0)
    {
        throw Util::FileLockException(__FILE__, __LINE__, errno, m_path);
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
        Util::FileLockException ex(__FILE__, __LINE__, errno, m_path);
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
        Util::FileLockException ex(__FILE__, __LINE__, errno, m_path);
        close(m_fd);
        throw ex;
    }
}

UtilInternal::FileLock::~FileLock()
{
    assert(m_fd > -1);
    unlink(m_path);
}

UtilInternal::ifstream::ifstream()
{
}

UtilInternal::ifstream::ifstream(const string& path, ios_base::openmode mode) : std::ifstream(path.c_str(), mode)
{
}

void
UtilInternal::ifstream::open(const string& path, ios_base::openmode mode)
{
    std::ifstream::open(path.c_str(), mode);
}

UtilInternal::ofstream::ofstream()
{
}

UtilInternal::ofstream::ofstream(const string& path, ios_base::openmode mode) : std::ofstream(path.c_str(), mode)
{
}

void
UtilInternal::ofstream::open(const string& path, ios_base::openmode mode)
{
    std::ofstream::open(path.c_str(), mode);
}

#endif

long 
UtilInternal::ifstream::length()
{
	if (is_open())
	{
#if 0
		long curpos = tellg();
		// get length of file:
		seekg(0, ios::end);
		long flen = tellg() - curpos;
		seekg(curpos);
#else
		// get pointer to associated buffer object
		filebuf *pbuf = rdbuf();
		streampos curpos = pbuf->pubseekoff(0, cur, ios::in);
		// get file size using buffer's members
		long flen = pbuf->pubseekoff(0, end, ios::in) - curpos;
		pbuf->pubseekpos(curpos, ios::in);
#endif
		return flen;
	}

	return -1;
}

std::string 
UtilInternal::ifstream::load()
{
	long flength = length();
	if (0 != flength)
	{
		string result;
		result.resize(flength);

		// read data as a block:
		read(&result[0], flength);
		
		//close(); // the destructor would call close().

		assert(flength == gcount() || eof());
		result.resize(gcount());

		return result;
	}

	return "";
}

void 
UtilInternal::ifstream::reset()
{
	if (is_open())
	{
		close();
		clear();
	}
}

long UtilInternal::length(const std::string& path)
{
	UtilInternal::structstat st;
	if (0 == UtilInternal::stat(path, &st) && S_ISREG(st.st_mode))
	{
		return st.st_size;
	}
	return -1;
}

long UtilInternal::length(int fd)
{
#if 0
	long flength = ::filelength(fd);
#else
	long curpos = lseek(fd, 0, SEEK_CUR);
	long flength = lseek(fd, 0, SEEK_END) - curpos;
	lseek(fd, curpos, SEEK_SET);
#endif

	return flength;
}

// Get the file size, so we can pre-allocate the string. HUGE speed impact.
long UtilInternal::length(FILE* file)
{
	//FILE* file = fopen(path.c_str(), "rb");
	if (NULL == file)
	{
		return -1;
	}

#if 0
	return length(fileno(file));
#else
	int curpos = fseek(file, 0, SEEK_CUR);
	fseek(file, 0, SEEK_END);
	long flength = ftell(file) - curpos;
	fseek(file, curpos, SEEK_SET);
	//fclose(file);
	return flength;
#endif
}

std::string UtilInternal::fload(const std::string& path)
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

#if 0
	string content;
	content.resize(flength);

	if (1 != fread(&content[0], flength, 1, file))
	{
		fclose(file);
		return "";
	}
#else
	char* const buffer = new char[flength];

	size_t bytes_last_read = 0;  // # of bytes read in the last fread()
	size_t bytes_read = 0;       // # of bytes read so far

	// Keeps reading the file until we cannot read further or the
	// pre-determined file size is reached.
	do
	{
		bytes_last_read = fread(buffer + bytes_read, 1, flength - bytes_read, file);
		bytes_read += bytes_last_read;
	} while (bytes_last_read > 0 && bytes_read < static_cast<size_t>(flength));

	const std::string content(buffer, bytes_read);
	delete[] buffer;
#endif

	fclose(file);

	return Util::String::TranslatingCR2LF(content);
}

std::string UtilInternal::fload(int fd)
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

	return Util::String::TranslatingCR2LF(result);
}

std::string UtilInternal::fload(FILE* file)
{
#if 0

	return fload(fileno(file));

#else

	const size_t file_size = length(file);
	char* const buffer = new char[file_size];

	size_t bytes_last_read = 0;  // # of bytes read in the last fread()
	size_t bytes_read = 0;       // # of bytes read so far

	//fseek(file, 0, SEEK_SET);

	// Keeps reading the file until we cannot read further or the
	// pre-determined file size is reached.
	do 
	{
		bytes_last_read = fread(buffer + bytes_read, 1, file_size - bytes_read, file);
		bytes_read += bytes_last_read;
	} while (bytes_last_read > 0 && bytes_read < file_size);

	const std::string content(buffer, bytes_read);
	delete[] buffer;

	return Util::String::TranslatingCR2LF(content);

#endif
}

void  UtilInternal::redirection(const std::string& filename, 
								const Util::StringConverterPtr& converter, 
								FILE* oldfile/* = stdout | stderr*/)
{
	if ("" != filename)
	{
#ifdef _LARGEFILE64_SOURCE
		FILE* file = freopen64(filename.c_str(), "a", oldfile);
#else
#ifdef _WIN32
		FILE* file = _wfreopen(Util::StringToWstring(
			NativeToUTF8(converter, filename)).c_str(), L"a", oldfile);
#else
		FILE* file = freopen(filename.c_str(), "a", oldfile);
#endif
#endif
		if (0 == file)
		{
			Util::FileException ex(__FILE__, __LINE__, UtilInternal::GetSystemErrno(), filename);
			throw ex;
		}
	}
}

std::vector<std::vector<std::string> > 
UtilInternal::LoadCSVFile(const string& csvfile, const string& separator)
{
	UtilInternal::ifstream in(csvfile);
	if (!in)
	{
		throw Util::FileException(__FILE__, __LINE__, UtilInternal::GetSystemErrno(), csvfile);
	}

	std::vector<std::vector<std::string> > retvec;
	string line;
	bool firstLine = true;
	while (getline(in, line))
	//while (getmultiline(in, line))
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
		Util::String::SplitString(line, separator, colms, true);
		retvec.push_back(colms);
	}

	return retvec;
}
