#pragma once
/**
	@file
	@brief bit vector
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <cybozu/exception.hpp>
#include <algorithm>
#include <vector>

namespace cybozu {

template<class T>
class BitVectorT {
	static const size_t unitSize = sizeof(T) * 8;
	size_t bitSize_;
	std::vector<T> v_;
public:
	BitVectorT() : bitSize_(0) {}
	BitVectorT(const T *buf, size_t bitSize)
	{
		init(buf, bitSize);
	}
	void init(const T *buf, size_t bitSize)
	{
		resize(bitSize);
		std::copy(buf, buf + v_.size(), &v_[0]);
	}
	void resize(size_t bitSize)
	{
		bitSize_ = bitSize;
		v_.resize((bitSize + unitSize - 1) / unitSize);
	}
	bool get(size_t idx) const
	{
		if (idx >= bitSize_) throw cybozu::Exception("BitVectorT:get:bad idx") << idx;
		size_t q = idx / unitSize;
		size_t r = idx % unitSize;
		return (v_[q] & (T(1) << r)) != 0;
	}
	void clear()
	{
		bitSize_ = 0;
		v_.clear();
	}
	void set(size_t idx, bool b)
	{
		if (b) {
			set(idx);
		} else {
			reset(idx);
		}
	}
	// set(idx, true);
	void set(size_t idx)
	{
		if (idx >= bitSize_) throw cybozu::Exception("BitVectorT:set:bad idx") << idx;
		size_t q = idx / unitSize;
		size_t r = idx % unitSize;
		v_[q] |= T(1) << r;
	}
	// set(idx, false);
	void reset(size_t idx)
	{
		if (idx >= bitSize_) throw cybozu::Exception("BitVectorT:reset:bad idx") << idx;
		size_t q = idx / unitSize;
		size_t r = idx % unitSize;
		v_[q] &= ~(T(1) << r);
	}
	size_t size() const { return bitSize_; }
	const T *getBlock() const { return &v_[0]; }
	T *getBlock() { return &v_[0]; }
	size_t getBlockSize() const { return v_.size(); }
};

typedef BitVectorT<size_t> BitVector;

} // cybozu
