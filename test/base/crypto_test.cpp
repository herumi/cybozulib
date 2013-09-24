#include <cybozu/test.hpp>
#include <cybozu/crypto.hpp>

CYBOZU_TEST_AUTO(hashName)
{
	const char *tbl[] = {
		"sha1", "sha224", "sha256", "sha384", "sha512"
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		cybozu::crypto::Hash::Name name = cybozu::crypto::Hash::getName(tbl[i]);
		CYBOZU_TEST_EQUAL(tbl[i], cybozu::crypto::Hash::getName(name));
	}
	CYBOZU_TEST_EXCEPTION(cybozu::crypto::Hash::getName("sha123"), cybozu::Exception);
}

