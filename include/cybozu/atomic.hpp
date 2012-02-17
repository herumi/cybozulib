#pragma once
/**
	@file
	@brief atomic operation

	Copyright (C) 2007-2012 Cybozu Inc., all rights reserved.
	@author MITSUNARI Shigeo
*/
#include <cybozu/inttype.hpp>
#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#endif

namespace cybozu {

/**
	atomic operation
	see http://gcc.gnu.org/onlinedocs/gcc-4.4.0/gcc/Atomic-Builtins.html
	http://msdn.microsoft.com/en-us/library/ms683504(VS.85).aspx
*/
/**
	tmp = *p;
	*p += y;
	return tmp;
*/
inline int AtomicAdd(int *p, int y)
{
#ifdef _WIN32
	return _InterlockedExchangeAdd((long*)p, y);
#else
	return __sync_fetch_and_add(p, y);
#endif
}

inline unsigned int AtomicAdd(unsigned int *p, unsigned int y)
{
	return (unsigned int)AtomicAdd((int*)p, (int)y);
}

#if defined(_WIN64) || defined(__x86_64__)
inline int64_t AtomicAdd(int64_t *p, int64_t y)
{
#ifdef _WIN32
	return _InterlockedExchangeAdd64(p, y);
#else
	return __sync_fetch_and_add(p, y);
#endif
}

inline uint64_t AtomicAdd(uint64_t *p, uint64_t y)
{
	return (uint64_t)AtomicAdd((int64_t*)p, (int64_t)y);
}
#endif

/**
	tmp = *p;
	if (*p == oldValue) *p = newValue;
	return tmp;
*/
inline int AtomicCompareExchange(int *p, int newValue, int oldValue)
{
#ifdef _WIN32
	return InterlockedCompareExchange((long*)p, newValue, oldValue);
#else
	return __sync_val_compare_and_swap(p, oldValue, newValue);
#endif
}

inline int64_t AtomicCompareExchange(int64_t *p, int64_t newValue, int64_t oldValue)
{
#ifdef _WIN32
	return _InterlockedCompareExchange64(p, newValue, oldValue);
#else
	return __sync_val_compare_and_swap(p, oldValue, newValue);
#endif
}

inline unsigned int AtomicCompareExchange(unsigned int *p, unsigned int newValue, unsigned int oldValue)
{
	return (unsigned int)AtomicCompareExchange((int*)p, (int)newValue, (int)oldValue);
}

inline uint64_t AtomicCompareExchange(uint64_t *p, uint64_t newValue, uint64_t oldValue)
{
	return (uint64_t)AtomicCompareExchange((int64_t*)p, (int64_t)newValue, (int64_t)oldValue);
}

/**
	tmp *p;
	*p = newValue;
	return tmp;
*/
inline int AtomicExchange(int *p, int newValue)
{
#ifdef _WIN32
	return InterlockedExchange((long*)p, newValue);
#else
	return __sync_lock_test_and_set(p, newValue);
#endif
}

inline unsigned int AtomicExchange(unsigned int *p, unsigned int newValue)
{
	return (unsigned int)AtomicExchange((int*)p, (int)newValue);
}

inline size_t AtomicExchangeSize_t(size_t *p, size_t newValue)
{
#ifdef _WIN32
	#ifdef _WIN64
		return (size_t)_InterlockedExchange64((__int64*)p, (__int64)newValue);
	#else
		return AtomicExchange((int*)p, (int)newValue);
	#endif
#else
	return __sync_lock_test_and_set(p, newValue);
#endif
}

} // cybozu
