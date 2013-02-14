#include <stdio.h>
#include <cybozu/parallel.hpp>
#include <cybozu/test.hpp>
#include <math.h>

struct X {
	int x;
	int processedNum;
	X()
		: x(0)
		, processedNum(0)
	{
	}
	void operator()(int idx, int /*threadIdx*/)
	{
		double ret = 0;
		const int N = 40000;
		for (int i = 0; i < N; i++) {
			ret += sin(double(i) / N) * idx;
		}
		cybozu::AtomicAdd(&processedNum, 1);
		cybozu::AtomicAdd(&x, (int)ret);
	}
};

void test(int n, int threadNum)
{
	X x;
	cybozu::parallel_for(x, n, threadNum);
	CYBOZU_TEST_EQUAL(x.processedNum, n);
}

CYBOZU_TEST_AUTO(parallel)
	try
{
	for (int threadNum = 1; threadNum < 16; threadNum++) {
		printf("threadNum=%d\n", threadNum);
		for (int n = 1; n < 1000; n += 10) {
			test(n, threadNum);
		}
	}
} catch (std::exception& e) {
	printf("err %s\n", e.what());
}
