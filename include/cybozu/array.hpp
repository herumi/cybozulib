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
	T& operator[](size_t idx) throw() { return p_[idx]; }
	const T& operator[](size_t idx) const throw() { return p_[idx]; }
	size_t size() const throw() { return size_; }
	bool empty() const throw() { return size_ == 0; }
	T* begin() throw() { return p_; }
	T* end() throw() { return p_ + size_; }
	const T* begin() const throw() { return p_; }
	const T* end() const throw() { return p_ + size_; }
    T* data() throw() { return p_; }
    const T* data() const throw() { return p_; }
};

/**
	T must be POD type
	16byte aligment array
*/
template<class T, size_t N = 16, bool defaultDoClear = true>
class AlignedArray {
	T *p_;
	size_t size_;
	T *alloc(size_t size, bool doClear) const
	{
		// +16 to avoid overrunning by SSE4.2 string operation
		T *p = static_cast<T*>(AlignedMalloc(size * sizeof(T) + 16, N));
		if (p == 0) throw std::bad_alloc();
		if (doClear) {
			for (size_t i = 0; i < size; i++) {
				p[i] = 0;
			}
		}
		return p;
	}
	void copy(T *dst, const T *src) const
	{
		for (size_t i = 0; i < size_; i++) {
			dst[i] = src[i];
		}
	}
public:
	/*
		don't clear buffer with zero if doClear is false and T = char/int, ...
	*/
	explicit AlignedArray(size_t size = 0, bool doClear = defaultDoClear)
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
		p_ = alloc(rhs.size_, false);
		size_ = rhs.size_;
		copy(p_, rhs.p_);
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
	AlignedArray& operator=(AlignedArray&& rhs) throw()
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
		don't clear buffer with zero if doClear is false
	*/
	void resize(size_t size, bool doClear = defaultDoClear)
	{
		if (size == size_) return;
		// shrink
		if (size < size_) {
			size_ = size;
			if (size == 0) {
				AlignedFree(p_);
				p_ = 0;
			}
			return;
		}
		// extend
		T *p = alloc(size, doClear);
		copy(p, p_);
		AlignedFree(p_);
		p_ = p;
		size_ = size;
	}
	void clear()
	{
		resize(0, false);
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
	T& operator[](size_t idx) throw() { return p_[idx]; }
	const T& operator[](size_t idx) const throw() { return p_[idx]; }
	size_t size() const throw() { return size_; }
	bool empty() const throw() { return size_ == 0; }
	T* begin() throw() { return p_; }
	T* end() throw() { return p_ + size_; }
	const T* begin() const throw() { return p_; }
	const T* end() const throw() { return p_ + size_; }
    T* data() throw() { return p_; }
    const T* data() const throw() { return p_; }
#if (CYBOZU_CPP_VERSION == CYBOZU_CPP_VERSION_CPP11)
	const T* cbegin() const throw() { return p_; }
	const T* cend() const throw() { return p_ + size_; }
#endif
};

} // cybozu
