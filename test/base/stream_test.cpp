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
		CYBOZU_TEST_ASSERT(cybozu::InputStreamTag<std::istringstream>::hasNext(is));
		size_t readSize = cybozu::InputStreamTag<std::istringstream>::readSome(is, buf, sizeof(buf));
		CYBOZU_TEST_ASSERT(!cybozu::InputStreamTag<std::istringstream>::hasNext(is));
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
	typedef cybozu::InputStreamTag<cybozu::MemoryInputStream> InputStream;
	const std::string str = "123";
	{
		char buf[64] = {};
		cybozu::MemoryOutputStream os(buf, sizeof(buf));
		cybozu::OutputStreamTag<cybozu::MemoryOutputStream>::write(os, str.c_str(), str.size());
		CYBOZU_TEST_EQUAL(str.size(), os.getPos());
		CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
	}
	{
		cybozu::MemoryInputStream is(str.c_str(), str.size());
		char buf[64] = {};
		CYBOZU_TEST_ASSERT(is.hasNext());
		size_t readSize = InputStream::readSome(is, buf, sizeof(buf));
		CYBOZU_TEST_ASSERT(!is.hasNext());
		CYBOZU_TEST_EQUAL(str.size(), readSize);
		CYBOZU_TEST_EQUAL(is.getPos(), readSize);
		CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
	}
	{
		cybozu::MemoryInputStream is(str.c_str(), str.size());
		char buf[64] = {};
		cybozu::stream::read(is, buf, str.size());
		CYBOZU_TEST_EQUAL(str.size(), is.getPos());
		CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
	}
	{
		cybozu::MemoryInputStream is(str.c_str(), str.size());
		CYBOZU_TEST_EQUAL(InputStream::readChar(is), '1');
		CYBOZU_TEST_EQUAL(InputStream::readChar(is), '2');
		CYBOZU_TEST_ASSERT(InputStream::hasNext(is));
		CYBOZU_TEST_EQUAL(InputStream::readChar(is), '3');
		CYBOZU_TEST_ASSERT(!InputStream::hasNext(is));
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
		CYBOZU_TEST_ASSERT(is.hasNext());
		char buf[64] = {};
		size_t readSize = cybozu::InputStreamTag<cybozu::StringInputStream>::readSome(is, buf, sizeof(buf));
		CYBOZU_TEST_ASSERT(!is.hasNext());
		CYBOZU_TEST_EQUAL(str.size(), readSize);
		CYBOZU_TEST_EQUAL(is.getPos(), readSize);
		CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
	}
	{
		cybozu::StringInputStream is(str);
		char buf[64] = {};
		cybozu::stream::read(is, buf, str.size());
		CYBOZU_TEST_EQUAL(str.size(), is.getPos());
		CYBOZU_TEST_ASSERT(memcmp(buf, str.c_str(), str.size()) == 0);
	}
}
