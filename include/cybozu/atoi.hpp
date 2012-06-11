#pragma once
/**
	@file
	@brief converter between integer and string

	Copyright (C) 2008 Cybozu Labs, Inc., all rights reserved.
*/

#include <memory.h>
#include <limits.h>
#include <cybozu/exception.hpp>

namespace cybozu {

struct AtoiException : public cybozu::Exception {
	AtoiException() : cybozu::Exception("atoi") { }
};

namespace atoi_local {

template<typename T, size_t n>
T convertToInt(bool *b, const char *p, size_t size, const char (&max)[n], T min, T overflow1, char overflow2)
{
	if (b) *b = true;
	const char *keepP = p;
	const size_t keepSize = size;
	bool isMinus = false;
	if (*p == '-') {
		isMinus = true;
		p++;
		size--;
	}
	if (size > 0 && *p) {
		// skip leading zero
		while (size > 0 && *p == '0') {
			p++;
			size--;
		}
		// check minimum
		if (isMinus && size >= n - 1 && memcmp(max, p, n - 1) == 0) {
			return min;
		}
		T x = 0;
		for (;;) {
			char c = *p;
			if (size == 0 || c == '\0') {
				return isMinus ? -x : x;
			}
			unsigned int y = c - '0';
			if (y <= 9) {
				if (x > overflow1 || (x == overflow1 && c >= overflow2)) {
					break;
				}
				x *= 10;
				x += (unsigned char)y;
			} else {
				break;
			}
			p++;
			size--;
		}
	}
	if (b) {
		*b = false;
		return 0;
	} else {
		cybozu::AtoiException e; e << "convertToInt" << cybozu::exception::makeString(keepP, keepSize);
		throw e;
	}
}

template<typename T>
T convertToUint(bool *b, const char *p, size_t size, T overflow1, char overflow2)
{
	if (b) *b = true;
	const char *keepP = p;
	const size_t keepSize = size;
	if (size > 0 && *p) {
		// skip leading zero
		while (size > 0 && *p == '0') {
			p++;
			size--;
		}
		T x = 0;
		for (;;) {
			char c = *p;
			if (size == 0 || c == '\0') {
				return x;
			}
			unsigned int y = c - '0';
			if (y <= 9) {
				if (x > overflow1 || (x == overflow1 && c >= overflow2)) {
					break;
				}
				x *= 10;
				x += (unsigned char)y;
			} else {
				break;
			}
			p++;
			size--;
		}
	}
	if (b) {
		*b = false;
		return 0;
	} else {
		cybozu::AtoiException e; e << "convertToUint" << cybozu::exception::makeString(keepP, keepSize);
		throw e;
	}
}

template<typename T>
T convertHexToInt(bool *b, const char *p, size_t size)
{
	bool isOK = true;
	T x = 0;
	size_t i = 0;
	while (i < size) {
		char c = p[i];
		if (c == '\0') {
			break;
		}
		if (i >= sizeof(T) * 2) {
			isOK = false;
			break;
		}
		if ('A' <= c && c <= 'F') {
			c = (c - 'A') + 10;
		} else if ('a' <= c && c <= 'f') {
			c = (c - 'a') + 10;
		} else if ('0' <= c && c <= '9') {
			c = c - '0';
		} else {
			isOK = false;
			break;
		}
		x = x * 16 + (unsigned int)c;
		i++;
	}
	if (i == 0) isOK = false;
	if (b) *b = isOK;
	if (isOK) return x;
	cybozu::AtoiException e; e << "convertHexToInt" << cybozu::exception::makeString(p, size);
	throw e;
}

} // atoi_local

/**
	auto detect return value class
	@note if you set bool pointer p then throw nothing and set *p = false if bad string
*/
class atoi {
	const char *p_;
	size_t size_;
	bool *b_;
	void set(bool *b, const char *p, size_t size)
	{
		b_ = b;
		p_ = p;
		size_ = size;
	}
public:
	atoi(const char *p, size_t size = -1)
	{
		set(0, p, size);
	}
	atoi(bool *b, const char *p, size_t size = -1)
	{
		set(b, p, size);
	}
	atoi(const std::string& str)
	{
		set(0, str.c_str(), str.size());
	}
	atoi(bool *b, const std::string& str)
	{
		set(b, str.c_str(), str.size());
	}

	inline operator short() const
	{
		return atoi_local::convertToInt<short>(b_, p_, size_, "32768", -32768, 3276, '8');
	}
	inline operator unsigned short() const
	{
		return atoi_local::convertToUint<unsigned short>(b_, p_, size_, 6553, '6');
	}

	inline operator int() const
	{
		return atoi_local::convertToInt<int>(b_, p_, size_, "2147483648", INT_MIN, 214748364, '8');
	}

	inline operator int64_t() const
	{
		return atoi_local::convertToInt<int64_t>(b_, p_, size_, "9223372036854775808", LLONG_MIN, 922337203685477580LL, '8');
	}

	inline operator unsigned int() const
	{
		return atoi_local::convertToUint<unsigned int>(b_, p_, size_, 429496729, '6');
	}

	inline operator uint64_t() const
	{
		return atoi_local::convertToUint<uint64_t>(b_, p_, size_, 1844674407370955161ULL, '6');
	}
};

class hextoi {
	const char *p_;
	size_t size_;
	bool *b_;
	void set(bool *b, const char *p, size_t size)
	{
		b_ = b;
		p_ = p;
		size_ = size;
	}
public:
	hextoi(const char *p, size_t size = -1)
	{
		set(0, p, size);
	}
	hextoi(bool *b, const char *p, size_t size = -1)
	{
		set(b, p, size);
	}
	hextoi(const std::string& str)
	{
		set(0, str.c_str(), str.size());
	}
	hextoi(bool *b, const std::string& str)
	{
		set(b, str.c_str(), str.size());
	}
	operator unsigned char() const { return atoi_local::convertHexToInt<unsigned char>(b_, p_, size_); }
	operator unsigned short() const { return atoi_local::convertHexToInt<unsigned short>(b_, p_, size_); }
	operator unsigned int() const { return atoi_local::convertHexToInt<unsigned int>(b_, p_, size_); }
	operator uint64_t() const { return atoi_local::convertHexToInt<uint64_t>(b_, p_, size_); }
};

} // cybozu
