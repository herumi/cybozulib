#pragma once
/**
	@file
	@brief format string

	Copyright (C) 2011 Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <cybozu/exception.hpp>

namespace cybozu {

inline void vformat(std::string& str, const char *format, va_list args)
{
#ifdef _MSC_VER
	va_list args2;
	va_copy(args2, args);
	_locale_t curLoc = _get_current_locale();
	int size = _vscprintf_l(format, curLoc, args);
	if (size < 0 || size >= INT_MAX) throw cybozu::Exception("vformat:_vscprintf_l");

	str.resize(size + 1);

	int ret = _vsprintf_s_l(&str[0], size + 1, format, curLoc, args2);
	if (ret < 0) throw cybozu::Exception("vformat:_vsprintf_s_l");
	str.resize(size);
#else
	char *p;
	int ret = vasprintf(&p, format, args);
	if (ret < 0) throw cybozu::Exception("vformat:vasnprintf");
    try {
		str.assign(p, ret);
		free(p);
	} catch (...) {
		free(p);
		throw std::bad_alloc();
	}
#endif
}

inline void format(std::string& str, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	cybozu::vformat(str, format, args);
	va_end(args);
}

inline std::string format(const char *format, ...)
{
	std::string str;
	va_list args;
	va_start(args, format);
	cybozu::vformat(str, format, args);
	va_end(args);
	return str;
}

} // cybozu
