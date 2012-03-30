#pragma once
/**
	@file
	@brief normal random generator

	Copyright (C) 2007-2012 Cybozu Labs, Inc., all rights reserved.
	@author MITSUNARI Shigeo
*/

namespace cybozu { namespace nlp {

/*
	xor shift
*/
class UniformRandomGenerator {
	double a_;
	double b_;
	unsigned int x_, y_, z_, w_;
public:
	/* generate uniform random value in [a, b) */
	explicit UniformRandomGenerator(double a = 0, double b = 1, int seed = 0)
		: a_(a)
		, b_(b)
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
	/* [a, b) random number */
	double getDouble()
	{
		unsigned int x = get() >> 5;
		unsigned int y = get() >> 6;
		double z = (x * double(1U << 26) + y) * (1.0 / double(1LL << 53));
		return (b_ - a_) * z + a_;
	}
};
/*
	normal random generator
*/
class NormalRandomGenerator {
	UniformRandomGenerator gen_;
	double u_;
	double s_;
public:
	explicit NormalRandomGenerator(double u = 0, double s = 1, int seed = 0)
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
