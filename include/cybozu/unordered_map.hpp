#pragma once

#include <cybozu/inttype.hpp>

#if (CYBOZU_CPP_VERSION == CYBOZU_CPP_VERSION_CPP11) || (defined __APPLE__)
	#include <unordered_map>
#elif (CYBOZU_CPP_VERSION == CYBOZU_CPP_VERSION_TR1)
	#include <list>
	#include <tr1/unordered_map>
#endif

