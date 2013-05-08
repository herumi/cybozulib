#include <cybozu/test.hpp>
#include <cybozu/frequency.hpp>

CYBOZU_TEST_AUTO(char)
{
	std::string a = "abcaaab\x80\x80\xff\xff\xff";
	typedef cybozu::freq_local::FrequencyVec<char> Freq;
	Freq freq(a.begin(), a.end());
	const struct {
		char c;
		size_t n;
	} tbl[] = {
		{ 'a', 4 },
		{ '\xff', 3 },
		{ '\x80', 2 },
		{ 'b', 2 },
		{ 'c', 1 },
	};
	CYBOZU_TEST_EQUAL(freq.size(), 5u);
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		CYBOZU_TEST_EQUAL(freq.getFreq(tbl[i].c), tbl[i].n);
		CYBOZU_TEST_EQUAL(freq.getIdx(tbl[i].c), i);
		CYBOZU_TEST_EQUAL(freq.getElem(i), tbl[i].c);
	}
}

CYBOZU_TEST_AUTO(int)
{
	const int inTbl[] = {
		3, 5, 2, 3, 5, 2, 9, 5, 2, 2, 0,
	};
	typedef cybozu::Frequency<int> Freq;
	Freq freq(inTbl, inTbl + CYBOZU_NUM_OF_ARRAY(inTbl));
	const struct {
		int  c;
		size_t n;
	} tbl[] = {
		{ 2, 4 },
		{ 5, 3 },
		{ 3, 2 },
		{ 9, 1 },
		{ 0, 1 },
	};
	CYBOZU_TEST_EQUAL(freq.size(), 5u);
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		CYBOZU_TEST_EQUAL(freq.getFreq(tbl[i].c), tbl[i].n);
		CYBOZU_TEST_EQUAL(freq.getIdx(tbl[i].c), i);
		CYBOZU_TEST_EQUAL(freq.getElem(i), tbl[i].c);
	}
}

CYBOZU_TEST_AUTO(string)
{
	const std::string inTbl[] = {
		"abc", "123", "a", "abc", "abc", "x", "123", "x",
	};
	typedef cybozu::Frequency<std::string> Freq;
	Freq freq(inTbl, inTbl + CYBOZU_NUM_OF_ARRAY(inTbl));
	const struct {
		std::string c;
		size_t n;
	} tbl[] = {
		{ "abc", 3 },
		{ "x", 2 },
		{ "123", 2 },
		{ "a", 1 },
	};
	CYBOZU_TEST_EQUAL(freq.size(), 4u);
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		CYBOZU_TEST_EQUAL(freq.getFreq(tbl[i].c), tbl[i].n);
		CYBOZU_TEST_EQUAL(freq.getIdx(tbl[i].c), i);
		CYBOZU_TEST_EQUAL(freq.getElem(i), tbl[i].c);
	}
}
