#include <stdio.h>
#include <cybozu/test.hpp>
#include <cybozu/nlp/random.hpp>
#include <math.h>

CYBOZU_TEST_AUTO(random)
{
	double sum = 0;
	double sqsum = 0;
	const int n = 1000000;

	cybozu::nlp::NormalRandomGenerator g;
	for (int i = 0; i < n; i++) {
		double v = g.get();
		sum += v;
		sqsum += v * v;
	}
	double m = sum / n;
	double s = sqsum / n - m * m;
	printf("m=%f, s=%f\n", m, s);
	CYBOZU_TEST_ASSERT(fabs(m) < 2e-3);
	CYBOZU_TEST_ASSERT(fabs(s - 1) < 2e-3);
}
