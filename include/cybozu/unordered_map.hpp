#pragma once

#if (__cplusplus >= 201103) || (_MSC_VER >= 1500) || defined(__GXX_EXPERIMENTAL_CXX0X__)
	#include <unordered_map>
	#if defined(_MSC_VER) && (_MSC_VER < 1600)
		#define CYBOZU_UNORDERED_MAP_STD std::tr1
	#else
		#define CYBOZU_UNORDERED_MAP_STD std
	#endif
#elif (__GNUC__ >= 4 && __GNUC_MINOR__ >= 5) || (__clang_major__ >= 3)
	#include <tr1/unordered_map>
	#define CYBOZU_UNORDERED_MAP_STD std::tr1
#endif
