#include <stdio.h>
#include <cybozu/serializer.hpp>
#include <cybozu/test.hpp>
#include <cybozu/stream.hpp>
//#include <boost/unordered_map.hpp>
#include <cybozu/stream.hpp>
#include <map>
#include <set>

CYBOZU_TEST_AUTO(int)
{
	const struct {
		int in;
		const char *out;
	} tbl[] = {
		{ 0, "0," },
		{ 123, "123," },
		{ -456, "-456," },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		std::string str;
		cybozu::RefStringOutputStream os(str);
		cybozu::serialize(os, tbl[i].in);
		CYBOZU_TEST_EQUAL(str, tbl[i].out);

		cybozu::MemoryInputStream is(str);
		int x;
		cybozu::deserialize(is, x);
		CYBOZU_TEST_EQUAL(x, tbl[i].in);
	}
}

CYBOZU_TEST_AUTO(uint)
{
	const struct {
		unsigned int in;
		const char *out;
	} tbl[] = {
		{ 0, "0," },
		{ 123, "123," },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		std::string str;
		cybozu::RefStringOutputStream os(str);
		cybozu::serialize(os, tbl[i].in);
		CYBOZU_TEST_EQUAL(str, tbl[i].out);

		cybozu::MemoryInputStream is(str);
		unsigned int x;
		cybozu::deserialize(is, x);
		CYBOZU_TEST_EQUAL(x, tbl[i].in);
	}
}

CYBOZU_TEST_AUTO(int64)
{
	const struct {
		int64_t in;
		const char *out;
	} tbl[] = {
		{ 0, "0," },
		{ -123, "-123," },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		std::string str;
		cybozu::RefStringOutputStream os(str);
		cybozu::serialize(os, tbl[i].in);
		CYBOZU_TEST_EQUAL(str, tbl[i].out);

		cybozu::MemoryInputStream is(str);
		int64_t x;
		cybozu::deserialize(is, x);
		CYBOZU_TEST_EQUAL(x, tbl[i].in);
	}
}

CYBOZU_TEST_AUTO(uint64)
{
	const struct {
		uint64_t in;
		const char *out;
	} tbl[] = {
		{ 0, "0," },
		{ 123, "123," },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		std::string str;
		cybozu::RefStringOutputStream os(str);
		cybozu::serialize(os, tbl[i].in);
		CYBOZU_TEST_EQUAL(str, tbl[i].out);

		cybozu::MemoryInputStream is(str);
		uint64_t x;
		cybozu::deserialize(is, x);
		CYBOZU_TEST_EQUAL(x, tbl[i].in);
	}
}

CYBOZU_TEST_AUTO(float)
{
	const struct {
		float in;
		const char *out;
	} tbl[] = {
		{ 0.0f, "00000000," },
		{ 1.0f, "3f800000," },
		{ 1.5f, "3fc00000," },
		{ -123.133003f, "c2f64419," },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		std::string str;
		cybozu::RefStringOutputStream os(str);
		cybozu::serialize(os, tbl[i].in);
		CYBOZU_TEST_EQUAL(str, tbl[i].out);

		cybozu::MemoryInputStream is(str);
		float x;
		cybozu::deserialize(is, x);
		CYBOZU_TEST_EQUAL(x, tbl[i].in);
	}
}

CYBOZU_TEST_AUTO(double)
{
	const struct {
		double in;
		const char *out;
	} tbl[] = {
		{ 0.0, "0000000000000000," },
		{ 1.0, "3ff0000000000000," },
		{ 1.5, "3ff8000000000000," },
		{ -123.133003, "c05ec8831f03d146," },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		std::string str;
		cybozu::RefStringOutputStream os(str);
		cybozu::serialize(os, tbl[i].in);
		CYBOZU_TEST_EQUAL(str, tbl[i].out);

		cybozu::MemoryInputStream is(str);
		double x;
		cybozu::deserialize(is, x);
		CYBOZU_TEST_EQUAL(x, tbl[i].in);
	}
}

CYBOZU_TEST_AUTO(serializeStr)
{
	const struct {
		const char *in;
		const char *out;
	} tbl[] = {
		{ "abc", "abc," },
		{ "abc\n\r,xxx", "abc\\n\\r\\,xxx," },
		{ "abc\n\r\\,xxx", "abc\\n\\r\\\\\\,xxx," },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		std::string out;
		cybozu::RefStringOutputStream os(out);

		cybozu::serialize(os, std::string(tbl[i].in));
		CYBOZU_TEST_EQUAL(out, tbl[i].out);
	}
}

CYBOZU_TEST_AUTO(deserializeStr)
{
	const struct {
		const char *in;
		const char *out;
	} tbl[] = {
		{ "abc,", "abc" },
		{ "abc,def", "abc" },
		{ "abc\\n\\r\\,xxx,", "abc\n\r,xxx" },
		{ "abc\\n\\r\\\\\\,xxx,", "abc\n\r\\,xxx" },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		std::string in = tbl[i].in;
		cybozu::MemoryInputStream is(in);

		std::string out;
		cybozu::deserialize(is, out);
		CYBOZU_TEST_EQUAL(out, tbl[i].out);
	}
}

template<typename V, typename T>
void Set(V& x, const T* in, size_t n)
{
	x.clear();
	for (size_t i = 0; i < n; i++) {
		x.push_back(in[i]);
	}
}

template<typename V>
void Put(const V& v)
{
	for (typename V::const_iterator i = v.begin(), ie = v.end(); i != ie; ++i) {
		std::cout << *i << ",";
	}
	std::cout << std::endl;
}

CYBOZU_TEST_AUTO(vectorInt)
{
	const struct {
		const char *in;
		size_t n;
		int v[5];
	} tbl[] = {
		{ "0,", 0, { 0 } },
		{ "1,1234567,", 1, { 1234567 } },
		{ "4,0,12,243,-344,", 4, { 0, 12, 243, -344 } },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		{
			std::string in = tbl[i].in;
			cybozu::MemoryInputStream is(in);

			std::vector<int> v, ok;
			cybozu::deserialize(is, v);
			Set(ok, tbl[i].v, tbl[i].n);
			CYBOZU_TEST_ASSERT(v == ok);
		}
		{
			std::string out;
			cybozu::RefStringOutputStream os(out);

			std::vector<int> v;
			Set(v, tbl[i].v, tbl[i].n);
			cybozu::serialize(os, v);
			CYBOZU_TEST_EQUAL(out, tbl[i].in);
		}
	}
}


CYBOZU_TEST_AUTO(vectorStr)
{
	const struct {
		const char *in;
		size_t n;
		const char *v[5];
	} tbl[] = {
		{ "0,", 0, { "" } },
		{ "1,1234567,", 1, { "1234567" } },
		{ "5,0,12,243,-344,abc,", 5, { "0", "12", "243", "-344", "abc" } },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		{
			std::string in = tbl[i].in;
			cybozu::MemoryInputStream is(in);

			std::vector<std::string> v, ok;
			cybozu::deserialize(is, v);
			Set(ok, tbl[i].v, tbl[i].n);
			CYBOZU_TEST_ASSERT(v == ok);
		}
		{
			std::string out;
			cybozu::RefStringOutputStream os(out);

			std::vector<std::string> v;
			Set(v, tbl[i].v, tbl[i].n);
			cybozu::serialize(os, v);
			CYBOZU_TEST_EQUAL(out, tbl[i].in);
		}
	}
}

CYBOZU_TEST_AUTO(vectorVectorStr)
{
	const std::string in = "3,0,1,1234567,5,abc,def,asdf,234,521,";

	const struct {
		size_t n;
		const char *v[5];
	} tbl[] = {
		{ 0, { "" } },
		{ 1, { "1234567" } },
		{ 5, { "abc", "def", "asdf", "234", "521" } },
	};
	std::vector<std::vector<std::string> > a, b;

	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		std::vector<std::string> v;
		Set(v, tbl[i].v, tbl[i].n);
		a.push_back(v);
	}

	std::string out;
	cybozu::RefStringOutputStream os(out);
	cybozu::serialize(os, a);

	CYBOZU_TEST_EQUAL(out, in);

	cybozu::MemoryInputStream is(in);
	cybozu::deserialize(is, b);
	CYBOZU_TEST_ASSERT(a == b);
}

CYBOZU_TEST_AUTO(str2double)
{
	std::map<std::string, double> x, y;
	x["asdfasd"] = 1.2;
	x["this"] = 3.141592;
	x["is"] = 1.2;
	x["a"] = -120;
	x["pen"] = 0;

	std::string out;
	cybozu::RefStringOutputStream os(out);
	cybozu::serialize(os, x);

	cybozu::MemoryInputStream is(out);
	cybozu::deserialize(is, y);

	CYBOZU_TEST_ASSERT(x == y);
}

#if 0
CYBOZU_TEST_AUTO(strHashDouble)
{
	boost::unordered_map<std::string, int> x, y;
	x["asdfasd"] = 12;
	x["this"] = 3141592;
	x["is"] = 999;
	x["a"] = -120;
	x["pen"] = 0;

	std::string out;
	cybozu::RefStringOutputStream os(out);
	cybozu::serialize(os, x);

	cybozu::MemoryInputStream is(out);
	cybozu::deserialize(is, y);

	CYBOZU_TEST_ASSERT(x == y);
}
#endif

CYBOZU_TEST_AUTO(list)
{
	std::list<std::string> x, y;
	x.push_back("asdfasd");
	x.push_back("absdf");
	x.push_back("hit");
	x.push_back("hello");

	std::string out;
	cybozu::RefStringOutputStream os(out);
	cybozu::serialize(os, x);

	cybozu::MemoryInputStream is(out);
	cybozu::deserialize(is, y);

	CYBOZU_TEST_ASSERT(x == y);
}

CYBOZU_TEST_AUTO(set)
{
	std::set<std::string> x, y;
	x.insert("asdfasd");
	x.insert("absdf");
	x.insert("hit");
	x.insert("hello");

	std::string out;
	cybozu::RefStringOutputStream os(out);
	cybozu::serialize(os, x);

	cybozu::MemoryInputStream is(out);
	cybozu::deserialize(is, y);

	CYBOZU_TEST_ASSERT(x == y);
}

CYBOZU_TEST_AUTO(mapMap)
{
	typedef std::map<std::string, double> Str2Double;
	std::map<std::string, Str2Double> x, y;

	Str2Double a, b, c;
	a["sdf"] = 10.2;
	a["this"] = -123.42;
	b["std"] = 0;
	b["map"] = 9998.1234;
	b["this this"] = 122.22;
	c["do\r\t\n"] = 9.33099;
	c[",,,"] = 333;

	x["123"] = a;
	x["##$$"] = b;
	x["4232\""] = c;

	std::string out;
	cybozu::RefStringOutputStream os(out);
	cybozu::serialize(os, x);
	cybozu::MemoryInputStream is(out);
	cybozu::deserialize(is, y);

	CYBOZU_TEST_ASSERT(x == y);
}

#if 0
#include <cybozu/stream_ext.hpp>
template<class X, class Y>
void compressTest(X& x, Y& y, bool useCompression)
{
	std::string out;
	{
		cybozu::RefStringOutputStream os(out);
		cybozu::CompressOutputStream<cybozu::RefStringOutputStream> enc(os, useCompression);

		cybozu::serialize(enc, x);
		// enc.flush(); call in destructor of enc
	}

	cybozu::MemoryInputStream is(out);
	cybozu::DecompressInputStream<cybozu::MemoryInputStream> dec(is, useCompression);
	cybozu::deserialize(dec, y);

	CYBOZU_TEST_ASSERT(x == y);
}

CYBOZU_TEST_AUTO(hashHash)
{
	typedef boost::unordered_map<std::string, double> Str2Double;
	boost::unordered_map<std::string, Str2Double> x, y;

	Str2Double a, b, c;
	a["sdf"] = 10.2;
	a["this"] = -123.42;
	b["std"] = 0;
	b["map"] = 9998.1234;
	b["this this"] = 122.22;
	c["do\r\t\n"] = 9.33099;
	c[",,,"] = 333;

	x["123"] = a;
	x["##$$"] = b;
	x["4232\""] = c;

	std::string out;
	cybozu::RefStringOutputStream os(out);
	cybozu::serialize(os, x);
	cybozu::MemoryInputStream is(out);
	cybozu::deserialize(is, y);

	CYBOZU_TEST_ASSERT(x == y);

//	compressTest(x, y, true);
//	compressTest(x, y, false);
}
#endif

CYBOZU_TEST_AUTO(bool)
{
	for (int i = 0; i < 2; i++) {
		std::string out;
		cybozu::RefStringOutputStream os(out);
		bool x = i == 0;
		cybozu::serialize(os, x);
		cybozu::MemoryInputStream is(out);
		bool y;
		cybozu::deserialize(is, y);
		CYBOZU_TEST_EQUAL(y, x);
	}
}

#ifdef _MSC_VER
#include <unordered_map>

CYBOZU_TEST_AUTO(tr1Hash)
{
	typedef std::tr1::unordered_map<std::string, double> Str2Double;
	typedef std::map<std::string, Str2Double> MapMap;
	MapMap x, y;

	Str2Double a, b, c;
	a["sdf"] = 10.2;
	a["this"] = -123.42;
	b["std"] = 0;
	b["map"] = 9998.1234;
	b["this this"] = 122.22;
	c["do\r\t\n"] = 9.33099;
	c[",,,"] = 333;

	x["123"] = a;
	x["##$$"] = b;
	x["4232\""] = c;

	std::string out;
	cybozu::RefStringOutputStream os(out);
	cybozu::serialize(os, x);
	cybozu::MemoryInputStream is(out);
	cybozu::deserialize(is, y);

	CYBOZU_TEST_EQUAL(x.size(), y.size());
	if (x.size() != y.size()) return;

	// CYBOZU_TEST_ASSERT(x == y);
	// compare each element because unordered_map does not keep the ordered so (x == y) may be false
	for (MapMap::const_iterator xi = x.begin(), xie = x.end(); xi != xie; ++xi) {
		MapMap::const_iterator yi = y.find(xi->first);
		CYBOZU_TEST_ASSERT(yi != y.end());
		if (yi == y.end()) return;
		CYBOZU_TEST_EQUAL(xi->first, yi->first);
		CYBOZU_TEST_EQUAL(xi->second.size(), yi->second.size());
		if (xi->second.size() != yi->second.size()) return;
		for (Str2Double::const_iterator xk = xi->second.begin(), xke = xi->second.end(); xk != xke; ++xk) {
			Str2Double::const_iterator yk = yi->second.find(xk->first);
			CYBOZU_TEST_ASSERT(yk != yi->second.end());
			if (yk == yi->second.end()) return;
			CYBOZU_TEST_EQUAL(xk->first, yk->first);
			CYBOZU_TEST_EQUAL(xk->second, yk->second);
		}
	}
}

#endif
