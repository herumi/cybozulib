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

namespace boost {

template<class T>
struct hash;

} // boost

namespace std { CYBOZU_NAMESPACE_TR1_BEGIN

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4099) // missmatch class and struct
#endif
template<class T>
struct hash;
#ifdef _MSC_VER
	#pragma warning(pop)
#endif

CYBOZU_NAMESPACE_TR1_END } // std
