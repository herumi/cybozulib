#pragma once
/**
	@file
	@brief cpu clock

	Copyright (C) 2008 Cybozu Labs, Inc., all rights reserved.
*/
#include <cybozu/inttype.hpp>
#ifdef _MSC_VER
	#include <intrin.h>
#endif

namespace cybozu {

class CpuClock {
public:
	static inline uint64_t getRdtsc()
	{
#ifdef _MSC_VER
		return __rdtsc();
#else
		unsigned int eax, edx;
		__asm__ volatile("rdtsc" : "=a"(eax), "=d"(edx));
		return ((uint64_t)edx << 32) | eax;
#endif
	}
	CpuClock()
		: clock_(0)
		, count_(0)
	{
	}
	void begin()
	{
		clock_ -= getRdtsc();
	}
	void end()
	{
		clock_ += getRdtsc();
		count_++;
	}
	int getCount() const { return count_; }
	uint64_t getClock() const { return clock_; }
	void clear() { count_ = 0; clock_ = 0; }
private:
	uint64_t clock_;
	int count_;
};

} // cybozu
