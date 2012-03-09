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

class A : public cybozu::ThreadBase {
	cybozu::Mutex& mutex_;
public:
	A(cybozu::Mutex& mutex)
		: mutex_(mutex)
	{
	}
	void threadEntry()
	{
		puts("start A");
		puts("lock mutex");
		mutex_.lock();
		puts("wait 100msec");
		cybozu::Sleep(100);
		mutex_.unlock();
		cybozu::Sleep(100);
		CYBOZU_TEST_ASSERT(!mutex_.lockTimeout(10));
		CYBOZU_TEST_ASSERT(mutex_.lockTimeout(10));
		mutex_.unlock();
	}
};

class B : public cybozu::ThreadBase {
	cybozu::Mutex& mutex_;
public:
	B(cybozu::Mutex& mutex)
		: mutex_(mutex)
	{
	}
	void threadEntry()
	{
		puts("start B");
		puts("lock mutex");
		CYBOZU_TEST_ASSERT(!mutex_.lockTimeout(10));
		cybozu::Sleep(100);
		CYBOZU_TEST_ASSERT(mutex_.lockTimeout(10));
		cybozu::Sleep(100);
		mutex_.unlock();
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

CYBOZU_TEST_AUTO(lockTimeout)
{
	cybozu::Mutex mutex_;
	A a(mutex_);
	B b(mutex_);
	a.beginThread();
	b.beginThread();
	a.joinThread();
	b.joinThread();
}
