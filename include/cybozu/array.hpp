#pragma once

/**
	@file
	@brief scoped array and aligned array

	Copyright (C) 2008-2012 Cybozu Labs, Inc., all rights reserved.
*/
#include <new>
#ifdef _MSC_VER
	#include <malloc.h>
#else
	#include <stdlib.h>
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
	+16 for SSE4.2 string operation overrun
*/
template<class T>
class AlignedArray {
	T *p_;
	size_t size_;
	AlignedArray(const AlignedArray&);
	void operator=(const AlignedArray&);
public:
	explicit AlignedArray(size_t size)
		: p_((T*)AlignedMalloc(size * sizeof(T) + 16, 16))
		, size_(size)
	{
		if (p_ == 0) throw std::bad_alloc();
		bool init = true;
		size_t initSize = 0;
		try {
			while (initSize < size_) {
				if (new(&p_[initSize]) T() == 0) {
					init = false;
					break;
				}
				initSize++;
			}
		} catch (...) {
			init = false;	
		}
		if (!init) {
			for (size_t i = 0; i < initSize; i++) {
				p_[initSize - 1 - i].~T();
			}
			AlignedFree(p_);
			throw std::bad_alloc();
		}
	}
	~AlignedArray()
	{
		for (size_t i = 0; i < size_; i++) {
			p_[i].~T();
		}
		AlignedFree(p_);
	}
	T& operator[](size_t idx) { return p_[idx]; }
	const T& operator[](size_t idx) const { return p_[idx]; }
	size_t size() const { return size_; }
	T* begin() { return p_; }
	T* end() { return p_ + size_; }
	const T* begin() const { return p_; }
	const T* end() const { return p_ + size_; }
};

} // cybozu

