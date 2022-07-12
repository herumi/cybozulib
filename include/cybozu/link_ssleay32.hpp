#pragma once
/**
	@file
	@brief link ssleay32.lib of openssl
	@author MITSUNARI Shigeo(@herumi)
*/
#if defined(_MSC_VER) && defined(_MT)
	#if _MSC_VER >= 1900 // VC2015
		#ifdef _WIN64
			#pragma comment(lib, "mt/14/ssleay32.lib")
		#else
			#pragma comment(lib, "mt/14/32/ssleay32.lib")
		#endif
//	#elif _MSC_VER == 1800 // VC2013
	#else
		#pragma comment(lib, "mt/12/ssleay32.lib")
	#endif
	#pragma comment(lib, "user32.lib")
#endif
