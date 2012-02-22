#include <cybozu/quick_search.hpp>
#include <cybozu/test.hpp>
#include <stdio.h>

CYBOZU_TEST_AUTO(search)
{
	const struct {
		const char *text;
		const char *key;
	} tbl [] = {
		{ "abcdefghijklmnopqrstu", "lmn" },
		{ "abc", "x" },
		{ "abc", "xy" },
		{ "abc", "xyz" },
		{ "abc", "xyzw" },
		{ "abc", "a" },
		{ "abc", "ab" },
		{ "abc", "abc" },
		{ "abc", "abcd" },
		{ "abcdefg", "cdg" },
		{ "abcabcabcabcd", "abcd" },
		{ "axabxabcxabcd", "abcd" },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const char *text = tbl[i].text;
		const char *key = tbl[i].key;
		const char *p = strstr(text, key);
		cybozu::QuickSearch qs(key);
		const char *q = qs.find(text, text + strlen(text));
		CYBOZU_TEST_EQUAL_POINTER(p, q);
	}
}
