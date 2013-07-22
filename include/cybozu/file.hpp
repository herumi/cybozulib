#pragma once
/**
	@file
	@brief file class and operations

	Copyright (C) 2008 Cybozu Labs, Inc., all rights reserved.
*/

#include <assert.h>
#include <sys/stat.h> // for stat
#include <cybozu/exception.hpp>
#include <cybozu/stream_fwd.hpp>
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
			if (dontThrow) return false;
			throw cybozu::Exception("file:open:bad mode") << name_ << mode;
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
			throw cybozu::Exception("file:open") << name_ << cybozu::ErrorNo() << static_cast<int>(mode);
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
			throw cybozu::Exception("fie:close") << name_ << cybozu::ErrorNo();
		}
		return isOK;
	}
	bool sync(bool dontThrow = true)
	{
		if (!isOpen()) return true;
#ifdef _WIN32
		bool isOK = FlushFileBuffers(hdl_) != 0;
#elif defined(__linux__) || defined(__CYGWIN__)
		bool isOK = fdatasync(hdl_) == 0;
#else
		bool isOK = fcntl(hdl_, F_FULLFSYNC) == 0;
#endif
		if (!dontThrow && !isOK) {
			throw cybozu::Exception("file:sync") << name_ << cybozu::ErrorNo();
		}
		return isOK;
	}
	ssize_t write(const void *buf, size_t bufSize, bool dontThrow = true)
	{
#ifdef _WIN32
		assert(bufSize < (1ULL << 31)); // modify for 64bit
		DWORD writeSize;
		bool isOK = WriteFile(hdl_, buf, static_cast<DWORD>(bufSize), &writeSize, NULL) != 0;
#else
		bool isOK = ::write(hdl_, buf, bufSize) == static_cast<ssize_t>(bufSize);
#endif
		if (!dontThrow && !isOK) {
			throw cybozu::Exception("file:write") << name_ << cybozu::ErrorNo();
		}
		return isOK ? bufSize : -1;
	}
	ssize_t read(void *buf, size_t bufSize, bool dontThrow = true)
	{
#ifdef _WIN32
		assert(bufSize < (1ULL << 31)); // modify for 64bit
		DWORD readSize;
		bool isOK = ReadFile(hdl_, buf, static_cast<DWORD>(bufSize), &readSize, NULL) != 0;
#else
		ssize_t readSize = ::read(hdl_, buf, bufSize);
		bool isOK = readSize >= 0;
#endif
		if (!dontThrow && !isOK) {
			throw cybozu::Exception("file:read") << name_ << cybozu::ErrorNo();
		}
		return isOK ? static_cast<ssize_t>(readSize) : -1;
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
			throw cybozu::Exception("file:seek") << name_ << cybozu::ErrorNo() << pos << static_cast<int>(dir);
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
			throw cybozu::Exception("file:getSize") << name_ << cybozu::ErrorNo();
		}
		return isOK ? fileSize : -1;
	}
};

/*
	name has suffix
*/
inline bool HasSuffix(const std::string& name, const std::string& suffix)
{
	const size_t nameSize = name.size();
	const size_t suffixSize = suffix.size();
	if (suffixSize == 0) return true;
	if (nameSize < suffixSize + 1) return false;
	const char *p = &name[nameSize - suffixSize - 1];
	return *p == '.' && memcmp(p + 1, &suffix[0], suffixSize) == 0;
}
/*
	split name as basename.suffix
*/
inline std::string GetBaseName(const std::string& name, std::string *suffix = 0)
{
	size_t pos = name.find_last_of('.');
	if (pos == std::string::npos) {
		if (suffix) suffix->clear();
		return name;
	}
	if (suffix) {
		*suffix = name.substr(pos + 1);
	}
	return name.substr(0, pos);
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
	@note file name is the form "xxx.exe" then baseName = xxx
*/
inline std::string GetExePath(std::string *baseName = 0)
{
	std::string path;
	path.resize(4096);
#ifdef _WIN32
	if (GetModuleFileNameA(NULL, &path[0], static_cast<int>(path.size()) - 2)) {
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
				const std::string name = path.substr(pos + 1);
				std::string suffix;
				std::string base = GetBaseName(name, &suffix);
				if (suffix == "exe") {
					*baseName = base;
				} else {
					*baseName = name;
				}
			}
			path.resize(pos + 1);
		}
	}
#endif
	return path;
}

/**
	get file size
*/
inline uint64_t GetFileSize(const std::string& name)
{
#ifdef _WIN32
	struct __stat64 buf;
	bool isOK = _stat64(name.c_str(), &buf) == 0;
#else
	struct stat buf;
	bool isOK = stat(name.c_str(), &buf) == 0;
#endif
	if (!isOK) {
		throw cybozu::Exception("GetFileSize") << name << cybozu::ErrorNo();
	}
	return buf.st_size;
}

/**
	verify whether path exists or not
*/
inline bool DoesFileExist(const std::string& path)
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

inline void RenameFile(const std::string& from, const std::string& to)
{
	if (DoesFileExist(to)) {
		throw cybozu::Exception("RenameFile:file already exist") << from << to;
	}
#ifdef _WIN32
	bool isOK = ::MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH) != 0;
#else
	bool isOK = ::rename(from.c_str(), to.c_str()) == 0;
#endif
	if (!isOK) {
		throw cybozu::Exception("RenameFile") << from << to << cybozu::ErrorNo();
	}
}

/**
	remove file
*/
inline void RemoveFile(const std::string& name)
{
#ifdef _WIN32
	bool isOK = DeleteFileA(name.c_str()) != 0;
#else
	bool isOK = unlink(name.c_str()) == 0;
#endif
	if (!isOK) {
		throw cybozu::Exception("RemoveFile") << name << cybozu::ErrorNo();
	}
}

struct FileInfo {
	std::string name;
	bool isFile;
	FileInfo() : isFile(false) {}
};

typedef std::vector<FileInfo> FileList;

/**
	get file name in dir
	@param list [out] list must be able to push_back(file::Info)
	@param dir [in] directory
	@param suffix [in] select files having suffix and all directory
	@param cond [in] filter function (select if cond(targetFile, suffix) is true)
*/
template<class List>
inline bool GetFileList(List &list, const std::string& dir, const std::string& suffix = "", bool (*cond)(const std::string&, const std::string&) = cybozu::HasSuffix)
{
#ifdef _WIN32
	std::string path = dir + "/*";
	WIN32_FIND_DATAA fd;
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
	Handle hdl(FindFirstFileA(path.c_str(), &fd));
	if (hdl.hdl_ == INVALID_HANDLE_VALUE) {
		return false;
	}
	do {
		FileInfo fi;
		fi.name = fd.cFileName;
		fi.isFile = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
		if (!fi.isFile || cond(fi.name, suffix)) {
			list.push_back(fi);
		}
	} while (FindNextFileA(hdl.hdl_, &fd) != 0);
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
		if (!fi.isFile || cond(fi.name, suffix)) {
			list.push_back(fi);
		}
	}
#endif
}

template<>
struct InputStreamTag<cybozu::File> {
	static inline size_t read(cybozu::File& is, char *buf, size_t size, const char * = "")
	{
		return (size_t)is.read(buf, size);
	}
};

template<>
struct OutputStreamTag<cybozu::File> {
	static inline void write(cybozu::File& os, const char *buf, size_t size, const char *msg = "")
	{
		if ((size_t)os.write(buf, size) != size) throw cybozu::Exception("OutputStreamTag<cybozu::File>:write") << size << msg;
	}
};

} // cybozu
