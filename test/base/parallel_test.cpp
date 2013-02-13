#include <stdio.h>
#include <cybozu/parallel.hpp>
#include <cybozu/test.hpp>
#include <math.h>

struct X {
	int x_;
	X()
		: x_(0)
	{
	}
	void f(int x)
	{
		double ret = 0;
		const int N = 10000;
		for (int i = 0; i < N; i++) {
			ret += sin(double(i) / N) * x;
		}
		cybozu::AtomicAdd(&x_, (int)ret);
	}
};

void test(int n, int threadNum)
{
	X x;
	int processedNum = 0;
	cybozu::parallel_for(x, &X::f, n, threadNum, &processedNum);
	CYBOZU_TEST_EQUAL(processedNum, n);
}

CYBOZU_TEST_AUTO(parallel)
	try
{
	for (int threadNum = 1; threadNum < 8; threadNum++) {
		printf("threadNum=%d\n", threadNum);
		for (int n = 1; n < 1000; n += 10) {
			test(n, threadNum);
		}
	}
} catch (std::exception& e) {
	printf("err %s\n", e.what());
}
