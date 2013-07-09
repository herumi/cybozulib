#include <cybozu/option.hpp>
#include <cybozu/test.hpp>

char progName[] = "test";

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
		bool x = false;
		cybozu::option_local::Var v(&x);
		CYBOZU_TEST_ASSERT(!v.isSet());
		v.set("1");
		CYBOZU_TEST_ASSERT(x);
		CYBOZU_TEST_EQUAL(v.toStr(), "1");
		CYBOZU_TEST_ASSERT(v.isSet());
		v.set("0");
		CYBOZU_TEST_ASSERT(!x);
		CYBOZU_TEST_EQUAL(v.toStr(), "0");
	}
	{
		char x = 0;
		cybozu::option_local::Var v(&x);
		CYBOZU_TEST_ASSERT(!v.isSet());
		v.set("a");
		CYBOZU_TEST_EQUAL(x, 'a');
		CYBOZU_TEST_EQUAL(v.toStr(), "a");
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

CYBOZU_TEST_AUTO(appendOpt)
{
	cybozu::Option opt;
	int x;
	opt.appendOpt(&x, 3, "x");
	{
		char *argv[] = { progName };
		CYBOZU_TEST_ASSERT(opt.parse(1, argv));
		CYBOZU_TEST_EQUAL(x, 3);
	}
	{
		char optX[] = "-x";
		char i999[] = "999";
		char *argv[] = { progName, optX, i999 };
		CYBOZU_TEST_ASSERT(opt.parse(3, argv));
		CYBOZU_TEST_EQUAL(x, 999);
	}
}

CYBOZU_TEST_AUTO(appendMust)
{
	cybozu::Option opt;
	int x = 9;
	opt.appendMust(&x, "x");
	{
		char *argv[] = { progName };
		CYBOZU_TEST_ASSERT(!opt.parse(1, argv));
	}
	{
		char optX[] = "-x";
		char i999[] = "999";
		char *argv[] = { progName, optX, i999 };
		CYBOZU_TEST_ASSERT(opt.parse(3, argv));
		CYBOZU_TEST_EQUAL(x, 999);
	}
}

CYBOZU_TEST_AUTO(appendVec)
{
	cybozu::Option opt;
	std::vector<int> x;
	opt.appendVec(&x, "x");
	{
		char *argv[] = { progName };
		CYBOZU_TEST_ASSERT(opt.parse(1, argv));
		CYBOZU_TEST_ASSERT(x.empty());
	}
	{
		char optX[] = "-x";
		char i999[] = "999";
		char i123[] = "123";
		char *argv[] = { progName, optX, i999, i123 };
		CYBOZU_TEST_ASSERT(opt.parse(4, argv));
		CYBOZU_TEST_EQUAL(x.size(), 2u);
		CYBOZU_TEST_EQUAL(x[0], 999);
		CYBOZU_TEST_EQUAL(x[1], 123);
	}
}

CYBOZU_TEST_AUTO(appendParam)
{
	cybozu::Option opt;
	std::string x;
	opt.appendParam(&x, "x");
	{
		char *argv[] = { progName };
		CYBOZU_TEST_ASSERT(!opt.parse(1, argv));
	}
	{
		char aaa[] = "aaa";
		char *argv[] = { progName, aaa };
		CYBOZU_TEST_ASSERT(opt.parse(2, argv));
		CYBOZU_TEST_EQUAL(x, "aaa");
	}
}

CYBOZU_TEST_AUTO(appendParamOpt)
{
	cybozu::Option opt;
	std::string x;
	opt.appendParamOpt(&x, "x");
	{
		char *argv[] = { progName };
		CYBOZU_TEST_ASSERT(opt.parse(1, argv));
		CYBOZU_TEST_ASSERT(x.empty());
	}
	{
		char aaa[] = "aaa";
		char *argv[] = { progName, aaa };
		CYBOZU_TEST_ASSERT(opt.parse(2, argv));
		CYBOZU_TEST_EQUAL(x, "aaa");
	}
}

CYBOZU_TEST_AUTO(appendParamVec)
{
	cybozu::Option opt;
	std::vector<char> x;
	opt.appendParamVec(&x, "x");
	{
		char *argv[] = { progName };
		CYBOZU_TEST_ASSERT(opt.parse(1, argv));
		CYBOZU_TEST_ASSERT(x.empty());
	}
	{
		char a[] = "a";
		char z[] = "z";
		char *argv[] = { progName, a, z };
		CYBOZU_TEST_ASSERT(opt.parse(3, argv));
		CYBOZU_TEST_EQUAL(x.size(), 2u);
		CYBOZU_TEST_EQUAL(x[0], 'a');
		CYBOZU_TEST_EQUAL(x[1], 'z');
	}
}

CYBOZU_TEST_AUTO(dupplicate)
{
	cybozu::Option opt;
	int x;
	opt.appendMust(&x, "x");
	CYBOZU_TEST_EXCEPTION(opt.appendMust(&x, "x"), cybozu::Exception);
}

CYBOZU_TEST_AUTO(appendParamErr)
{
	cybozu::Option opt;
	int x;
	int y;
	std::vector<int> z;
	opt.appendParamOpt(&x, "x");
	CYBOZU_TEST_EXCEPTION(opt.appendParam(&y, "y"), cybozu::Exception);
	CYBOZU_TEST_EXCEPTION(opt.appendParamVec(&z, "z"), cybozu::Exception);
}

CYBOZU_TEST_AUTO(opt_begin_with_not_a_number)
{
	cybozu::Option opt;
	int x;
	// opt can't srat number
	CYBOZU_TEST_EXCEPTION(opt.appendOpt(&x, 9, "3a"), cybozu::Exception);
}
