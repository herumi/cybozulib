#pragma once

#include <cybozu/inttype.hpp>

#if (CYBOZU_CPP_VERSION == CYBOZU_CPP_VERSION_CPP11)
	#include <unordered_set>
#elif (CYBOZU_CPP_VERSION == CYBOZU_CPP_VERSION_TR1)
	#include <list>
	#include <tr1/unordered_set>
#endif

