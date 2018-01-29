#pragma once
/**
	@file
	@brief stream and line stream class

	Copyright (C) Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include <iosfwd>
#include <cybozu/exception.hpp>

namespace cybozu {

namespace stream_local {

template <typename From, typename To>
struct is_convertible {
	typedef char yes;
	typedef int no;

	static no test(...);
	static yes test(const To*);
	static const bool value = sizeof(test(static_cast<const From*>(0))) == sizeof(yes);
};

template <bool b, class T = void>
struct enable_if { typedef T type; };

template <class T>
struct enable_if<false, T> {};

/* specialization for istream */
template<class InputStream>
size_t readSome_inner(void *buf, size_t size, InputStream& is, typename enable_if<is_convertible<InputStream, std::istream>::value>::type* = 0)
{
	is.read(static_cast<char *>(buf), size);
	const std::streamsize readSize = is.gcount();
	if (readSize < 0) throw cybozu::Exception("stream:readSome_inner:bad readSize") << size << readSize;
	if (readSize > 0x7fffffff && sizeof(std::streamsize) > sizeof(size_t)) throw cybozu::Exception("stream:readSome_inner:too large") << readSize;
	return static_cast<size_t>(readSize);
}

/* generic version for size_t readSome(void *, size_t) */
template<class InputStream>
size_t readSome_inner(void *buf, size_t size, InputStream& is, typename enable_if<!is_convertible<InputStream, std::istream>::value>::type* = 0)
{
	return is.readSome(buf, size);
}

/* specialization for ostream */
template<class OutputStream>
void writeSub(OutputStream& os, const void *buf, size_t size, typename enable_if<is_convertible<OutputStream, std::ostream>::value>::type* = 0)
{
	if (!os.write(static_cast<const char *>(buf), size)) throw cybozu::Exception("stream:writeSub") << size;
}

/* generic version for void write(const void*, size_t), which writes all data */
template<class OutputStream>
void writeSub(OutputStream& os, const void *buf, size_t size, typename enable_if<!is_convertible<OutputStream, std::ostream>::value>::type* = 0)
{
	os.write(buf, size);
}

} // stream_local

/*
	make a specializaiton of class to use new InputStream, OutputStream
*/
template<class InputStream>
struct InputStreamTag {
	static size_t readSome(void *buf, size_t size, InputStream& is)
	{
		return stream_local::readSome_inner<InputStream>(buf, size, is);
	}
	static bool readChar(char *c, InputStream& is)
	{
		return readSome(c, 1, is) == 1;
	}
};

template<class OutputStream>
struct OutputStreamTag {
	static void write(OutputStream& os, const void *buf, size_t size)
	{
		stream_local::writeSub<OutputStream>(os, buf, size);
	}
};

class MemoryInputStream {
	const char *p_;
	size_t size_;
	size_t pos;
public:
	MemoryInputStream(const void *p, size_t size) : p_(static_cast<const char *>(p)), size_(size), pos(0) {}
	size_t readSome(void *buf, size_t size)
	{
		if (size > size_  - pos) size = size_ - pos;
		memcpy(buf, p_ + pos, size);
		pos += size;
		return size;
	}
	size_t getPos() const { return pos; }
};

class MemoryOutputStream {
	char *p_;
	size_t size_;
	size_t pos;
public:
	MemoryOutputStream(void *p, size_t size) : p_(static_cast<char *>(p)), size_(size), pos(0) {}
	void write(const void *buf, size_t size)
	{
		if (size > size_ - pos) throw cybozu::Exception("MemoryOutputStream:write") << size << size_ << pos;
		memcpy(p_ + pos, buf, size);
		pos += size;
	}
	size_t getPos() const { return pos; }
};

class StringInputStream {
	const std::string& str_;
	size_t pos;
	StringInputStream(const StringInputStream&);
	void operator=(const StringInputStream&);
public:
	explicit StringInputStream(const std::string& str) : str_(str), pos(0) {}
	size_t readSome(void *buf, size_t size)
	{
		const size_t remainSize = str_.size() - pos;
		if (size > remainSize) size = remainSize;
		memcpy(buf, &str_[pos], size);
		pos += size;
		return size;
	}
	size_t getPos() const { return pos; }
};

class StringOutputStream {
	std::string& str_;
	StringOutputStream(const StringOutputStream&);
	void operator=(const StringOutputStream&);
public:
	explicit StringOutputStream(std::string& str) : str_(str) {}
	void write(const void *buf, size_t size)
	{
		str_.append(static_cast<const char *>(buf), size);
	}
	size_t getPos() const { return str_.size(); }
};

template<class InputStream>
size_t readSome(void *buf, size_t size, InputStream& is)
{
	return stream_local::readSome_inner(buf, size, is);
}

template<class OutputStream>
void write(OutputStream& os, const void *buf, size_t size)
{
	stream_local::writeSub(os, buf, size);
}

template<typename InputStream>
void read(void *buf, size_t size, InputStream& is)
{
	char *p = static_cast<char*>(buf);
	while (size > 0) {
		size_t readSize = cybozu::readSome(p, size, is);
		if (readSize == 0) throw cybozu::Exception("stream:read");
		p += readSize;
		size -= readSize;
	}
}

template<class InputStream>
bool readChar(char *c, InputStream& is)
{
	return readSome(c, 1, is) == 1;
}

template<class OutputStream>
void writeChar(OutputStream& os, char c)
{
	cybozu::write(os, &c, 1);
}

} // cybozu
