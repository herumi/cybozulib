#include <stdio.h>
#include <cybozu/thread.hpp>
#include <cybozu/mutex.hpp>
#include <cybozu/test.hpp>
#include <cybozu/atomic.hpp>

const int N = 1000000;
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
		for (int i = 0; i < N; i++) {
			cybozu::AutoLock al(mutex_);
			s_count++;
		}
	}
};


CYBOZU_TEST_AUTO(autoLock)
{
	cybozu::Mutex mutex_;
	ThreadTest t1(mutex_);
	ThreadTest t2(mutex_);
	t1.beginThread();
	t2.beginThread();
	/*
		verity number of increment
	*/
	puts("check mutex");
	puts("start main");
	for (int i = 0; i < N; i++) {
		cybozu::AutoLock al(mutex_);
		s_count++;
	}
	puts("end main");

	t1.joinThread();
	t2.joinThread();
	cybozu::mfence();
	CYBOZU_TEST_EQUAL(s_count, N * 3);
}

