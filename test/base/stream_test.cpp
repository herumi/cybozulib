#include <cybozu/test.hpp>
#include <cybozu/stream.hpp>
#include <sstream>

CYBOZU_TEST_AUTO(sstream)
{
	std::stringstream ss;
	const std::string str = "123";
	cybozu::OutputStreamTag<std::stringstream>::write(ss, str.c_str(), str.size());
	CYBOZU_TEST_EQUAL(ss.str(), str);
	char buf[64] = {};
	cybozu::InputStreamTag<std::stringstream>::read(ss, buf, str.size());
	CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
}

CYBOZU_TEST_AUTO(memory)
{
	const char in[] = "this is a pen";
	const size_t inSize = strlen(in);
	cybozu::MemoryInputStream is(in, inSize);
	char buf[64] = {};
	cybozu::InputStreamTag<cybozu::MemoryInputStream>::read(is, buf, inSize);
	CYBOZU_TEST_ASSERT(memcmp(buf, in, inSize) == 0);
}
