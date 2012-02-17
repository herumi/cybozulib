#pragma once
/**
	@file
	@brief normal random generator

	Copyright (C) 2007-2012 Cybozu Inc., all rights reserved.
	@author MITSUNARI Shigeo
*/

namespace cybozu { namespace nlp {

/*
	xor shift
*/
class RandomGenerator {
	unsigned int x_, y_, z_, w_;
public:
	RandomGenerator(int seed = 0)
	{
		init(seed);
	}
	void init(int seed = 0)
	{
		x_ = 123456789 + seed;
		y_ = 362436069;
		z_ = 521288629;
		w_ = 88675123;
	}
	/* [0, 2^32) random number */
	unsigned int get()
	{
		unsigned int t = x_ ^ (x_ << 11);
		x_ = y_; y_ = z_; z_ = w_;
		return w_ = (w_ ^ (w_ >> 19)) ^ (t ^ (t >> 8));
	}
	/* [0, 1) random number */
	double getDouble()
	{
		unsigned int a = get() >> 5;
		unsigned int b = get() >> 6;
		return (a * double(1U << 26) + b) * (1.0 / double(1LL << 53));
	}
};
/*
	normal random generator
*/
class NormalRandomGenerator {
	RandomGenerator gen_;
	double u_;
	double s_;
public:
	NormalRandomGenerator(double u = 0, double s = 1, int seed = 0)
		: gen_(seed)
		, u_(u)
		, s_(s)
	{
	}
	void init(int seed = 0)
	{
		gen_.init(seed);
	}
	double get()
	{
		double sum = -6;
		for (int i = 0; i < 12; i++) {
			sum += gen_.getDouble();
		}
		return sum * s_ + u_;
	}
};

} } // cybozu::nlp
