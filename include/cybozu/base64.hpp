#pragma once
/**
	@file
	@brief base64 encoder and decoder

	Copyright (C) 2008-2012 Cybozu Labs, Inc., all rights reserved.
*/

#include <iostream>
#include <assert.h>
#include <cybozu/stream.hpp>

namespace cybozu {

struct Base64Exception : public cybozu::Exception {
	Base64Exception() : cybozu::Exception("base64") { }
};

namespace base64_local {

template<class OutputStream>
void Write(OutputStream& os, const char *buf, size_t size)
{
	size_t writeSize = static_cast<size_t>(os.write(buf, size));
	if (writeSize != size) {
		cybozu::Base64Exception e;
		e << "Write" << cybozu::exception::makeString(buf, size);
		throw e;
	}
}

} // base64_local

/**
	base64 encode
	@param os [in] output stream
	@param is [in] intpu stream
	@param maxLineSize [in] max line size(multiply of 4 or zero(means infinite line))
	@param isCRLF [in] put CRLF if ture, put LF if false
*/
template<class OutputStream, class InputStream>
void EncodeToBase64(OutputStream& os, InputStream& is, size_t maxLineSize = 76, bool isCRLF = true)
{
	const size_t innerMaxLineSize = 128;
	if (maxLineSize > innerMaxLineSize || ((maxLineSize % 4) != 0)) {
		cybozu::Base64Exception e;
		e << "EncodeToBase64" << "bad line size" << (int)maxLineSize;
		throw e;
	}

	const char *tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	unsigned int remain = 0;
	size_t idx = 0; /* 0, 1, 2 */
	size_t pos = 0; /* line position */

	char inBuf[1024];
	char outBuf[4096];
	assert(sizeof(outBuf) > (sizeof(inBuf) * 4) / 3 +(sizeof(inBuf) * 2) /* max # of CRLF */ + 16 /* margin */);
	size_t outBufSize = 0;
	for (;;) {
		ssize_t readSize = is.read(inBuf, sizeof(inBuf));
		if (readSize <= 0) break;
		for (size_t i = 0; i < static_cast<size_t>(readSize); i++) {
			unsigned int c = (unsigned char)inBuf[i];
			if (idx == 0) {
				remain = (c & 3) << 4;
				outBuf[outBufSize++] = tbl[c >> 2];
				pos++;
				idx = 1;
			} else if (idx == 1) {
				unsigned int tmp = remain | (c >> 4);
				remain = (c & 0xf) << 2;
				outBuf[outBufSize++] = tbl[tmp];
				pos++;
				idx = 2;
			} else {
				outBuf[outBufSize++] = tbl[remain | (c >> 6)];
				outBuf[outBufSize++] = tbl[c & 0x3f];
				pos += 2;
				idx = 0;
			}
			if (maxLineSize && pos == maxLineSize) {
				if (isCRLF) outBuf[outBufSize++] = (char)0x0d;
				outBuf[outBufSize++] = (char)0x0a;
				pos = 0;
			}
			assert(outBufSize <= sizeof(outBuf));
		}
		base64_local::Write(os, outBuf, outBufSize);
		outBufSize = 0;
	}
	if (idx > 0) {
		outBuf[outBufSize++] = tbl[remain];
		outBuf[outBufSize++] = '=';
		pos += 2;
		if (idx == 1) {
			outBuf[outBufSize++] = '=';
			pos++;
		}
	}
	if (maxLineSize && pos > 0) {
		if (isCRLF) outBuf[outBufSize++] = (char)0x0d;
		outBuf[outBufSize++] = (char)0x0a;
	}
	if (outBufSize > 0) {
		base64_local::Write(os, outBuf, outBufSize);
	}
}

/*
	base64 decoder class
	@param os [in] output stream
*/
template<class OutputStream>
class Base64Decoder {
	OutputStream& os_;
	char outBuf_[1024];
	size_t outBufSize_;
	size_t idx_;
	unsigned int cur_;
	Base64Decoder(const Base64Decoder&);
	void operator=(const Base64Decoder&);
public:
	Base64Decoder(OutputStream& os)
		: os_(os)
		, outBufSize_(0)
		, idx_(0)
		, cur_(0)
	{
	}
	/*
		@param buf [in] input buffer
		@param size [in] input buffer size
		@return same size(always)
	*/
	ssize_t write(const char *buf, size_t size)
	{
		const unsigned int S = 255; /* skip character */
		static const unsigned int tbl[256] = {
			S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
			S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
			S, S, S, S, S, S, S, S, S, S, S, 62, S, S, S, 63,
			52, 53, 54, 55, 56, 57, 58, 59, 60, 61, S, S, S, S, S, S,
			S, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
			15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, S, S, S, S, S,
			S, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
			41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, S, S, S, S, S,
			S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
			S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
			S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
			S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
			S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
			S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
			S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
			S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, S,
		};
		for (size_t i = 0; i < size; i++) {
			unsigned int t = tbl[(unsigned char)buf[i]];
			if (t == S) continue;
			if (idx_ == 0) {
				cur_ = t << 2;
				idx_ = 1;
			} else if (idx_ == 1) {
				outBuf_[outBufSize_++] = static_cast<char>(cur_ | (t >> 4));
				cur_ = (t & 0xf) << 4;
				idx_ = 2;
			} else if (idx_ == 2) {
				outBuf_[outBufSize_++] = static_cast<char>(cur_ | (t >> 2));
				cur_ = (t & 3) << 6;
				idx_ = 3;
			} else {
				outBuf_[outBufSize_++] = static_cast<char>(cur_ | t);
				idx_ = 0;
			}
			if (outBufSize_ == sizeof(outBuf_)) {
				base64_local::Write(os_, outBuf_, outBufSize_);
				outBufSize_ = 0;
			}
		}
		return static_cast<ssize_t>(size);
	}
	/**
		flush data
	*/
	void flush()
	{
		if (outBufSize_ > 0) {
			base64_local::Write(os_, outBuf_, outBufSize_);
			outBufSize_ = 0;
		}
	}
};

/**
	base64 decode
	@param os [in] output stream
	@param is [in] intpu stream
*/
template<class OutputStream, class InputStream>
void DecodeFromBase64(OutputStream& os, InputStream& is)
{
	cybozu::Base64Decoder<OutputStream> dec(os);
	for (;;) {
		char buf[1024];
		ssize_t readSize = is.read(buf, sizeof(buf));
		if (readSize <= 0) break;
		dec.write(buf, readSize);
	}
	dec.flush();
}

} // cybozu
