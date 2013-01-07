#pragma once
/**
	@file
	@brief bit operation
*/
#include <cybozu/inttype.hpp>

#if defined(_WIN32)
	#include <intrin.h>
#elif defined(__linux__)
	#include <x86intrin.h>
#elif defined(__GNUC__)
	#include <emmintrin.h>
#endif

namespace cybozu {

static inline int bsf(uint32_t x)
{
#if defined(_WIN32)
	unsigned long out;
	_BitScanForward(&out, x);
	return out;
#else
	return __builtin_ctz(x);
#endif
}

static inline int bsf64(uint64_t x)
{
#if defined(_WIN32)
	unsigned long out;
	_BitScanForward64(&out, x);
	return out;
#else
	return __builtin_ctzl(x);
#endif
}

static inline int bsr(uint32_t x)
{
#if defined(_WIN32)
	unsigned long out;
	_BitScanReverse(&out, x);
	return out;
#else
	return __builtin_clz(x) ^ 0x1f;
#endif
}

static inline int bsr64(uint64_t x)
{
#if defined(_WIN32)
	unsigned long out;
	_BitScanReverse64(&out, x);
	return out;
#else
	return __builtin_clzl(x) ^ 0x3f;
#endif
}

} // cybozu
