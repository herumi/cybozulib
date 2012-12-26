#include <cybozu/format.hpp>
#include <cybozu/test.hpp>

CYBOZU_TEST_AUTO(format)
{
	std::string a;
	a = cybozu::format("");
	CYBOZU_TEST_EQUAL(a, std::string(""));
	a = cybozu::format("%d", 123);
	CYBOZU_TEST_EQUAL(a, std::string("123"));
	a = cybozu::format("%s%c%d%s", "abcdefg", 0, 123, "xxx");
	CYBOZU_TEST_EQUAL(a, std::string("abcdefg" "\x00" "123xxx", 14));
}
