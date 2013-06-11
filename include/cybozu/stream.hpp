#pragma once
/**
	@file
	@brief stream and line stream class

	Copyright (C) Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include <iosfwd>
#include <cybozu/exception.hpp>
#include <cybozu/stream_fwd.hpp>

namespace cybozu {

namespace stream_local {

template <typename From, typename To>
struct is_convertible {
	typedef char yes;
	typedef int no;

	static no test(...);
	static yes test(const To*);
	static const bool value = sizeof(test((const From*)0)) == sizeof(yes);
};

template <bool b, class T = void>
struct enable_if { typedef T type; };

template <class T>
struct enable_if<false, T> {};

template<class InputStream>
size_t readSub(InputStream& is, char *buf, size_t size, typename enable_if<is_convertible<InputStream, std::istream>::value>::type* = 0)
{
	is.read(buf, size);
	return (size_t)is.gcount();
}

template<class InputStream>
size_t readSub(InputStream& is, char *buf, size_t size, typename enable_if<!is_convertible<InputStream, std::istream>::value>::type* = 0)
{
	return is.read(buf, size);
}

} // stream_local

/*
	make a specializaiton of class to use new InputStream, OutputStream
*/
template<class InputStream>
struct InputStreamTag {
	static inline size_t read(InputStream& is, char *buf, size_t size)
	{
		return stream_local::readSub<InputStream>(is, buf, size);
	}
};

template<class OutputStream>
struct OutputStreamTag {
	static inline void write(OutputStream& os, const char *buf, size_t size)
	{
		if (!os.write(buf, size)) throw cybozu::Exception("OutputStream:write") << size;
	}
};

class MemoryInputStream {
	const char *p_;
	size_t size_;
public:
	MemoryInputStream(const char *p, size_t size) : p_(p), size_(size) {}
	size_t read(char *buf, size_t size, const char * = "")
	{
		if (size > size_) size = size_;
		memcpy(buf, p_, size);
		p_ += size;
		size_ -= size;
		return size;
	}
};

class MemoryOutputStream {
	char *p_;
	size_t size_;
public:
	MemoryOutputStream(char *p, size_t size) : p_(p), size_(size) {}
	void write(const char *buf, size_t size)
	{
		if (size > size_) throw cybozu::Exception("MemoryOutputStream:write") << size << size_;
		memcpy(p_, buf, size);
		p_ += size;
		size_ -= size;
	}
};

class StringInputStream {
	const std::string& str_;
	size_t cur_;
	StringInputStream(const StringInputStream&);
	void operator=(const StringInputStream&);
public:
	explicit StringInputStream(const std::string& str) : str_(str), cur_(0) {}
	size_t read(char *buf, size_t size)
	{
		const size_t remainSize = str_.size() - cur_;
		if (size > remainSize) size = remainSize;
		memcpy(buf, &str_[cur_], size);
		cur_ += size;
		return size;
	}
};

class StringOutputStream {
	std::string& str_;
	StringOutputStream(const StringOutputStream&);
	void operator=(const StringOutputStream&);
public:
	explicit StringOutputStream(std::string& str) : str_(str) {}
	void write(const char *buf, size_t size)
	{
		str_.append(buf, size);
	}
};

} // cybozu
