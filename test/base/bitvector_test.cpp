#include <cybozu/test.hpp>
#include <cybozu/bitvector.hpp>
#include <cybozu/xorshift.hpp>

CYBOZU_TEST_AUTO(set)
{
	cybozu::BitVector cv;
	std::vector<bool> sv;
	cybozu::XorShift rg;
	const size_t size = 100;
	cv.resize(size);
	sv.resize(size);
	for (int j = 0; j < 2; j++) {
		for (size_t i = 0; i < size; i++) {
			bool b = (rg.get32() & 1) != 0;
			cv.set(i, b);
			sv[i] = b;
		}
		for (size_t i = 0; i < size; i++) {
			CYBOZU_TEST_EQUAL(cv.get(i), sv[i]);
		}
	}
}
