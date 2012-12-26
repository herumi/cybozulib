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

std::string format(const char *format, ...)
{
#ifdef _MSC_VER
	va_list args;
	va_start(args, format);
	_locale_t curLoc = _get_current_locale();
	va_start(args, format);
	int size = _vscprintf_l(format, curLoc, args);
	va_end(args);
	if (size < 0 || size >= INT_MAX) throw cybozu::Exception("format:_vscprintf_l");

	std::string str;
	str.resize(size + 1);

	va_start(args, format);
	int ret = _vsprintf_s_l(&str[0], size + 1, format, curLoc, args);
	va_end(args);
	if (ret < 0) throw cybozu::Exception("format:_vsprintf_s_l");
	str.resize(size);
	return str;
#else
	char *p;
	va_list args;
	va_start(args, format);
	int ret = vasprintf(&p, format, args);
	va_end(args);
	if (ret < 0) throw cybozu::Exception("format:vasnprintf");
    try {
		std::string str(p, ret);
		free(p);
		return str;
	} catch (...) {
		free(p);
		throw std::bad_alloc();
	}
#endif
}

} // cybozu
