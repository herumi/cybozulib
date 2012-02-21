#pragma once
/**
	@file
	@brief mmap class

	Copyright (C) 2009-2012 Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include <cybozu/exception.hpp>

#ifdef _WIN32
	#include <windows.h>
#else
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/mman.h>
	#include <unistd.h>
	#include <fcntl.h>
#endif

namespace cybozu {

struct MmapException : public cybozu::Exception {
	MmapException() : cybozu::Exception("mmap") { }
};

class Mmap {
	const char *map_;
	int64_t size_;
#ifdef _WIN32
	HANDLE hFile_;
	HANDLE hMap_;
#endif
public:
	explicit Mmap(const std::string& fileName)
		: map_(0)
		, size_(0)
#ifdef _WIN32
		, hFile_(INVALID_HANDLE_VALUE)
		, hMap_(0)
#endif
	{
		MmapException e;
#ifdef _WIN32
		hFile_ = CreateFile(fileName.c_str(), GENERIC_READ, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile_ == INVALID_HANDLE_VALUE) {
			e << "CreateFile";
			goto ERR_EXIT;
		}
		{
			LARGE_INTEGER size;
			if (GetFileSizeEx(hFile_, &size) == 0) {
				e << "GetFileSizeEx";
				goto ERR_EXIT;
			}
			size_ = size.QuadPart;
		}

		hMap_ = CreateFileMapping(hFile_, NULL, PAGE_READONLY, 0, 0, NULL);
		if (hMap_ == NULL) {
			e << "CreateFileMapping";
			goto ERR_EXIT;
		}

		map_ = (const char*)MapViewOfFile(hMap_, FILE_MAP_READ, 0, 0, 0);
		if (map_ == 0) {
			e << "MapViewOfFile";
			goto ERR_EXIT;
		}
		return;
	ERR_EXIT:
		if (hMap_) CloseHandle(hMap_);
		if (hFile_ != INVALID_HANDLE_VALUE) CloseHandle(hFile_);
		e << fileName;
		throw e;
#else
		int fd = open(fileName.c_str(), O_RDONLY);
		if (fd == -1) {
			MmapException e;
			e << "open" << fileName;
			throw e;
		}

		struct stat st;
		int ret = fstat(fd, &st);
		if (ret != 0) {
			close(fd);
			MmapException e;
			e << "fstat" << fileName;
			throw e;
		}
		size_ = st.st_size;

		map_ = (const char*)mmap(NULL, size_, PROT_READ, MAP_SHARED, fd, 0);
		close(fd);
		if (map_ == MAP_FAILED) {
			MmapException e;
			e << "mmap" << fileName;
			throw e;
		}
#endif
	}
	~Mmap()
	{
#ifdef _WIN32
		UnmapViewOfFile(map_);
		CloseHandle(hMap_);
		CloseHandle(hFile_);
#else
		munmap(const_cast<char*>(map_), size_);
#endif
	}
	int64_t size() const { return size_; }
	const char *get() const { return map_; }
private:
	Mmap(const Mmap &);
	void operator=(const Mmap &);
};

} // cybozu
