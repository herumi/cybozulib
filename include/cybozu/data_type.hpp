#pragma once
/**
	@file
	@brief typedef of some STL data type

	Copyright (C) 2012 Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include <map>
#include <vector>

namespace cybozu {

typedef std::vector<int> IntVec;
typedef std::vector<std::string> StrVec;
typedef std::map<int, int> Int2Int;
typedef std::map<int, std::string> Int2Str;
typedef std::map<std::string, int> Str2Int;
typedef std::map<std::string, std::string> Str2Str;

} // cybozu
