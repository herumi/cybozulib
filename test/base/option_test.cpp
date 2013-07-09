#include <cybozu/option.hpp>
#include <cybozu/test.hpp>

CYBOZU_TEST_AUTO(convert_int)
{
	struct {
		const char *str;
		int val;
		bool valid;
	} tbl[] = {
		{ "123", 123, true },
		{ "-52145", -52145, true },
		{ "2147483647", 2147483647, true },
		{ "-2147483648", -2147483647 - 1, true },
		{ "2147483648", 0, false },
		{ "-2147483649", 0, false },
		{ "1k", 1000, true },
		{ "-1k", -1000, true },
		{ "2K", 2048, true },
		{ "1m", 1000000, true },
		{ "1M", 1024 * 1024, true },
		{ "1g", 1000000000, true },
		{ "1G", 1024 * 1024 * 1024, true },
		{ "2g", 2000000000, true },
		{ "2G", 0, false },
		{ "-2G", -1024 * 1024 * 1024 * 2, true },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		int x;
		bool b = cybozu::option_local::convertInt(&x, tbl[i].str);
		CYBOZU_TEST_EQUAL(b, tbl[i].valid);
		if (b) {
			CYBOZU_TEST_EQUAL(x, tbl[i].val);
		}
	}
}

CYBOZU_TEST_AUTO(convert_uint)
{
	struct {
		const char *str;
		uint32_t val;
		bool valid;
	} tbl[] = {
		{ "123", 123, true },
		{ "2147483647", 2147483647, true },
		{ "2147483648", 2147483648U, true },
		{ "4294967295", 4294967295U, true },
		{ "4294967296", 0, false },
		{ "-5", 0, false },
		{ "1m", 1000000, true },
		{ "1M", 1024 * 1024, true },
		{ "1g", 1000000000, true },
		{ "1G", 1024 * 1024 * 1024, true },
		{ "2g", 2000000000, true },
		{ "2G", 1024U * 1024 * 1024 * 2, true },
		{ "3G", 1024U * 1024 * 1024 * 3, true },
		{ "4G", 0, false },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		uint32_t x;
		bool b = cybozu::option_local::convertInt(&x, tbl[i].str);
		CYBOZU_TEST_EQUAL(b, tbl[i].valid);
		if (b) {
			CYBOZU_TEST_EQUAL(x, tbl[i].val);
		}
	}
}

CYBOZU_TEST_AUTO(convert_int64_t)
{
	struct {
		const char *str;
		int64_t val;
		bool valid;
	} tbl[] = {
		{ "123", 123, true },
		{ "-52145", -52145, true },
		{ "2147483647", 2147483647, true },
		{ "-2147483648", -2147483647 - 1, true },
		{ "9223372036854775807", int64_t(9223372036854775807LL), true },
		{ "9223372036854775808", 0, false },
		{ "1k", 1000, true },
		{ "-1k", -1000, true },
		{ "2K", 2048, true },
		{ "1m", 1000000, true },
		{ "1M", 1024 * 1024, true },
		{ "1g", 1000000000, true },
		{ "1G", 1024 * 1024 * 1024, true },
		{ "-2G", -1024 * 1024 * 1024 * 2, true },
		{ "8589934591G", int64_t(9223372035781033984LL), true },
		{ "8589934592G", 0, false },
		{ "-8589934592G", int64_t(-9223372036854775807LL - 1), true },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		int64_t x;
		bool b = cybozu::option_local::convertInt(&x, tbl[i].str);
		CYBOZU_TEST_EQUAL(b, tbl[i].valid);
		if (b) {
			CYBOZU_TEST_EQUAL(x, tbl[i].val);
		}
	}
}

CYBOZU_TEST_AUTO(convert_uint64_t)
{
	struct {
		const char *str;
		uint64_t val;
		bool valid;
	} tbl[] = {
		{ "123", 123, true },
		{ "-52145", 0, false },
		{ "18446744073709551615", uint64_t(18446744073709551615ULL), true },
		{ "18446744073709551616", 0, false },
		{ "1k", 1000, true },
		{ "2K", 2048, true },
		{ "1m", 1000000, true },
		{ "1M", 1024 * 1024, true },
		{ "1g", 1000000000, true },
		{ "1G", 1024 * 1024 * 1024, true },
		{ "17179869183G", uint64_t(18446744072635809792ULL), true },
		{ "17179869184G", 0, false },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		uint64_t x;
		bool b = cybozu::option_local::convertInt(&x, tbl[i].str);
		CYBOZU_TEST_EQUAL(b, tbl[i].valid);
		if (b) {
			CYBOZU_TEST_EQUAL(x, tbl[i].val);
		}
	}
}

CYBOZU_TEST_AUTO(Var)
{
	{
		int x = 3;
		cybozu::option_local::Var v(&x);
		CYBOZU_TEST_ASSERT(!v.isSet());
		v.set("12345");
		CYBOZU_TEST_EQUAL(x, 12345);
		CYBOZU_TEST_EQUAL(v.toStr(), "12345");
		CYBOZU_TEST_ASSERT(v.isSet());
		v.set("333");
		CYBOZU_TEST_EQUAL(x, 333);
		cybozu::option_local::Var w;
		CYBOZU_TEST_ASSERT(!w.isSet());
		CYBOZU_TEST_EQUAL(w.toStr(), "");
		w.swap(v);
		CYBOZU_TEST_ASSERT(w.isSet());
		CYBOZU_TEST_EQUAL(w.toStr(), "333");
		CYBOZU_TEST_ASSERT(!v.isSet());
		CYBOZU_TEST_EQUAL(v.toStr(), "");
	}
	{
		double x = 2;
		cybozu::option_local::Var v(&x);
		CYBOZU_TEST_ASSERT(!v.isSet());
		v.set("2.345");
		CYBOZU_TEST_EQUAL(x, 2.345);
		CYBOZU_TEST_EQUAL(v.toStr(), "2.345");
		CYBOZU_TEST_ASSERT(v.isSet());
	}
	{
		std::string x = "123";
		cybozu::option_local::Var v(&x);
		CYBOZU_TEST_ASSERT(!v.isSet());
		v.set("2.345");
		CYBOZU_TEST_EQUAL(x, "2.345");
		CYBOZU_TEST_EQUAL(v.toStr(), "2.345");
		CYBOZU_TEST_ASSERT(v.isSet());
	}
	{
		std::vector<int> x;
		cybozu::option_local::Var v(&x);
		CYBOZU_TEST_ASSERT(!v.isSet());
		v.set("5");
		v.set("1234");
		CYBOZU_TEST_EQUAL(x.size(), 2u);
		CYBOZU_TEST_EQUAL(x[0], 5);
		CYBOZU_TEST_EQUAL(x[1], 1234);
		CYBOZU_TEST_EQUAL(v.toStr(), "5 1234");
		CYBOZU_TEST_ASSERT(v.isSet());
	}
}
