#pragma once
/**
	@file
	@brief definition of abstruct exception class
	Copyright (C) 2008 Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include <algorithm>
#include <sstream>
#include <errno.h>
#ifdef _WIN32
	#include <winsock2.h>
	#include <windows.h>
#else
	#include <string.h> // for strerror_r
#endif
#include <cybozu/inttype.hpp>
#ifdef CYBOZU_USE_STACKTRACE
	#include <cybozu/stacktrace.hpp>
#endif

namespace cybozu {

const bool DontThrow = true;

namespace exception {

/* get max 16 characters to avoid buffer overrun */
inline std::string makeString(const char *str, size_t size)
{
	return std::string(str, std::min<size_t>(size, 16));
}

} // cybozu::exception

/**
	convert errno to string
	@param err [in] errno
	@note for both windows and linux
*/
inline std::string ConvertErrorNoToString(int err)
{
	char errBuf[256];
#ifdef _WIN32
	strerror_s(errBuf, sizeof(errBuf), err);
	return errBuf;
#elif defined(_GNU_SOURCE)
	return ::strerror_r(err, errBuf, sizeof(errBuf));
#else
	if (strerror_r(err, errBuf, sizeof(errBuf)) == 0) {
		return errBuf;
	} else {
		return "strerror_r error";
	}
#endif
}

class Exception : public std::exception {
	std::string str_;
public:
	explicit Exception(const std::string& name = "")
		: str_(name)
	{
#ifdef CYBOZU_USE_STACKTRACE
		str_ += ';';
		str_ += cybozu::StackTrace().toString();
#endif
	}
	~Exception() throw() {}
	const char *what() const throw() { return str_.c_str(); }
	const std::string& toString() const throw() { return str_; }
	template<class T>
	Exception& operator<<(const T& x)
	{
		str_ += ':';
		std::ostringstream os;
		os << x;
		str_ += os.str();
		return *this;
	}
};

class ErrorNo {
public:
#ifdef _WIN32
	typedef unsigned int NativeErrorNo;
#else
	typedef int NativeErrorNo;
#endif
	explicit ErrorNo(NativeErrorNo err)
		: err_(err)
	{
	}
	ErrorNo()
		: err_(getLatestNativeErrorNo())
	{
	}
	NativeErrorNo getLatestNativeErrorNo() const
	{
#ifdef _WIN32
		return ::GetLastError();
#else
		return errno;
#endif
	}
	/**
		convert NativeErrNo to string(maybe UTF8)
		@param err [in] errno
		@note Linux   : same as ConvertErrorNoToString
			  Windows : for Win32 API(use en-us)
	*/
	std::string toString() const
	{
#ifdef _WIN32
		const int msgSize = 256;
		wchar_t msg[msgSize];
		int size = FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0,
			err_,
			MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			msg,
			msgSize,
			NULL
		);
		if (size <= 0) return "";
		// remove last "\r\n"
		if (size > 2 && msg[size - 2] == '\r') {
			msg[size - 2] = 0;
			size -= 2;
		}
		std::string ret;
		ret.resize(size);
		// assume ascii only
		for (int i = 0; i < size; i++) {
			ret[i] = (char)msg[i];
		}
		return ret;
#else
		return ConvertErrorNoToString(err_);
#endif
	}
private:
	NativeErrorNo err_;
};

inline std::ostream& operator<<(std::ostream& os, const cybozu::ErrorNo& self)
{
	return os << self.toString();
}

} // cybozu
