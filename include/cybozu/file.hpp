#pragma once
/**
	@file
	@brief file class and operations

	Copyright (C) 2008-2012 Cybozu Labs, Inc., all rights reserved.
*/

#include <assert.h>
#include <sys/stat.h> // for stat
#include <cybozu/exception.hpp>
#include <vector>
#include <ios>
#ifdef _WIN32
	#include <shlwapi.h>
	#include <io.h>
	#include <fcntl.h>
	#include <shlobj.h>
	#include <direct.h>
	#include <windows.h>
	#pragma comment(lib, "shlwapi.lib")
	#pragma comment(lib, "shell32.lib")
	#pragma comment(lib, "User32.lib")
#else
	#include <stdio.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <fcntl.h>
	#include <dirent.h>
#endif

namespace cybozu {

struct FileException : public cybozu::Exception {
	FileException() : cybozu::Exception("file") { }
};

class File {
	std::string name_;
#ifdef _WIN32
	HANDLE hdl_;
#else
	int hdl_;
	enum {
		INVALID_HANDLE_VALUE = -1
	};
#endif
	File(const File&);
	void operator=(const File&);
public:
	File()
		: hdl_(INVALID_HANDLE_VALUE)
	{
	}
	File(const std::string& name, std::ios::openmode mode, bool dontThrow = true)
		: hdl_(INVALID_HANDLE_VALUE)
	{
		open(name, mode, dontThrow);
	}
	~File() throw()
	{
		sync(cybozu::DontThrow);
		close(cybozu::DontThrow);
	}
	bool isOpen() const throw() { return hdl_ != INVALID_HANDLE_VALUE; }
	/**
		support mode
		always binary mode
		ios::in : read only
		ios::out : write only(+truncate)
		ios::out + ios::app(append)
	*/
	bool open(const std::string& name, std::ios::openmode mode, bool dontThrow = true)
	{
		name_ = name;
		bool isCorrectMode = true;
		// verify mode
		if (!!(mode & std::ios::in) == !!(mode & std::ios::out)) isCorrectMode = false;
		if (mode & std::ios::in) {
			if ((mode & std::ios::app) || (mode & std::ios::trunc)) isCorrectMode = false;
		} else {
			if ((mode & std::ios::app) && (mode & std::ios::trunc)) isCorrectMode = false;
		}
		if (!isCorrectMode) {
			if (!dontThrow) {
				cybozu::FileException e;
				e << "bad mode" << name_ << mode;
				throw e;
			}
			return false;
		}
#ifdef _WIN32
		DWORD access = GENERIC_READ;
		DWORD disposition = OPEN_EXISTING;
		DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE;
		if (mode & std::ios::out) {
			access = GENERIC_WRITE;
			disposition = CREATE_ALWAYS;
			if (mode & std::ios::app) {
				disposition = OPEN_ALWAYS;
			}
		}
#else
		int flags = O_RDONLY; // | O_NOATIME; /* can't use on NFS */
		mode_t access = 0644;
		if (mode & std::ios::out) {
			flags = O_WRONLY | O_CREAT;
			if (mode & std::ios::app) {
				flags |= O_APPEND;
			} else {
				flags |= O_TRUNC;
			}
		}
#endif
#ifdef _WIN32
		hdl_ = CreateFileA(name.c_str(), access, share, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
#else
		hdl_ = ::open(name.c_str(), flags, access);
#endif
		if (isOpen()) {
			if (mode & std::ios::app) {
				seek(getSize(), std::ios::beg);
			}
			return true;
		} else {
			if (dontThrow) return false;
			cybozu::FileException e;
			e << "open" << name_ << cybozu::ErrorNo().toString() << (int)mode;
			throw e;
		}
	}
	bool openW(const std::string& name, bool dontThrow = true)
	{
		return open(name, std::ios::out | std::ios::binary | std::ios::trunc, dontThrow);
	}
	bool openR(const std::string& name, bool dontThrow = true)
	{
		return open(name, std::ios::in | std::ios::binary, dontThrow);
	}
	bool close(bool dontThrow = true)
	{
		if (!isOpen()) return true;
#ifdef _WIN32
		bool isOK = CloseHandle(hdl_) != 0;
#else
		bool isOK = ::close(hdl_) == 0;
#endif
		hdl_ = INVALID_HANDLE_VALUE;
		if (!dontThrow && !isOK) {
			cybozu::FileException e;
			e << "close" << name_ << cybozu::ErrorNo().toString();
			throw e;
		}
		return isOK;
	}
	bool sync(bool dontThrow = true)
	{
		if (!isOpen()) return true;
#ifdef _WIN32
		bool isOK = FlushFileBuffers(hdl_) != 0;
#elif defined(__linux__)
		bool isOK = fdatasync(hdl_) == 0;
#else
		bool isOK = fcntl(hdl_, F_FULLFSYNC) == 0;
#endif
		if (!dontThrow && !isOK) {
			cybozu::FileException e;
			e << "sync" << name_ << cybozu::ErrorNo().toString();
			throw e;
		}
		return isOK;
	}
	ssize_t write(const void *buf, size_t bufSize, bool dontThrow = true)
	{
#ifdef _WIN32
		assert(bufSize < (1ULL << 32)); // modify for 64bit
		DWORD writeSize;
		bool isOK = WriteFile(hdl_, buf, (DWORD)bufSize, &writeSize, NULL) != 0;
#else
		bool isOK = ::write(hdl_, buf, bufSize) == static_cast<ssize_t>(bufSize);
#endif
		if (!dontThrow && !isOK) {
			cybozu::FileException e;
			e << "write" << name_ << cybozu::ErrorNo().toString();
			throw e;
		}
		return isOK ? bufSize : -1;
	}
	ssize_t read(void *buf, size_t bufSize, bool dontThrow = true)
	{
#ifdef _WIN32
		assert(bufSize < (1ULL << 32)); // modify for 64bit
		DWORD readSize;
		bool isOK = ReadFile(hdl_, buf, (DWORD)bufSize, &readSize, NULL) != 0;
#else
		int readSize = ::read(hdl_, buf, bufSize);
		bool isOK = readSize >= 0;
#endif
		if (!dontThrow && !isOK) {
			cybozu::FileException e;
			e << "read" << name_ << cybozu::ErrorNo().toString();
			throw e;
		}
		return isOK ? (int)readSize : -1;
	}
	bool seek(int64_t pos, std::ios::seek_dir dir, bool dontThrow = true)
	{
#ifdef _WIN32
		LARGE_INTEGER largePos;
		largePos.QuadPart = pos;
		DWORD posMode = FILE_BEGIN;
		switch (dir) {
		case std::ios::beg:
			posMode = FILE_BEGIN;
			break;
		case std::ios::cur:
			posMode = FILE_CURRENT;
			break;
		case std::ios::end:
			posMode = FILE_END;
			break;
		default:
			__assume(0);
		}
		bool isOK = SetFilePointerEx(hdl_, largePos, NULL, posMode) != 0;
#else
		int whence;
		switch (dir) {
		case std::ios::beg:
			whence = SEEK_SET;
			break;
		case std::ios::cur:
			whence = SEEK_CUR;
			break;
		case std::ios::end:
		default:
			whence = SEEK_END;
			break;
		}
		bool isOK = lseek(hdl_, pos, whence) >= 0;
#endif
		if (!dontThrow && !isOK) {
			cybozu::FileException e;
			e << "seek" << name_ << cybozu::ErrorNo().toString() << pos << (int)dir;
			throw e;
		}
		return isOK;
	}
	int64_t getSize(bool dontThrow = true) const
	{
		int64_t fileSize;
#ifdef _WIN32
		LARGE_INTEGER size;
		bool isOK = GetFileSizeEx(hdl_, &size) != 0;
		fileSize = size.QuadPart;
#else
		struct stat stat;
		bool isOK = fstat(hdl_, &stat) == 0;
		fileSize = stat.st_size;
#endif
		if (!dontThrow && !isOK) {
			cybozu::FileException e;
			e << "getSize" << name_ << cybozu::ErrorNo().toString();
			throw e;
		}
		return isOK ? fileSize : -1;
	}
};

namespace file {

/**
	if str has tail, then remove '.' + tail
	otherwise return str
*/
inline std::string RemoveTail(const std::string& str, const std::string& tail, char c = '.')
{
	size_t pos = str.find_last_of(c);
	if (str.substr(pos + 1) == tail) {
		return str.substr(0, pos);
	}
	return str;
}
/**
	replace \ with /
*/
inline void ReplaceBackSlash(std::string& str)
{
	for (size_t i = 0, n = str.size(); i < n; i++) {
		if (str[i] == '\\') str[i] = '/';
	}
}

/**
	get exe path and baseNamme
	@note file name should be "xxx.exe"
*/
inline std::string GetExePath(std::string *baseName = 0)
{
	std::string path;
	path.resize(4096);
#ifdef _WIN32
	if (GetModuleFileNameA(NULL, &path[0], (int)path.size() - 2)) {
		PathRemoveExtensionA(&path[0]);
		if (baseName) {
			*baseName = PathFindFileNameA(&path[0]);
		}
		if (::PathRemoveFileSpecA(&path[0])) {
			::PathAddBackslashA(&path[0]);
			path[0] = static_cast<char>(tolower(path[0]));
			path.resize(strlen(&path[0]));
			ReplaceBackSlash(path);
		}
	}
#else
	int ret = readlink("/proc/self/exe", &path[0], path.size() - 2);

	if (ret != -1) {
		path.resize(ret);
		size_t pos = path.find_last_of('/');
		if (pos != std::string::npos) {
			if (baseName) {
				*baseName = RemoveTail(path.substr(pos + 1), "exe");
			}
			path.resize(pos + 1);
			ReplaceBackSlash(path);
		}
	}
#endif
	return path;
}

/**
	get file size
*/
inline int64_t GetSize(const std::string& name, bool dontThrow = true)
{
#ifdef _WIN32
	struct __stat64 buf;
	bool isOK = _stat64(name.c_str(), &buf) == 0;
#else
	struct stat buf;
	bool isOK = stat(name.c_str(), &buf) == 0;
#endif
	if (!dontThrow && !isOK) {
		cybozu::FileException e;
		e << "GetSize" << name << cybozu::ErrorNo().toString();
		throw e;
	}
	return isOK ? buf.st_size : -1;
}

/**
	verify whether path exists or not
*/
inline bool DoesExist(const std::string& path)
{
	if (path.empty()) return false;
	std::string p = path;
	char c = p[p.size() - 1];
	if (c == '/' || c == '\\') {
		p.resize(p.size() - 1);
	}
#ifdef _WIN32
	struct _stat buf;
	return _stat(p.c_str(), &buf) == 0;
#else
	struct stat buf;
	return stat(p.c_str(), &buf) == 0;
#endif
}

inline bool Move(const std::string& from, const std::string& to, bool dontThrow = true)
{
	if (DoesExist(to)) {
		if (dontThrow) {
			return false;
		} else {
			cybozu::FileException e;
			e << "file already exist" << to;
			throw e;
		}
	}
#ifdef _WIN32
	bool isOK = ::MoveFileEx(from.c_str(), to.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH) != 0;
#else
	bool isOK = rename(from.c_str(), to.c_str()) == 0;
#endif
	if (!dontThrow && !isOK) {
		cybozu::FileException e;
		e << "Move" << from << to << cybozu::ErrorNo().toString();
		throw e;
	}
	return isOK;
}

/**
	remove file
*/
inline bool Remove(const std::string& name, bool dontThrow = true)
{
#ifdef _WIN32
	bool isOK = DeleteFile(name.c_str()) != 0;
#else
	bool isOK = unlink(name.c_str()) == 0;
#endif
	if (!dontThrow && !isOK) {
		cybozu::FileException e;
		e << "Remove" << name << cybozu::ErrorNo().toString();
		throw e;
	}
	return isOK;
}

struct FileInfo {
	std::string name;
	bool isFile;
};

typedef std::vector<FileInfo> FileInfoVec;
/**
	get file name in dir
	@param list [out] list must be able to push_back(FileInfo)
	@param dir [in] directory
*/
template<class T>
bool GetFilesInDir(T &list, const std::string& dir)
{
#ifdef _WIN32
	std::string path = dir + "/*";
	WIN32_FIND_DATA fd;
	struct Handle {
		Handle(HANDLE hdl)
			: hdl_(hdl)
		{
		}
		~Handle()
		{
			if (hdl_ != INVALID_HANDLE_VALUE) {
				FindClose(hdl_);
			}
		}
		HANDLE hdl_;
	};
	Handle hdl(FindFirstFile(path.c_str(), &fd));
	if (hdl.hdl_ == INVALID_HANDLE_VALUE) {
		return false;
	}
	do {
		FileInfo fi;
		fi.name = fd.cFileName;
		fi.isFile = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
//		LARGE_INTEGER fileSize;
//		fileSize.LowPart = fd.nFileSizeLow;
//		fileSize.HighPart = fd.nFileSizeHigh;
//		fi.size = fileSize.QuadPart;
		list.push_back(fi);
	} while (FindNextFile(hdl.hdl_, &fd) != 0);
	return true;
#else
	struct Handle {
		DIR *dir_;
		Handle(DIR *dir)
			: dir_(dir)
		{
			if (dir_ == 0) {
				perror("opendir");
			}
		}
		~Handle()
		{
			if (dir_) {
				if (::closedir(dir_)) {
					perror("closedir");
				}
			}
		}
		bool isValid() const { return dir_ != 0; }
	};
	Handle hdl(::opendir(dir.c_str()));
	if (!hdl.isValid()) return false;
	for (;;) {
		struct dirent *dp = ::readdir(hdl.dir_);
		if (dp == 0) return true;
		FileInfo fi;
		fi.name = dp->d_name;
		fi.isFile = dp->d_type == DT_REG;
		list.push_back(fi);
	}
#endif
}

} } // cybozu::file
