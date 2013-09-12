#include <cybozu/test.hpp>
#include <cybozu/thread.hpp>
#include <cybozu/socket.hpp>

CYBOZU_TEST_AUTO(open_move)
{
	cybozu::Socket s;
	CYBOZU_TEST_ASSERT(!s.isValid());
	s.bind(65000);
	CYBOZU_TEST_ASSERT(s.isValid());
#if CYBOZU_CPP_VERSION == CYBOZU_CPP_VERSION_CPP11
	cybozu::Socket t(std::move(s));
#else
	cybozu::Socket t(s);
#endif
	CYBOZU_TEST_ASSERT(!s.isValid());
	CYBOZU_TEST_ASSERT(t.isValid());
#if CYBOZU_CPP_VERSION == CYBOZU_CPP_VERSION_CPP11
	cybozu::Socket u(std::move(t));
#else
	cybozu::Socket u(t);
#endif
	CYBOZU_TEST_ASSERT(!t.isValid());
	CYBOZU_TEST_ASSERT(u.isValid());
}
