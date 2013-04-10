// don't remove BOM(EF BB BF) for VC
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <cybozu/test.hpp>
#include <cybozu/regex.hpp>

CYBOZU_TEST_AUTO(regex_search)
{
	cybozu::String str(CYBOZU_RE("こんにちはUTF-8です ゔ♡"));
	for (size_t i = 0; i < str.size(); i++) {
		printf("%04x ", str[i]);
	}
	printf("\n");

	cybozu::regex r("\\w+");
	cybozu::smatch m;
	bool b = cybozu::regex_search(str, m, r);
	CYBOZU_TEST_ASSERT(b);
	CYBOZU_TEST_EQUAL(m.str(), "UTF");
	CYBOZU_TEST_EQUAL(m.prefix(), cybozu::String(CYBOZU_RE("こんにちは")));
	CYBOZU_TEST_EQUAL(m.suffix(), cybozu::String(CYBOZU_RE("-8です ゔ♡")));
	CYBOZU_TEST_EQUAL(m.position(), 5u);
}

CYBOZU_TEST_AUTO(regex_token_iterator)
{
	cybozu::String data(CYBOZU_RE("あいうえおあああかきいあいくけこいいう"));
	cybozu::regex r(CYBOZU_RE("[あ-う]+"));
	cybozu::sregex_token_iterator i(data.begin(), data.end(), r);
	cybozu::sregex_token_iterator iend;

	const cybozu::String tbl[] = {
		CYBOZU_RE("あいう"),
		CYBOZU_RE("あああ"),
		CYBOZU_RE("いあい"),
		CYBOZU_RE("いいう"),
	};
	size_t pos = 0;
	while (i != iend) {
		cybozu::String s(*i++);
		CYBOZU_TEST_EQUAL(s, tbl[pos]);
		pos++;
	}
	CYBOZU_TEST_EQUAL(pos, CYBOZU_NUM_OF_ARRAY(tbl));
}

CYBOZU_TEST_AUTO(regex_replace)
{
	const cybozu::String s(CYBOZU_RE("サイボウズらららだ"));
	cybozu::regex r(CYBOZU_RE("ら+"));
	cybozu::String t = cybozu::regex_replace(s, r, CYBOZU_RE("live"));
	CYBOZU_TEST_EQUAL(t, CYBOZU_RE("サイボウズliveだ"));
}

CYBOZU_TEST_AUTO(regex_replace2)
{
	const cybozu::String s(CYBOZU_RE("cybozu Cybozu CYBOZU 坊主"));
	// icase is not supported now
	cybozu::regex r(CYBOZU_RE("cybozu|坊主") /*, std::regex_constatns::icase */);
	cybozu::String t = cybozu::regex_replace(s, r, CYBOZU_RE("サイボウズ"));
	std::cout << t << std::endl;
	CYBOZU_TEST_EQUAL(t, CYBOZU_RE("サイボウズ Cybozu CYBOZU サイボウズ"));
}