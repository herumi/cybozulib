#include <stdio.h>
#include <cybozu/env.hpp>
#include <cybozu/test.hpp>

CYBOZU_TEST_AUTO(env)
{
	std::string val;
	CYBOZU_TEST_ASSERT(!cybozu::QueryEnv(val, "aw3vraw3vasv"));
#ifdef _WIN32
	const char *key = "SystemDrive";
	const char *assumeVal = "C:";
#elif defined(__APPLE__)
	const char *key = "OSTYPE";
	const char *assumeVal = "darwin";
#else
	const char *key = "OSTYPE";
	const char *assumeVal = "linux";
#endif
	val = cybozu::GetEnv(key);
	CYBOZU_TEST_EQUAL(val, assumeVal);
	CYBOZU_TEST_EQUAL(cybozu::GetEnv("asvrifansevjase", "default"), "default");
}
