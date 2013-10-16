#include <cybozu/array.hpp>
#include <cybozu/test.hpp>

int a_count = 0;
int b_count = 0;

struct A {
	int n;
	A()
		: n(a_count)
	{
		a_count++;
	}
	~A()
	{
		a_count--;
	}
};

struct B {
	std::string x;
	B()
	{
		if (b_count == 3) throw std::bad_exception();
		b_count++;
		x = "dummy";
	}
	~B()
	{
		b_count--;
	}
};

CYBOZU_TEST_AUTO(scoped_array)
{
	{
		cybozu::ScopedArray<A> a(10);
		CYBOZU_TEST_EQUAL(a_count, 10);
		for (int i = 0; i < 10; i++) {
			CYBOZU_TEST_EQUAL(a[i].n, i);
		}
	}
	CYBOZU_TEST_EQUAL(a_count, 0);
}

CYBOZU_TEST_AUTO(aligned_array)
{
	{
		cybozu::AlignedArray<A> a(10);
		CYBOZU_TEST_EQUAL(a_count, 10);
		CYBOZU_TEST_EQUAL((reinterpret_cast<uintptr_t>(&a[0])) & 15, static_cast<uintptr_t>(0));
		for (int i = 0; i < 10; i++) {
			CYBOZU_TEST_EQUAL(a[i].n, i);
		}
	}
	CYBOZU_TEST_EQUAL(a_count, 0);
}

CYBOZU_TEST_AUTO(aligned_array_char)
{
	{
		cybozu::AlignedArray<char> a(10);
		CYBOZU_TEST_EQUAL((reinterpret_cast<uintptr_t>(&a[0])) & 15, static_cast<uintptr_t>(0));
		for (int i = 0; i < 10; i++) {
			CYBOZU_TEST_EQUAL(a[i], 0);
		}
	}
}

CYBOZU_TEST_AUTO(resize)
{
	{
		cybozu::AlignedArray<char> a(10);
		CYBOZU_TEST_EQUAL((reinterpret_cast<uintptr_t>(&a[0])) & 15, static_cast<uintptr_t>(0));
		a.resize(123);
		CYBOZU_TEST_EQUAL((reinterpret_cast<uintptr_t>(&a[0])) & 15, static_cast<uintptr_t>(0));
	}
}

CYBOZU_TEST_AUTO(aligned_array_exception)
{
	CYBOZU_TEST_EXCEPTION(cybozu::AlignedArray<B> a(4), std::exception);
	CYBOZU_TEST_EQUAL(b_count, 0);
}

