#pragma once

/**
	@file
	@brief scoped array and aligned array

	Copyright (C) 2008 Cybozu Labs, Inc., all rights reserved.
*/
#include <new>
#include <utility>
#ifdef _MSC_VER
	#include <malloc.h>
#else
	#include <stdlib.h>
#endif
#include <cybozu/inttype.hpp>
#ifdef CYBOZU_ARRAY_DEBUG_FILL
	#include <memory.h>
#endif

namespace cybozu {

inline void *AlignedMalloc(size_t size, size_t alignment)
{
#ifdef _MSC_VER
	return _aligned_malloc(size, alignment);
#else
	void *p;
	int ret = posix_memalign(&p, alignment, size);
	return (ret == 0) ? p : 0;
#endif
}

inline void AlignedFree(void *p)
{
#ifdef _MSC_VER
	_aligned_free(p);
#else
	free(p);
#endif
}

template<class T>
class ScopedArray {
	T *p_;
	size_t size_;
	ScopedArray(const ScopedArray&);
	void operator=(const ScopedArray&);
public:
	explicit ScopedArray(size_t size)
		: p_(new T[size])
		, size_(size)
	{
	}
	~ScopedArray()
	{
		delete[] p_;
	}
	T& operator[](size_t idx) { return p_[idx]; }
	const T& operator[](size_t idx) const { return p_[idx]; }
	size_t size() const { return size_; }
	T* begin() { return p_; }
	T* end() { return p_ + size_; }
	const T* begin() const { return p_; }
	const T* end() const { return p_ + size_; }
};

/**
	16byte aligment array
	+16 to avoid overrunning by SSE4.2 string operation
*/
template<class T, size_t N = 16>
class AlignedArray {
	T *p_;
	size_t size_;
	void shrink(size_t size)
	{
		const size_t n = size_ - size;
		for (size_t i = 0; i < n; i++) {
			p_[size_ - 1 - i].~T();
		}
		size_ = size;
		if (size > 0) return;
		AlignedFree(p_);
		p_ = 0;
	}
public:
	/*
		don't clear buffer with zero if doClear is false and T = char/int, ...
	*/
	explicit AlignedArray(size_t size = 0, bool doClear = true)
		: p_(0)
		, size_(0)
	{
		resize(size, doClear);
	}
	AlignedArray(const AlignedArray& rhs)
		: p_(0)
		, size_(0)
	{
		*this = rhs;
	}
	AlignedArray& operator=(const AlignedArray& rhs)
	{
		clear();
		resize(rhs.size_, false);
		for (size_t i = 0; i < size_; i++) {
			p_[i] = rhs.p_[i];
		}
		return *this;
	}
#if (CYBOZU_CPP_VERSION == CYBOZU_CPP_VERSION_CPP11)
	AlignedArray(AlignedArray&& rhs) throw()
		: p_(rhs.p_)
		, size_(rhs.size_)
	{
		rhs.p_ = 0;
		rhs.size_ = 0;
	}
	AlignedArray& operator=(AlignedArray&& rhs)
	{
		clear();
		p_ = rhs.p_;
		size_ = rhs.size_;
		rhs.p_ = 0;
		rhs.size_ = 0;
		return *this;
	}
#endif
	/*
		don't clear buffer with zero if doClear is false and T = char/int, ...
	*/
	void resize(size_t size, bool doClear = true)
	{
		if (size == size_) return;
		if (size < size_) {
			shrink(size);
			return;
		}
		clear();
		p_ = static_cast<T*>(AlignedMalloc(size * sizeof(T) + 16, N));
		if (p_ == 0) throw std::bad_alloc();
#ifdef CYBOZU_ARRAY_DEBUG_FILL
		memset(p_, 'x', size * sizeof(T));
#endif
		try {
			for (size_ = 0; size_ < size; size_++) {
				if (doClear) {
					new(&p_[size_]) T();
				} else {
					new(&p_[size_]) T; // don't clear if T = char
				}
			}
		} catch (...) {
			shrink(0);
			throw std::bad_alloc();
		}
	}
	void clear()
	{
		shrink(0);
	}
	~AlignedArray()
	{
		clear();
	}
	void swap(AlignedArray& rhs) throw()
	{
		std::swap(p_, rhs.p_);
		std::swap(size_, rhs.size_);
	}
	T& operator[](size_t idx) { return p_[idx]; }
	const T& operator[](size_t idx) const { return p_[idx]; }
	size_t size() const { return size_; }
	T* begin() { return p_; }
	T* end() { return p_ + size_; }
	const T* begin() const { return p_; }
	const T* end() const { return p_ + size_; }
#if (CYBOZU_CPP_VERSION == CYBOZU_CPP_VERSION_CPP11)
	const T* cbegin() const { return p_; }
	const T* cend() const { return p_ + size_; }
#endif
};

} // cybozu
