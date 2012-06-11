#pragma once
/**
	@file
	@brief stream and line stream class

	Copyright (C) 2008 Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include <memory.h>
#include <cybozu/file.hpp>

namespace cybozu {

struct StreamException : public cybozu::Exception {
	StreamException() : cybozu::Exception("stream") { }
};

namespace stream {

#define CYBOZU_STREAM_CRLF "\x0d\x0a"

const char CR = '\x0d';
const char LF = '\x0a';
const char CRLF[] = CYBOZU_STREAM_CRLF;

} // stream

namespace stream_local {

/*
	offer method like C
	max of size_t is larger than max of ssize_t, but
	the value is (2^31-1) for 32bit, (2^63-1). so we can't use the large data.
*/
template<class T>
struct Readable {
	static inline ssize_t readC(void *param, char *buf, size_t size)
	{
		return ((T*)param)->read(buf, size);
	}
};

template<class T>
struct Writable {
	static inline ssize_t writeC(void *param, const char *buf, size_t size)
	{
		return ((T*)param)->write(buf, size);
	}
};

} // stream_local

/**
	inputstream interface
*/
struct InputStreamIF : public stream_local::Readable<InputStreamIF> {
	InputStreamIF(void *param = 0, ssize_t read(void *, char *, size_t) = 0)
		: param_(param)
		, read_(read)
	{
	}
	void *param_;
	ssize_t (*read_)(void *param, char *buf, size_t size);
	ssize_t read(char *buf, size_t size)
	{
		return read_(param_, buf, size);
	}
};
/**
	outputstream interface
*/
struct OutputStreamIF : public stream_local::Writable<OutputStreamIF> {
	OutputStreamIF(void *param = 0, ssize_t write(void *, const char*, size_t) = 0)
		: param_(param)
		, write_(write)
	{
	}
	void *param_;
	ssize_t (*write_)(void *param, const char *buf, size_t size);
	ssize_t write(const char *buf, size_t size)
	{
		return write_(param_, buf, size);
	}
};

/**
	some utility class for InputStream/OutputStream
*/

struct MemoryInputStream : public stream_local::Readable<MemoryInputStream> {
	const char *buf_;
	const size_t size_;
	size_t pos_;
	MemoryInputStream(const char *buf, size_t size)
		: buf_(buf)
		, size_(size)
		, pos_(0)
	{
	}
	MemoryInputStream(const std::string& str)
		: buf_(&str[0])
		, size_(str.size())
		, pos_(0)
	{
	}
	ssize_t read(char *buf, size_t size)
	{
		ssize_t readSize = static_cast<ssize_t>(std::min(size, size_ - pos_));
		memcpy(buf, &buf_[pos_], readSize);
		pos_ += readSize;
		return readSize;
	}
private:
	MemoryInputStream(const MemoryInputStream&);
	void operator=(const MemoryInputStream&);
};

struct MemoryOutputStream : public stream_local::Writable<MemoryOutputStream> {
	char *buf_;
	const size_t size_;
	size_t pos_;
	MemoryOutputStream(char *buf, size_t size)
		: buf_(buf)
		, size_(size)
		, pos_(0)
	{
	}
	ssize_t write(const char *buf, size_t size)
	{
		ssize_t writeSize = static_cast<ssize_t>(std::min(size, size_ - pos_));
		memcpy(&buf_[pos_], buf, writeSize);
		pos_ += writeSize;
		return writeSize;
	}
private:
	MemoryOutputStream(const MemoryOutputStream&);
	void operator=(const MemoryOutputStream&);
};

// for test(not efficient)
struct StringOutputStream : public stream_local::Writable<StringOutputStream> {
	std::string str_;
	ssize_t write(const char *buf, size_t size)
	{
		str_.append(buf, size);
		return static_cast<ssize_t>(size);
	}
};

struct RefStringOutputStream : public stream_local::Writable<RefStringOutputStream> {
	std::string& str_;
	RefStringOutputStream(std::string& str)
		: str_(str)
	{
	}
	ssize_t write(const char *buf, size_t size)
	{
		str_.append(buf, size);
		return static_cast<ssize_t>(size);
	}
private:
	RefStringOutputStream(const RefStringOutputStream&);
	void operator=(const RefStringOutputStream&);
};

struct FileInputStream : public stream_local::Readable<FileInputStream> {
	cybozu::File ifs_;
	FileInputStream(const std::string& name)
	{
		ifs_.openR(name, !cybozu::DontThrow);
	}
	ssize_t read(char *buf, size_t size)
	{
		return ifs_.read(buf, size);
	}
private:
	FileInputStream(const FileInputStream&);
	void operator=(const FileInputStream&);
};

struct FileOutputStream : public stream_local::Writable<FileOutputStream> {
	cybozu::File ofs_;
	FileOutputStream(const std::string& name)
	{
		ofs_.openW(name, !cybozu::DontThrow);
	}
	ssize_t write(const char *buf, size_t size)
	{
		return ofs_.write(buf, size);
	}
private:
	FileOutputStream(const FileOutputStream&);
	void operator=(const FileOutputStream&);
};

struct BufferedInputStream : public stream_local::Readable<BufferedInputStream> {
};

/**
	construct lines from reader
	accept 0x0d 0xa and 0x0a and remove them
	next() returns one line without CRLF
	throw exception if line.size() > maxLineSize (line.size() does not include CRLF)

	Remark:next() may return the last data without CRLF

	Reader must have size_t read(char *buf, size_t size); method

	How to use this

	LinstStreamT<Reader> lineStream(reader);
	for (;;) {
		const std::string *line = lineStream.next();
		if (line == 0) {
		  // no data
		  break;
		}
	}
*/
template<class Reader, size_t maxLineSize = 1024 * 2>
class LineStreamT {
	Reader& reader_;
	char buf_[maxLineSize * 2];
	size_t bufSize_;
	const char *next_;
	bool eof_;
	std::string line_;
	LineStreamT(const LineStreamT&);
	void operator=(const LineStreamT&);
public:
	LineStreamT(Reader& reader)
		: reader_(reader)
		, bufSize_(0)
		, next_(buf_)
		, eof_(false)
	{
	}
	/**
		get line without CRLF
		@param begin [out] begin of line
		@param end [out] end of line
		@retval true if sucess
		@retval false if not data
	*/
	bool next(const char **begin, const char **end)
	{
		if (eof_) return false;
		for (;;) {
			const size_t remainSize = &buf_[bufSize_] - next_;
			if (remainSize > 0) {
				const char *endl = (const char *)memchr(next_, cybozu::stream::LF, remainSize);
				if (endl) {
					if (endl > next_ && endl[-1] == cybozu::stream::CR) {
						*end = endl - 1;
					} else {
						*end = endl;
					}
					*begin = next_;
					next_ = endl + 1;
					if ((size_t)(*end - *begin) > maxLineSize) {
						cybozu::StreamException e;
						e << "next" << "line is too long" << *begin;
						throw e;
					}
					return true;
				}
				// move next_ to top of buf_
				for (size_t i = 0; i < remainSize; i++) {
					buf_[i] = next_[i];
				}
				next_ = buf_;
				bufSize_ = remainSize;
			} else {
				bufSize_ = 0;
				next_ = buf_;
			}
			ssize_t readSize = reader_.read(&buf_[bufSize_], sizeof(buf_) - bufSize_);
			if (readSize <= 0) {
				eof_ = true;
				if (bufSize_ == 0) return false;
				if (bufSize_ > maxLineSize) {
					cybozu::StreamException e;
					e << "next" << "line is too long" << "no CRLF";
					throw e;
				}
				// take all remain buffer
				*begin= buf_;
				*end = &buf_[bufSize_];
				return true;
			}
			bufSize_ += readSize;
		}
	}
	/**
		get line
		@remark return value will be destroyed by calling next()
		you may modify returned string if not NULL
	*/
	std::string *next()
	{
		const char *begin;
		const char *end;
		if (next(&begin, &end)) {
			line_.assign(begin, end);
			return &line_;
		}
		return 0;
	}
};

} // cybozu
