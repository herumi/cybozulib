#include <string>
#include <stdint.h>
#include <stdio.h>
#include <cybozu/string.hpp>
#include <cybozu/regex.hpp>
#include <cybozu/test.hpp>

#ifdef _MSC_VER
	#define STR(x) L##x
	#define CHAR wchar_t
#else
	#define STR(x) x
	#define CHAR char
#endif

CYBOZU_TEST_AUTO(regex)
{
//	const cybozu::Char str_[] = { 0x3053, 0x308c, 0x306f, 'U', 'T', 'F', '-', '8', 0 };
	const CHAR str_[] = STR("ゔこんにちはUTF-8です");
	for (int i = 0; i < CYBOZU_NUM_OF_ARRAY(str_); i++) {
		printf("%04x ", (uint16_t)str_[i]);
	}
	printf("\n");

	cybozu::regex r("\\w+");
	cybozu::String str(str_);
	cybozu::smatch m;
	bool b = cybozu::regex_search(str, m, r);
	CYBOZU_TEST_ASSERT(b);
	if (b) {
		CYBOZU_TEST_EQUAL(m.str(), "UTF");
	}
}
