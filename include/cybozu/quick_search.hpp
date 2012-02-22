#pragma once
/**
	@file
	@brief quick search algorithm

	Copyright (C) 2012 Cybozu Labs, Inc., all rights reserved.
	@author MITSUNARI Shigeo
*/
#include <algorithm>
#include <string.h>
#include <string>

namespace cybozu {

class QuickSearch {
	static const size_t SIZE = 256;
	size_t len_;
	int tbl_[SIZE];
	std::string str_;
	void init(const char *begin)
	{
		std::fill(tbl_, tbl_ + SIZE, static_cast<int>(len_ + 1));
		for (size_t i = 0; i < len_; i++) {
			tbl_[static_cast<unsigned char>(begin[i])] = len_ - i;
		}
	}
public:
	explicit QuickSearch(const char *begin, const char *end = 0)
		: len_(end ? end - begin : strlen(begin))
		, str_(begin, len_)
	{
		init(begin);
	}
	explicit QuickSearch(const std::string& key)
		: len_(key.size())
		, str_(key)
	{
		init(&key[0]);
	}
	const char *find(const char *begin, const char *end) const
	{
		while (begin <= end - len_) {
			for (size_t i = 0; i < len_; i++) {
				if (str_[i] != begin[i]) {
					goto NEXT;
				}
			}
			return begin;
		NEXT:
			begin += tbl_[static_cast<unsigned char>(begin[len_])];
		}
		return 0;
	}
};

} // cybozu
