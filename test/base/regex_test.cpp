#include <string>
#include <stdint.h>
#include <stdio.h>
#include <cybozu/test.hpp>
#if 0
#include <regex>

CYBOZU_TEST_AUTO(regex)
{
	const char str_[] = "01234UTF999";
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(str_); i++) {
		printf("%02x ", (uint8_t)str_[i]);
	}
	printf("\n");

	std::regex r("[a-z]+");
	std::string str(str_);
	std::smatch m;
	bool b = std::regex_search(str, m, r);
	CYBOZU_TEST_ASSERT(b);
	if (b) {
		CYBOZU_TEST_EQUAL(m.str(), "UTF");
	}
}

#else
#include <cybozu/regex.hpp>

CYBOZU_TEST_AUTO(regex)
{
//	const cybozu::Char str_[] = { 0x3053, 0x308c, 0x306f, 'U', 'T', 'F', '-', '8', 0 };
	const CYBOZU_RE_CHAR str_[] = CYBOZU_RE("ゔ こんにちはUTF-8です");
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(str_); i++) {
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

CYBOZU_TEST_AUTO(regex_token_iterator)
{
	cybozu::String data(CYBOZU_RE("あいうえおあああかきいあいくけこいいう"));
	cybozu::regex r(CYBOZU_RE("[あ-う]+"));
	cybozu::sregex_token_iterator i(data.begin(), data.end(), r);
	cybozu::sregex_token_iterator iend;
	while (i != iend) {
//		cybozu::String s = *i++;
		cybozu::String s(*i++);
		std::cout << s << std::endl;
	}
}

#endif
