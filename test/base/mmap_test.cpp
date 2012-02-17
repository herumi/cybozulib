#include <cybozu/test.hpp>
#include <cybozu/mmap.hpp>
#include <cybozu/file.hpp>
#include <string>

CYBOZU_TEST_AUTO(mmap)
{
	const std::string path = cybozu::file::GetExePath() + "../include/cybozu/mmap.hpp";
	cybozu::Mmap mmap(path);
	const std::string str(mmap.get(), (size_t)mmap.size());
	CYBOZU_TEST_EQUAL(str.find("#pragma once"), 0U);
}
