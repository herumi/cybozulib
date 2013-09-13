#include <cybozu/test.hpp>
#include <cybozu/stream.hpp>
#include <sstream>

CYBOZU_TEST_AUTO(sstream)
{
	const std::string str = "123";
	{
		std::ostringstream os;
		cybozu::OutputStreamTag<std::ostringstream>::write(os, str.c_str(), str.size());
		CYBOZU_TEST_EQUAL(os.str(), str);
	}
	{
		char buf[64] = {};
		std::istringstream is(str);
		size_t readSize = cybozu::InputStreamTag<std::istringstream>::readSome(is, buf, sizeof(buf));
		CYBOZU_TEST_EQUAL(str.size(), readSize);
		CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
	}
	{
		char buf[64] = {};
		std::istringstream is(str);
		cybozu::stream::read(is, buf, str.size());
		CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
	}
}

CYBOZU_TEST_AUTO(memory)
{
	const std::string str = "123";
	{
		char buf[64] = {};
		cybozu::MemoryOutputStream os(buf, sizeof(buf));
		cybozu::OutputStreamTag<cybozu::MemoryOutputStream>::write(os, str.c_str(), str.size());
		CYBOZU_TEST_EQUAL(str.size(), os.pos);
		CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
	}
	{
		cybozu::MemoryInputStream is(str.c_str(), str.size());
		char buf[64] = {};
		size_t readSize = cybozu::InputStreamTag<cybozu::MemoryInputStream>::readSome(is, buf, sizeof(buf));
		CYBOZU_TEST_EQUAL(str.size(), readSize);
		CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
	}
	{
		cybozu::MemoryInputStream is(str.c_str(), str.size());
		char buf[64] = {};
		cybozu::stream::read(is, buf, str.size());
		CYBOZU_TEST_EQUAL(str.size(), is.pos);
		CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
	}
}

CYBOZU_TEST_AUTO(string)
{
	const std::string str = "123";
	{
		std::string s;
		cybozu::StringOutputStream os(s);
		cybozu::OutputStreamTag<cybozu::StringOutputStream>::write(os, str.c_str(), str.size());
		CYBOZU_TEST_EQUAL(str, s);
	}
	{
		cybozu::StringInputStream is(str);
		char buf[64] = {};
		size_t readSize = cybozu::InputStreamTag<cybozu::StringInputStream>::readSome(is, buf, sizeof(buf));
		CYBOZU_TEST_EQUAL(str.size(), readSize);
		CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
	}
	{
		cybozu::StringInputStream is(str);
		char buf[64] = {};
		cybozu::stream::read(is, buf, str.size());
		CYBOZU_TEST_EQUAL(str.size(), is.pos);
		CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
	}
}
