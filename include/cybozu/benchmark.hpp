#pragma once
/**
	@file
	@brief measure exec time of function
	@author MITSUNARI Shigeo
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

namespace benchmark_local {

inline void putClk(const char *msg, const cybozu::CpuClock& clk, int N)
{
	double t = clk.getClock() / double(clk.getCount()) / N;
	if (msg && *msg) printf("%s ", msg);
	if (t > 1e6) {
		printf("%6.2f Mclk", t * 1e-6);
	} else if (t > 1e3) {
		printf("%6.2f Kclk", t * 1e-3);
	} else {
		printf("%6.2f  clk", t);
	}
	if (msg && *msg) printf("\n");
}

} // benchmark_local

/*
	CYBOZU_BENCH(<msg>, <func>, <param1>, <param2>, ...);
	if msg != 0 then print '<msg> <clk>\n' else '<clk>'
*/
#define CYBOZU_BENCH(msg, func, ...) \
{ \
	const uint64_t MAX_CLK = (uint64_t)3e8; \
	const int N = 1000; \
	cybozu::CpuClock clk; \
	for (int i = 0; i < 1000000; i++) { \
		clk.begin(); \
		for (int j = 0; j < N; j++) func(__VA_ARGS__); \
		clk.end(); \
		if (clk.getClock() > MAX_CLK) break; \
	} \
	cybozu::benchmark_local::putClk(msg, clk, N); \
}

} // cybozu
