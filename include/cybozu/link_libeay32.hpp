#pragma once
/**
	@file
	@brief link libeay32.lib of openssl
	Copyright (C) 2016 Cybozu Labs, Inc., all rights reserved.
*/
#if defined(_WIN32) && defined(_MT)
	#if _MSC_VER >= 1900 // VC2015
		#pragma comment(lib, "mt/14/libeay32.lib")
//	#elif _MSC_VER == 1800 // VC2013
	#else
		#pragma comment(lib, "mt/12/libeay32.lib")
	#endif
	#pragma comment(lib, "advapi32.lib")
	#pragma comment(lib, "gdi32.lib")
	#pragma comment(lib, "user32.lib")
#endif
