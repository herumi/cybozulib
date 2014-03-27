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
	cybozu::Socket t;
	t.moveFrom(s);
#endif
	CYBOZU_TEST_ASSERT(!s.isValid());
	CYBOZU_TEST_ASSERT(t.isValid());
#if CYBOZU_CPP_VERSION == CYBOZU_CPP_VERSION_CPP11
	cybozu::Socket u(std::move(t));
#else
	cybozu::Socket u;
	u.moveFrom(t);
#endif
	CYBOZU_TEST_ASSERT(!t.isValid());
	CYBOZU_TEST_ASSERT(u.isValid());

	cybozu::Socket v;
	CYBOZU_TEST_ASSERT(!v.isValid());
	v.moveFrom(u);
	CYBOZU_TEST_ASSERT(v.isValid());
	CYBOZU_TEST_ASSERT(!u.isValid());
}

CYBOZU_TEST_AUTO(timeout)
{
	cybozu::Socket s;
	s.bind(10000);
	s.setSendTimeout(12000);
	CYBOZU_TEST_EQUAL(s.getSendTimeout(), 12000);
	s.setReceiveTimeout(54000);
	CYBOZU_TEST_EQUAL(s.getReceiveTimeout(), 54000);
}
