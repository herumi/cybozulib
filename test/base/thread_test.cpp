#include <stdio.h>
#include <cybozu/thread.hpp>
#include <cybozu/mutex.hpp>
#include <cybozu/test.hpp>

const int N = 100000;
int s_count = 0;

class ThreadTest : public cybozu::ThreadBase {
	cybozu::Mutex& mutex_;
public:
	ThreadTest(cybozu::Mutex& mutex)
		: mutex_(mutex)
	{
	}

	void threadEntry()
	{
		puts("start threadEntry");
		for (int i = 0; i < N; i++) {
			int a;
			cybozu::AutoLock al(mutex_);
			a = s_count;
			s_count = a + 1;
		}
		puts("wait threadEntry");
	}
};

CYBOZU_TEST_AUTO(autoLock)
{
	cybozu::Mutex mutex_;
	ThreadTest test(mutex_);
	test.beginThread();
	/*
		verity number of increment
	*/
	puts("check mutex");
	puts("start main");
	for (int i = 0; i < N; i++) {
		int a;
		cybozu::AutoLock al(mutex_);
		a = s_count;
		s_count = a + 1;
	}
	puts("end main");

	test.joinThread();
	CYBOZU_TEST_EQUAL(s_count, N * 2);
}
