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

namespace atoi_local {

template<typename T, size_t n>
T convertToInt(bool *b, const char *p, size_t size, const char (&max)[n], T min, T overflow1, char overflow2)
{
	if (size > 0 && *p) {
		bool isMinus = false;
		size_t i = 0;
		if (*p == '-') {
			isMinus = true;
			i++;
		}
		if (i < size && p[i]) {
			// skip leading zero
			while (i < size && p[i] == '0') i++;
			// check minimum
			if (isMinus && size - i >= n - 1 && memcmp(max, &p[i], n - 1) == 0) {
				if (b) *b = true;
				return min;
			}
			T x = 0;
			for (;;) {
				unsigned char c;
				if (i == size || (c = (unsigned char)p[i]) == '\0') {
					if (b) *b = true;
					return isMinus ? -x : x;
				}
				unsigned int y = c - '0';
				if (y > 9 || x > overflow1 || (x == overflow1 && c >= overflow2)) {
	break;
}
				x = x * 10 + T(y);
				i++;
			}
		}
	}
	if (b) {
		*b = false;
		return 0;
	} else {
		throw cybozu::Exception("atoi::convertToInt") << cybozu::exception::makeString(p, size);
	}
}

template<typename T>
T convertToUint(bool *b, const char *p, size_t size, T overflow1, char overflow2)
{
	if (size > 0 && *p) {
		size_t i = 0;
		// skip leading zero
		while (i < size && p[i] == '0') i++;
		T x = 0;
		for (;;) {
			unsigned char c;
			if (i == size || (c = (unsigned char)p[i]) == '\0') {
				if (b) *b = true;
				return x;
			}
			unsigned int y = c - '0';
			if (y > 9 || x > overflow1 || (x == overflow1 && c >= overflow2)) {
				break;
			}
			x = x * 10 + T(y);
			i++;
		}
	}
	if (b) {
		*b = false;
		return 0;
	} else {
		throw cybozu::Exception("atoi::convertToUint") << cybozu::exception::makeString(p, size);
	}
}

template<typename T>
T convertHexToInt(bool *b, const char *p, size_t size)
{
	if (size > 0 && *p) {
		size_t i = 0;
		T x = 0;
		for (;;) {
			unsigned int c;
			if (i == size || (c = (unsigned char)p[i]) == '\0') {
				if (b) *b = true;
				return x;
			}
			if (c - 'A' <= 'F' - 'A') {
				c = (c - 'A') + 10;
			} else if (c - 'a' <= 'f' - 'a') {
				c = (c - 'a') + 10;
			} else if (c - '0' <= '9' - '0') {
				c = c - '0';
			} else {
				break;
			}
			// avoid overflow
			if (x >= T(1) << (sizeof(T) * 8 - 4)) break;
			x = x * 16 + T(c);
			i++;
		}
	}
	if (b) {
		*b = false;
		return 0;
	} else {
		throw cybozu::Exception("atoi::convertHexToInt") << cybozu::exception::makeString(p, size);
	}
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
