/*
	debug for AlignedAlloc with doClear = false
*/
#define CYBOZU_ARRAY_DEBUG_FILL

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
			CYBOZU_TEST_EQUAL(a.data()[i].n, i);
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
			CYBOZU_TEST_EQUAL(a.data()[i].n, i);
		}
	}
	CYBOZU_TEST_EQUAL(a_count, 0);
}

CYBOZU_TEST_AUTO(aligned_array_char)
{
	{
		cybozu::AlignedArray<char> a(51);
		CYBOZU_TEST_EQUAL((reinterpret_cast<uintptr_t>(&a[0])) & 15, static_cast<uintptr_t>(0));
		for (size_t i = 0; i < a.size(); i++) {
			CYBOZU_TEST_EQUAL(a[i], 0);
		}
	}
}

CYBOZU_TEST_AUTO(aligned_array_char_nonclear)
{
	cybozu::AlignedArray<char> a(51, false);
	for (size_t i = 0; i < a.size(); i++) {
		CYBOZU_TEST_EQUAL(a[i], 'x');
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


int g_x = 0;

struct X {
	int v;
	X() : v(g_x++) { }
	~X() { v = 0; g_x--; }
};

CYBOZU_TEST_AUTO(AlignedArray_resize)
{
	cybozu::AlignedArray<X> x;
	x.resize(10);
	const X* x0 = &x[0];
	CYBOZU_TEST_EQUAL(g_x, 10);
	for (int i = 0; i < 10; i++) {
		CYBOZU_TEST_EQUAL(x[i].v, i);
	}
	x.resize(3);
	CYBOZU_TEST_EQUAL(g_x, 3);
	const X* x1 = &x[0];
	CYBOZU_TEST_EQUAL(x0, x1);
	for (int i = 0; i < 3; i++) {
		CYBOZU_TEST_EQUAL(x[i].v, i);
	}
	for (int i = 3; i < 10; i++) {
		CYBOZU_TEST_EQUAL(x[i].v, 0);
	}
	x.resize(5);
	CYBOZU_TEST_EQUAL(g_x, 5);
	for (int i = 0; i < 5; i++) {
		CYBOZU_TEST_EQUAL(x[i].v, i);
	}
}

CYBOZU_TEST_AUTO(AlignedArray_copy)
{
	cybozu::AlignedArray<X> x, z, y;
	x.resize(10);
	y.resize(5);

	y = x;
	z = x;
	cybozu::AlignedArray<X> w(x);
	CYBOZU_TEST_EQUAL(x.size(), y.size());
	CYBOZU_TEST_EQUAL(x.size(), z.size());
	CYBOZU_TEST_EQUAL(x.size(), w.size());
	for (size_t i = 0; i < x.size(); i++) {
		CYBOZU_TEST_EQUAL(x[i].v, y[i].v);
		CYBOZU_TEST_EQUAL(x[i].v, z[i].v);
		CYBOZU_TEST_EQUAL(x[i].v, w[i].v);
	}
}
#if (CYBOZU_CPP_VERSION == CYBOZU_CPP_VERSION_CPP11)
CYBOZU_TEST_AUTO(AlignedArray_move)
{
	{
		cybozu::AlignedArray<X> x, y;
		x.resize(10);
		y.resize(5);
		const X* px = &x[0];
		const X* py = &y[0];
		CYBOZU_TEST_EQUAL(x.size(), 10);
		CYBOZU_TEST_EQUAL(y.size(), 5);
		for (int i = 0; i < 10; i++) {
			CYBOZU_TEST_EQUAL(x[i].v, i);
		}
		for (int i = 0; i < 5; i++) {
			CYBOZU_TEST_EQUAL(y[i].v, 10 + i);
		}
		cybozu::AlignedArray<X> z(std::move(x));
		const X* pz = &z[0];
		CYBOZU_TEST_EQUAL(pz, px);
		CYBOZU_TEST_EQUAL(x.data(), (X*)0);
		CYBOZU_TEST_EQUAL(z.data(), px);
		CYBOZU_TEST_EQUAL(z.size(), 10);
		for (int i = 0; i < 10; i++) {
			CYBOZU_TEST_EQUAL(z[i].v, i);
		}
		CYBOZU_TEST_EQUAL(x.size(), 0);

		z = std::move(y);
		CYBOZU_TEST_EQUAL(py, &z[0]);
		CYBOZU_TEST_EQUAL(py, z.data());
		CYBOZU_TEST_EQUAL(y.size(), 0);
		CYBOZU_TEST_EQUAL(z.size(), 5);
		for (int i = 0; i < 5; i++) {
			CYBOZU_TEST_EQUAL(z[i].v, 10 + i);
		}
	}
	CYBOZU_TEST_EQUAL(g_x, 0);
}
#endif
