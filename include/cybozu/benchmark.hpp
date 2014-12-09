#pragma once
/**
	@file
	@brief measure exec time of function
	@author MITSUNARI Shigeo
*/
#if defined(_MSC_VER) && (MSC_VER <= 1500)
	#include <cybozu/inttype.hpp>
#else
	#include <stdint.h>
#endif
#include <stdio.h>

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
	#define CYBOZU_BENCH_USE_RDTSC
#endif
#ifdef CYBOZU_BENCH_USE_RDTSC
	#ifdef _MSC_VER
		#include <intrin.h>
	#endif
#else
	#include <time.h>
	#include <assert.h>
#endif

#ifndef CYBOZU_UNUSED
	#ifdef __GNUC__
		#define CYBOZU_UNUSED __attribute__((unused))
	#else
		#define CYBOZU_UNUSED
	#endif
#endif

namespace cybozu {

#ifdef CYBOZU_BENCH_USE_RDTSC
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
	void put(const char *msg = 0, int N = 1) const
	{
		double t = getClock() / double(getCount()) / N;
		if (msg && *msg) printf("%s ", msg);
		if (t > 1e6) {
			printf("%7.3fMclk", t * 1e-6);
		} else if (t > 1e3) {
			printf("%7.3fKclk", t * 1e-3);
		} else {
			printf("%6.2f clk", t);
		}
		if (msg && *msg) printf("\n");
	}
	// adhoc constatns for CYBOZU_BENCH
	static const int loopN1 = 1000;
	static const int loopN2 = 1000000;
	static const uint64_t maxClk = (uint64_t)3e8;
private:
	uint64_t clock_;
	int count_;
};
#else
class CpuClock {
	uint64_t startNsec_;
	uint64_t clock_;
	int count_;
	uint64_t getTimeNsec() const
	{
		struct timespec tp;
		int ret CYBOZU_UNUSED = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);
		assert(ret == 0);
		return tp.tv_sec * 1000000000 + tp.tv_nsec;
	}
public:
	CpuClock() : startNsec_(0), clock_(0), count_(0) { }
	void begin()
	{
		startNsec_ = getTimeNsec();
	}
	/*
		@note QQQ ; this is not same api as rdtsc version
	*/
	void end()
	{
		uint64_t cur = getTimeNsec();
		clock_ += cur - startNsec_;
		startNsec_ = cur;
		count_++;
	}
	int getCount() const { return count_; }
	uint64_t getClock() const { return clock_; }
	void clear() { startNsec_ = 0; clock_ = 0; count_ = 0; }
	void put(const char *msg = 0, int N = 1) const
	{
		double t = getClock() / double(getCount()) / N;
		if (msg && *msg) printf("%s ", msg);
		if (t > 1) {
			printf("%6.2fmsec", t * 1e-6);
		} else if (t > 1e-3) {
			printf("%6.2fusec", t * 1e-3);
		} else {
			printf("%6.2fnsec", t);
		}
		if (msg && *msg) printf("\n");
	}
	// adhoc constatns for CYBOZU_BENCH
	static const int loopN1 = 1000;
	static const int loopN2 = 1000000;
	static const uint64_t maxClk = (uint64_t)3e8;
};
#endif

namespace bench {

static CpuClock g_clk;
static int CYBOZU_UNUSED g_loopNum;

} // cybozu::bench
/*
	loop counter is automatically determined
	CYBOZU_BENCH(<msg>, <func>, <param1>, <param2>, ...);
	if msg == "" then only set g_clk, g_loopNum
*/
#define CYBOZU_BENCH(msg, func, ...) \
{ \
	const uint64_t _cybozu_maxClk = cybozu::CpuClock::maxClk; \
	cybozu::CpuClock _cybozu_clk; \
	for (int _cybozu_i = 0; _cybozu_i < cybozu::CpuClock::loopN2; _cybozu_i++) { \
		_cybozu_clk.begin(); \
		for (int _cybozu_j = 0; _cybozu_j < cybozu::CpuClock::loopN1; _cybozu_j++) { func(__VA_ARGS__); } \
		_cybozu_clk.end(); \
		if (_cybozu_clk.getClock() > _cybozu_maxClk) break; \
	} \
	if (msg && *msg) _cybozu_clk.put(msg, cybozu::CpuClock::loopN1); \
	cybozu::bench::g_clk = _cybozu_clk; cybozu::bench::g_loopNum = cybozu::CpuClock::loopN1; \
}

/*
	loop counter N is given
	CYBOZU_BENCH_C(<msg>, <counter>, <func>, <param1>, <param2>, ...);
	if msg == "" then only set g_clk, g_loopNum
*/
#define CYBOZU_BENCH_C(msg, _N, func, ...) \
{ \
	cybozu::CpuClock _cybozu_clk; \
	_cybozu_clk.begin(); \
	for (int _cybozu_j = 0; _cybozu_j < _N; _cybozu_j++) { func(__VA_ARGS__); } \
	_cybozu_clk.end(); \
	if (msg && *msg) _cybozu_clk.put(msg, _N); \
	cybozu::bench::g_clk = _cybozu_clk; cybozu::bench::g_loopNum = _N; \
}

} // cybozu
