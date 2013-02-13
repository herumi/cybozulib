#pragma once
/**
	@file
	@brief XorShift

	Copyright (C) 2007 Cybozu Labs, Inc., all rights reserved.
	@author MITSUNARI Shigeo
*/
#include <cybozu/inttype.hpp>

namespace cybozu {

class XorShift {
	uint32_t x_, y_, z_, w_;
public:
	explicit XorShift(uint32_t x = 0, uint32_t y = 0, uint32_t z = 0, uint32_t w = 0)
	{
		init(x, y, z, w);
	}
	void init(uint32_t x = 0, uint32_t y = 0, uint32_t z = 0, uint32_t w = 0)
	{
		x_ = x ? x : 123456789;
		y_ = y ? y : 362436069;
		z_ = z ? z : 521288629;
		w_ = w ? w : 88675123;
	}
	uint32_t get32()
	{
		unsigned int t = x_ ^ (x_ << 11);
		x_ = y_; y_ = z_; z_ = w_;
		return w_ = (w_ ^ (w_ >> 19)) ^ (t ^ (t >> 8));
	}
	uint32_t operator()() { return get32(); }
	uint64_t get64()
	{
		uint32_t a = get32();
		uint32_t b = get32();
		return (uint64_t(a) << 32) | b;
	}
	template<class T>
	void read(T *x, size_t n)
	{
		const size_t size = sizeof(T) * n;
		const size_t q = size / sizeof(uint32_t);
		const size_t r = size % sizeof(uint32_t);
		uint32_t *p32 = (uint32_t*)&x;
		for (size_t i = 0; i < q; i++) {
			p32[i] = get32();
		}
		uint8_t *p8 = (uint8_t*)&p32[q];
		for (size_t i = 0; i < r; i++) {
			p8[i] = (uint8_t)get32();
		}
	}
};

} // cybozu
