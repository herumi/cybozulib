#pragma once
#include <cybozu/inttype.hpp>

namespace cybozu {

template<class T>
uint32_t hash32(const T *x, size_t n, uint32_t v = 0)
{
	if (v == 0) v = 2166136261U;
	for (size_t i = 0; i < n; i++) {
		v ^= x[i];
		v *= 16777619;
	}
	return v;
}
template<class T>
uint64_t hash64(const T *x, size_t n, uint64_t v = 0)
{
	if (v == 0) v = 14695981039346656037ULL;
	for (size_t i = 0; i < n; i++) {
		v ^= x[i];
		v *= 1099511628211ULL;
	}
	v ^= v >> 32;
	return v;
}

} // cybozu

