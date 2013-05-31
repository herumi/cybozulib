#include <cybozu/test.hpp>
#include <cybozu/stream.hpp>
#include <sstream>

CYBOZU_TEST_AUTO(sstream)
{
	std::stringstream ss;
	const std::string str = "123";
	cybozu::OutputStreamTag<std::stringstream>::write(ss, str.c_str(), str.size());
	CYBOZU_TEST_EQUAL(ss.str(), str);
	char buf[64];
	size_t ret = cybozu::InputStreamTag<std::stringstream>::read(ss, buf, sizeof(buf));
	CYBOZU_TEST_EQUAL(ret, str.size());
	CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
}

CYBOZU_TEST_AUTO(memory)
{
	const char in[] = "this is a pen";
	const size_t inSize = strlen(in);
	cybozu::MemoryInputStream is(in, inSize);
	char buf[64];
	size_t ret = cybozu::InputStreamTag<cybozu::MemoryInputStream>::read(is, buf, sizeof(buf));
	CYBOZU_TEST_EQUAL(ret, inSize);
	CYBOZU_TEST_ASSERT(memcmp(buf, in, inSize) == 0);
}
