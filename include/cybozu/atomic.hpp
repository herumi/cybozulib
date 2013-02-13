#pragma once
/**
	@file
	@brief atomic operation

	Copyright (C) 2007 Cybozu Labs, Inc., all rights reserved.
	@author MITSUNARI Shigeo
*/
#include <cybozu/inttype.hpp>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <intrin.h>
#else
#include <emmintrin.h>
#endif

namespace cybozu {

/**
	atomic operation
	see http://gcc.gnu.org/onlinedocs/gcc-4.4.0/gcc/Atomic-Builtins.html
	http://msdn.microsoft.com/en-us/library/ms683504(VS.85).aspx
*/
template<class T>
T AtomicAdd(T *p, T y);
/**
	tmp = *p;
	*p += y;
	return tmp;
*/
template<>
int AtomicAdd<int>(int *p, int y)
{
#ifdef _WIN32
	return _InterlockedExchangeAdd((long*)p, y);
#else
	return __sync_fetch_and_add(p, y);
#endif
}

template<>
unsigned int AtomicAdd<unsigned int>(unsigned int *p, unsigned int y)
{
	return (unsigned int)AtomicAdd((int*)p, (int)y);
}

#ifdef CYBOZU_64BIT

template<>
int64_t AtomicAdd<int64_t>(int64_t *p, int64_t y)
{
#ifdef _WIN32
	return _InterlockedExchangeAdd64(p, y);
#else
	return __sync_fetch_and_add(p, y);
#endif
}

template<>
uint64_t AtomicAdd<uint64_t>(uint64_t *p, uint64_t y)
{
	return (uint64_t)AtomicAdd<int64_t>((int64_t*)p, (int64_t)y);
}
#endif

/**
	tmp = *p;
	if (*p == oldValue) *p = newValue;
	return tmp;
*/
template<class T>
T AtomicCompareExchange(T *p, T newValue, T oldValue);

template<>
int AtomicCompareExchange<int>(int *p, int newValue, int oldValue)
{
#ifdef _WIN32
	return InterlockedCompareExchange((long*)p, newValue, oldValue);
#else
	return __sync_val_compare_and_swap(p, oldValue, newValue);
#endif
}

template<>
unsigned int AtomicCompareExchange<unsigned int>(unsigned int *p, unsigned int newValue, unsigned int oldValue)
{
	return (unsigned int)AtomicCompareExchange((int*)p, (int)newValue, (int)oldValue);
}

template<>
int64_t AtomicCompareExchange<int64_t>(int64_t *p, int64_t newValue, int64_t oldValue)
{
#ifdef _WIN32
	return _InterlockedCompareExchange64(p, newValue, oldValue);
#else
	return __sync_val_compare_and_swap(p, oldValue, newValue);
#endif
}

template<>
uint64_t AtomicCompareExchange<uint64_t>(uint64_t *p, uint64_t newValue, uint64_t oldValue)
{
	return (uint64_t)AtomicCompareExchange<int64_t>((int64_t*)p, (int64_t)newValue, (int64_t)oldValue);
}

template<>
void* AtomicCompareExchange<void*>(void **p, void *newValue, void *oldValue)
{
#ifdef CYBOZU_32BIT
	return (void*)AtomicCompareExchange<int>((int*)p, (int)newValue, (int)oldValue);
#else
	return (void*)AtomicCompareExchange<int64_t>((int64_t*)p, (int64_t)newValue, (int64_t)oldValue);
#endif
}

/**
	tmp *p;
	*p = newValue;
	return tmp;
*/
template<class T>
T AtomicExchange(T *p, T newValue);

template<>
int AtomicExchange<int>(int *p, int newValue)
{
#ifdef _WIN32
	return InterlockedExchange((long*)p, newValue);
#else
	return __sync_lock_test_and_set(p, newValue);
#endif
}

template<>
unsigned int AtomicExchange<unsigned int>(unsigned int *p, unsigned int newValue)
{
	return (unsigned int)AtomicExchange((int*)p, (int)newValue);
}

#ifdef CYBOZU_64BIT

template<>
int64_t AtomicExchange<int64_t>(int64_t *p, int64_t newValue)
{
#ifdef _WIN32
	return _InterlockedExchange64(p, newValue);
#else
	return __sync_lock_test_and_set(p, newValue);
#endif
}

template<>
uint64_t AtomicExchange<uint64_t>(uint64_t *p, uint64_t newValue)
{
	return (uint64_t)AtomicExchange<int64_t>((int64_t*)p, (int64_t)newValue);
}
#endif

template<>
void* AtomicExchange<void*>(void **p, void *newValue)
{
#ifdef CYBOZU_32BIT
	return (void*)AtomicExchange<int>((int*)p, (int)newValue);
#else
	return (void*)AtomicExchange<int64_t>((int64_t*)p, (int64_t)newValue);
#endif
}

inline void mfence()
{
	_mm_mfence();
}
} // cybozu
