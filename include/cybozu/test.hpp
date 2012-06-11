#pragma once
/**
	@file
	@brief unit test class

	Copyright (C) 2008 Cybozu Labs, Inc., all rights reserved.
*/

#include <stdio.h>
#include <string.h>
#include <string>
#include <list>
#include <iostream>
#include <utility>
#include <cybozu/exception.hpp>

namespace cybozu { namespace test {

class AutoRun {
	typedef void (*Func)();
	typedef std::list<std::pair<const char*, Func> > UnitTestList;
public:
	AutoRun()
		: init_(0)
		, term_(0)
		, okCount_(0)
		, ngCount_(0)
		, exceptionCount_(0)
	{
	}
	void setup(Func init, Func term)
	{
		init_ = init;
		term_ = term;
	}
	void append(const char *name, Func func)
	{
		list_.push_back(std::make_pair(name, func));
	}
	void set(bool isOK)
	{
		if (isOK) {
			okCount_++;
		} else {
			ngCount_++;
		}
	}
	std::string getBaseName(const std::string& name) const
	{
#ifdef _WIN32
		const char sep = '\\';
#else
		const char sep = '/';
#endif
		size_t pos = name.find_last_of(sep);
		std::string ret = name.substr(pos + 1);
		pos = ret.find('.');
		return ret.substr(0, pos);
	}
	int run(int, char *argv[])
	{
		std::string msg;
		try {
			if (init_) init_();
			for (UnitTestList::const_iterator i = list_.begin(), ie = list_.end(); i != ie; ++i) {
				std::cout << "ctest:module=" << i->first << std::endl;
				try {
					(i->second)();
				} catch (cybozu::Exception& e) {
					exceptionCount_++;
					std::cout << "ctest:  " << i->first << " is stopped by cybozu::Exception " << e.toString() << std::endl;
				} catch (std::exception& e) {
					exceptionCount_++;
					std::cout << "ctest:  " << i->first << " is stopped by std::exception " << e.what() << std::endl;
				} catch (...) {
					exceptionCount_++;
					std::cout << "ctest:  " << i->first << " is stopped by an exception" << std::endl;
				}
			}
			if (term_) term_();
		} catch (cybozu::Exception& e) {
			msg = "ctest:err:" + e.toString();
		} catch (...) {
			msg = "ctest:err: catch unexpected exception";
		}
		fflush(stdout);
		if (msg.empty()) {
			std::cout << "ctest:name=" << getBaseName(*argv)
					  << ", module=" << list_.size()
					  << ", total=" << (okCount_ + ngCount_ + exceptionCount_)
					  << ", ok=" << okCount_
					  << ", ng=" << ngCount_
					  << ", exception=" << exceptionCount_ << std::endl;
			return 0;
		} else {
			std::cout << msg << std::endl;
			return 1;
		}
	}
private:
	Func init_;
	Func term_;
	int okCount_;
	int ngCount_;
	int exceptionCount_;
	UnitTestList list_;
};

AutoRun autoRun;

void test(bool ret, const std::string& msg, const char *param, const char *file, int line)
{
	autoRun.set(ret);
	if (!ret) {
		std::cout << file << "(" << line << "):" << "ctest:" << msg << "(" << param << ");" << std::endl;
	}
}

template<typename T, typename U>
bool isEqual(const T& lhs, const U& rhs)
{
	return lhs == rhs;
}

bool isEqual(const char *lhs, const char *rhs)
{
	return strcmp(lhs, rhs) == 0;
}
bool isEqual(char *lhs, const char *rhs)
{
	return strcmp(lhs, rhs) == 0;
}
bool isEqual(const char *lhs, char *rhs)
{
	return strcmp(lhs, rhs) == 0;
}
bool isEqual(char *lhs, char *rhs)
{
	return strcmp(lhs, rhs) == 0;
}
// avoid to compare float directly
bool isEqual(float lhs, float rhs)
{
	union fi {
		float f;
		uint32_t i;
	} lfi, rfi;
	lfi.f = lhs;
	rfi.f = rhs;
	return lfi.i == rfi.i;
}
// avoid to compare double directly
bool isEqual(double lhs, double rhs)
{
	union di {
		double d;
		uint64_t i;
	} ldi, rdi;
	ldi.d = lhs;
	rdi.d = rhs;
	return ldi.i == rdi.i;
}

} } // cybozu::test

#ifndef CYBOZU_TEST_DISABLE_AUTO_RUN
int main(int argc, char *argv[])
{
	return cybozu::test::autoRun.run(argc, argv);
}
#endif

/**
	alert if !x
	@param x [in]
*/
#define CYBOZU_TEST_ASSERT(x) cybozu::test::test(!!(x), "CYBOZU_TEST_ASSERT", #x, __FILE__, __LINE__)

/**
	alert if x != y
	@param x [in]
	@param y [in]
*/
#define CYBOZU_TEST_EQUAL(x, y) { \
	bool eq = cybozu::test::isEqual(x, y); \
	cybozu::test::test(eq, "CYBOZU_TEST_EQUAL", #x ", " #y, __FILE__, __LINE__); \
	if (!eq) { \
		std::cout << "ctest:  lhs=" << (x) << std::endl; \
		std::cout << "ctest:  rhs=" << (y) << std::endl; \
	} \
}
/**
	alert if fabs(x, y) >= eps
	@param x [in]
	@param y [in]
*/
#define CYBOZU_TEST_NEAR(x, y, eps) { \
	bool isNear = fabs((x) - (y)) < eps; \
	cybozu::test::test(isNear, "CYBOZU_TEST_NEAR", #x ", " #y, __FILE__, __LINE__); \
	if (!isNear) { \
		std::cout << "ctest:  lhs=" << (x) << std::endl; \
		std::cout << "ctest:  rhs=" << (y) << std::endl; \
	} \
}

#define CYBOZU_TEST_EQUAL_POINTER(x, y) { \
	bool eq = x == y; \
	cybozu::test::test(eq, "CYBOZU_TEST_EQUAL_POINTER", #x ", " #y, __FILE__, __LINE__); \
	if (!eq) { \
		std::cout << "ctest:  lhs=" << (const void*)(x) << std::endl; \
		std::cout << "ctest:  rhs=" << (const void*)(y) << std::endl; \
	} \
}

/**
	always alert
	@param msg [in]
*/
#define CYBOZU_TEST_FAIL(msg) cybozu::test::test(false, "CYBOZU_TEST_FAIL", msg, __FILE__, __LINE__)

/**
	verify message in exception
*/
#define CYBOZU_TEST_EXCEPTION_MESSAGE(statement, Exception, msg) \
{ \
	int ret = 0; \
	std::string errMsg; \
	try { \
		statement; \
		ret = 1; \
	} catch (const Exception& e) { \
		errMsg = e.toString(); \
		if (errMsg.find(msg) == std::string::npos) { \
			ret = 2; \
		} \
	} catch (...) { \
		ret = 3; \
	} \
	if (ret) { \
		cybozu::test::autoRun.set(false); \
		cybozu::test::test(false, "CYBOZU_TEST_EXCEPTION_MESSAGE", #statement ", " #Exception, __FILE__, __LINE__); \
		if (ret == 1) { \
			std::cout << "ctest:  no exception" << std::endl; \
		} else if (ret == 2) { \
			std::cout << "ctest:  bad exception msg:" << errMsg << std::endl; \
		} else { \
			std::cout << "ctest:  unexpected exception" << std::endl; \
		} \
	} else { \
		cybozu::test::autoRun.set(true); \
	} \
}

#define CYBOZU_TEST_EXCEPTION(statement, Exception) \
{ \
	int ret = 0; \
	try { \
		statement; \
		ret = 1; \
	} catch (const Exception&) { \
	} catch (...) { \
		ret = 2; \
	} \
	if (ret) { \
		cybozu::test::autoRun.set(false); \
		cybozu::test::test(false, "CYBOZU_TEST_EXCEPTION", #statement ", " #Exception, __FILE__, __LINE__); \
		if (ret == 1) { \
			std::cout << "ctest:  no exception" << std::endl; \
		} else { \
			std::cout << "ctest:  unexpected exception" << std::endl; \
		} \
	} else { \
		cybozu::test::autoRun.set(true); \
	} \
}

/**
	verify statement does not throw
*/
#define CYBOZU_TEST_NO_EXCEPTION(statement) \
try { \
	statement; \
	cybozu::test::autoRun.set(true); \
} catch (...) { \
	cybozu::test::test(false, "CYBOZU_TEST_NO_EXCEPTION", #statement, __FILE__, __LINE__); \
}

/**
	append auto unit test
	@param name [in] module name
*/
#define CYBOZU_TEST_AUTO(name) \
void cybozu_test_ ## name(); \
struct cybozu_test_local_ ## name { \
	cybozu_test_local_ ## name() \
	{ \
		cybozu::test::autoRun.append(#name, cybozu_test_ ## name); \
	} \
} cybozu_test_local_instance_ ## name; \
void cybozu_test_ ## name()

/**
	append auto unit test with fixture
	@param name [in] module name
*/
#define CYBOZU_TEST_AUTO_WITH_FIXTURE(name, Fixture) \
void cybozu_test_ ## name(); \
void cybozu_test_real_ ## name() \
{ \
	Fixture f; \
	cybozu_test_ ## name(); \
} \
struct cybozu_test_local_ ## name { \
	cybozu_test_local_ ## name() \
	{ \
		cybozu::test::autoRun.append(#name, cybozu_test_real_ ## name); \
	} \
} cybozu_test_local_instance_ ## name; \
void cybozu_test_ ## name()

/**
	setup fixture
	@param Fixture [in] class name of fixture
	@note cstr of Fixture is called before test and dstr of Fixture is called after test
*/
#define CYBOZU_TEST_SETUP_FIXTURE(Fixture) \
Fixture *cybozu_test_local_fixture; \
void cybozu_test_local_init() \
{ \
	cybozu_test_local_fixture = new Fixture(); \
} \
void cybozu_test_local_term() \
{ \
	delete cybozu_test_local_fixture; \
} \
struct cybozu_test_local_fixture_setup_ { \
	cybozu_test_local_fixture_setup_() \
	{ \
		cybozu::test::autoRun.setup(cybozu_test_local_init, cybozu_test_local_term); \
	} \
} cybozu_test_local_fixture_setup_instance_;
