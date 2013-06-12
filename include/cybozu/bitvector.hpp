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

class BitVector {
	size_t bitSize_;
	std::vector<uint64_t> v_;
public:
	BitVector() : bitSize_(0) {}
	BitVector(const uint64_t *buf, size_t bitSize)
	{
		init(buf, bitSize);
	}
	void init(const uint64_t *buf, size_t bitSize)
	{
		resize(bitSize);
		std::copy(buf, buf + v_.size(), &v_[0]);
	}
	void resize(size_t bitSize)
	{
		bitSize_ = bitSize;
		v_.resize((bitSize + 63) / 64);
	}
	bool get(size_t idx) const
	{
		size_t q = idx / 64;
		size_t r = idx % 64;
		if (q >= v_.size()) throw cybozu::Exception("BitVector:get bad idx") << idx;
		return (v_[q] & (uint64_t(1) << r)) != 0;
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
		size_t q = idx / 64;
		size_t r = idx % 64;
		if (q >= v_.size()) throw cybozu::Exception("BitVector:set bad idx") << idx;
		v_[q] |= uint64_t(1) << r;
	}
	// set(idx, false);
	void reset(size_t idx)
	{
		size_t q = idx / 64;
		size_t r = idx % 64;
		if (q >= v_.size()) throw cybozu::Exception("BitVector:reset bad idx") << idx;
		v_[q] &= ~(uint64_t(1) << r);
	}
	size_t size() const { return bitSize_; }
	const uint64_t *getBlock() const { return &v_[0]; }
	uint64_t *getBlock() { return &v_[0]; }
	size_t getBlockSize() const { return v_.size(); }
};

} // cybozu
