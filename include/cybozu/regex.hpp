#pragma once
/**
	@file
	@brief regex for cybozu::String
	@note regex version (for win) does not support icase

	@author MITSUNARI Shigeo
*/

#include <cybozu/string.hpp>

#ifdef __GNUC__
	#define CYBOZU_RE_USE_BOOST_REGEX
#endif

#ifdef CYBOZU_RE_USE_BOOST_REGEX
	#define CYBOZU_RE_STD boost
	#ifndef BOOST_REGEX_USE_CPP_LOCALE
		#define BOOST_REGEX_USE_CPP_LOCALE
	#endif
	#include <boost/regex.hpp>
#else
	#define CYBOZU_RE_STD std
	#ifdef _MSC_VER
	#ifdef _DLL_CPPLIB
		#error "use /MT or /MTd option. /MD and /MDd are not supported"
	#endif
		#define CYBOZU_STRING_USE_WIN
		#pragma warning(disable : 4018) // signed/unsigned mismatch
	#endif
	#include <regex>
#endif

#ifdef _MSC_VER
	#define CYBOZU_RE_CHAR wchar_t
	#define CYBOZU_RE(x) L##x
#else
	#define CYBOZU_RE_CHAR char
	#define CYBOZU_RE(x) x
#endif

namespace regex_local {

/* true if c in [min, max] */
inline bool in(unsigned int c, char min, char max)
{
//	  return min <= c && c <= max;
	return static_cast<unsigned int>(c - min) <= static_cast<unsigned int>(max - min);
}

} // regex_local

#ifdef CYBOZU_STRING_USE_WIN
template<>
	class CYBOZU_RE_STD::regex_traits<cybozu::Char>
	: public CYBOZU_RE_STD::_Regex_traits<cybozu::Char>
	{   // specialization for char
public:
	int value(cybozu::Char ch, int base) const
		{   // map character value to numeric value
		if (base != 8 && '0' <= ch && ch <= '9'
			|| base == 8 && '0' <= ch && ch <= '7')
			return (ch - '0');
		else if (base != 16)
			;
		else if ('a' <= ch && ch <= 'f')
			return (ch - 'a' + 10);
		else if ('A' <= ch && ch <= 'F')
			return (ch - 'A' + 10);
		return (-1);
		}
	};

 #define _REGEX_CHAR_CLASS_NAME(n, c)   { n, sizeof(n)/sizeof(n[0]) - 1, c }

namespace cybozu { namespace string_local {

static const cybozu::Char _alpha[] = { 'a', 'l', 'p', 'h', 'a', 0 };
static const cybozu::Char _blank[] = { 'b', 'l', 'a', 'n', 'k', 0 };
static const cybozu::Char _cntrl[] = {'c', 'n', 't', 'r', 'l', 0 };
static const cybozu::Char _d[] = {'d', 0 };
static const cybozu::Char _digit[] = {'d', 'i', 'g', 'i', 't', 0 };
static const cybozu::Char _graph[] = {'g', 'r', 'a', 'p', 'h', 0 };
static const cybozu::Char _lower[] = {'l', 'o', 'w', 'e', 'r', 0 };
static const cybozu::Char _print[] = {'p', 'r', 'i', 'n', 't', 0 };
static const cybozu::Char _punct[] = {'p', 'u', 'n', 'c', 't', 0 };
static const cybozu::Char _space[] = {'s', 'p', 'a', 'c', 'e', 0 };
static const cybozu::Char _s[] = {'s', 0 };
static const cybozu::Char _upper[] = {'u', 'p', 'p', 'e', 'r', 0 };
static const cybozu::Char _w[] = {'w', 0 };
static const cybozu::Char _xdigit[]= {'x', 'd', 'i', 'g', 'i', 't', 0 };

} }

template<>
	const CYBOZU_RE_STD::_Cl_names<cybozu::Char> CYBOZU_RE_STD::_Regex_traits<cybozu::Char>::_Names[] =
	{   // map class names to numeric constants
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_alpha, _Regex_traits<cybozu::Char>::_Ch_alnum),
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_blank, _Regex_traits<cybozu::Char>::_Ch_blank),
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_cntrl, _Regex_traits<cybozu::Char>::_Ch_cntrl),
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_d, _Regex_traits<cybozu::Char>::_Ch_digit),
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_digit, _Regex_traits<cybozu::Char>::_Ch_digit),
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_graph, _Regex_traits<cybozu::Char>::_Ch_graph),
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_lower, _Regex_traits<cybozu::Char>::_Ch_lower),
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_print, _Regex_traits<cybozu::Char>::_Ch_print),
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_punct, _Regex_traits<cybozu::Char>::_Ch_punct),
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_space, _Regex_traits<cybozu::Char>::_Ch_space),
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_s, _Regex_traits<cybozu::Char>::_Ch_space),
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_upper, _Regex_traits<cybozu::Char>::_Ch_upper),
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_w, (_STD ctype_base::mask)(-1)),
	_REGEX_CHAR_CLASS_NAME(cybozu::string_local::_xdigit, _Regex_traits<cybozu::Char>::_Ch_xdigit),
	{0, 0, 0},
	};

#undef _REGEX_CHAR_CLASS_NAME

#endif

namespace cybozu {

/* constructor from basic_regexp.hpp */
struct regex : public CYBOZU_RE_STD::basic_regex<Char> {
	typedef size_t size_type;
	explicit regex(){}
	explicit regex(const Char* p, flag_type f = CYBOZU_RE_STD::regex_constants::ECMAScript)
		: CYBOZU_RE_STD::basic_regex<Char>(p, f)
	{
	}
#ifdef _MSC_VER
	explicit regex(const wchar_t* p, flag_type f = CYBOZU_RE_STD::regex_constants::ECMAScript)
		: CYBOZU_RE_STD::basic_regex<Char>(String(p).c_str(), f)
	{
	}
#endif
	explicit regex(const String& p, flag_type f = CYBOZU_RE_STD::regex_constants::ECMAScript)
		: CYBOZU_RE_STD::basic_regex<Char>(p.c_str(), f)
	{
	}
	regex(const Char* p1, const Char* p2, flag_type f = CYBOZU_RE_STD::regex_constants::ECMAScript)
		: CYBOZU_RE_STD::basic_regex<Char>(p1, p2, f)
	{
	}
	regex(const Char* p, size_type len, flag_type f)
		: CYBOZU_RE_STD::basic_regex<Char>(p, len, f)
	{
	}
	regex(const regex& that)
	  : CYBOZU_RE_STD::basic_regex<Char>(that) {}
};

#if 0
struct smatch : public CYBOZU_RE_STD::match_results<cybozu::String::const_iterator> {
	String str(int sub = 0) const {
		return CYBOZU_RE_STD::match_results<cybozu::String::const_iterator>::str(sub);
	}
//	String prefix() const {
//		return CYBOZU_RE_STD::match_results<cybozu::String::const_iterator>::prefix();
//	}
};

struct sregex_iterator : public CYBOZU_RE_STD::regex_iterator<String::const_iterator> {
   sregex_iterator(){}
   sregex_iterator(const String::const_iterator a, const String::const_iterator b, const regex& re, CYBOZU_RE_STD::regex_constants::match_flag_type m = CYBOZU_RE_STD::regex_constants::match_default)
	  : CYBOZU_RE_STD::regex_iterator<String::const_iterator>(a, b, re, m) {}
   sregex_iterator(const sregex_iterator& that)
	  : CYBOZU_RE_STD::regex_iterator<String::const_iterator>(that) {}
#ifndef CYBOZU_STRING_USE_WIN /* return temporary address when using vc tr1? */
   const smatch& operator*()const
   { return *static_cast<const smatch*>(&CYBOZU_RE_STD::regex_iterator<String::const_iterator>(*this).operator*()); }
   const smatch* operator->()const
   { return static_cast<const smatch*>(CYBOZU_RE_STD::regex_iterator<String::const_iterator>(*this).operator->()); }
#endif
};
#endif

#if 0
inline bool regex_search(const String::const_iterator begin, const String::const_iterator end, smatch& m, const regex& e, CYBOZU_RE_STD::regex_constants::match_flag_type flags = CYBOZU_RE_STD::regex_constants::match_default)
{
	return CYBOZU_RE_STD::regex_search(begin, end, m, e, flags);
}

inline bool regex_search(const String& s, smatch& m, const regex& e, CYBOZU_RE_STD::regex_constants::match_flag_type flags = CYBOZU_RE_STD::regex_constants::match_default)
{
	return regex_search(s.begin(), s.end(), m, e, flags);
}

inline bool regex_search(const String& s, const regex& e, CYBOZU_RE_STD::regex_constants::match_flag_type flags = CYBOZU_RE_STD::regex_constants::match_default)
{
	smatch m;
	return regex_search(s, m, e, flags);
}

inline bool regex_search(const String::const_iterator begin, const String::const_iterator end, const regex& e, CYBOZU_RE_STD::regex_constants::match_flag_type flags = CYBOZU_RE_STD::regex_constants::match_default)
{
	smatch m;
	return regex_search(begin, end, m, e, flags);
}
#endif

inline String regex_replace(const String::const_iterator begin, const String::const_iterator end, const regex& e, const String& fmt, CYBOZU_RE_STD::regex_constants::match_flag_type flags = CYBOZU_RE_STD::regex_constants::match_default)
{
	String result;
	CYBOZU_RE_STD::regex_replace(std::back_inserter(result), begin, end, e, fmt, flags);
	return result;
}

inline String regex_replace(const String& s, const regex& e, const String& fmt, CYBOZU_RE_STD::regex_constants::match_flag_type flags = CYBOZU_RE_STD::regex_constants::match_default)
{
	return regex_replace(s.begin(), s.end(), e, fmt, flags);
}


#if 0
inline bool regex_match(const String::const_iterator begin, const String::const_iterator end, smatch& m, const regex& e, CYBOZU_RE_STD::regex_constants::match_flag_type flags = CYBOZU_RE_STD::regex_constants::match_default)
{
	return CYBOZU_RE_STD::regex_match(begin, end, m, e, flags);
}

inline bool regex_match(const String& s, smatch& m, const regex& e, CYBOZU_RE_STD::regex_constants::match_flag_type flags = CYBOZU_RE_STD::regex_constants::match_default)
{
	return regex_match(s.begin(), s.end(), m, e, flags);
}

inline bool regex_match(const String::const_iterator begin, const String::const_iterator end, const regex& e, CYBOZU_RE_STD::regex_constants::match_flag_type flags = CYBOZU_RE_STD::regex_constants::match_default)
{
	smatch m;
	return regex_match(begin, end, m, e, flags);
}

inline bool regex_match(const String& s, const regex& e, CYBOZU_RE_STD::regex_constants::match_flag_type flags = CYBOZU_RE_STD::regex_constants::match_default)
{
	smatch m;
	return regex_match(s, m, e, flags);
}
#endif

//using CYBOZU_RE_STD::regex_search;
//typedef basic_regex<cybozu::Char> regex;

using CYBOZU_RE_STD::regex_iterator;
using CYBOZU_RE_STD::basic_regex;
using CYBOZU_RE_STD::regex_search;
using CYBOZU_RE_STD::match_results;
//using CYBOZU_RE_STD::regex_replace;
using CYBOZU_RE_STD::regex_match;
using CYBOZU_RE_STD::regex_token_iterator;

typedef match_results<cybozu::String::const_iterator> smatch;
typedef regex_iterator<String::const_iterator> sregex_iterator;
typedef regex_token_iterator<cybozu::String::const_iterator> sregex_token_iterator;

} // cybozu

#ifdef _MSC_VER

namespace std {

template<>
bool ctype<cybozu::Char>::is(std::ctype_base::mask m, cybozu::Char c) const
{
/*
	s(48):p(1d7):c(20):u(1):l(2):a(103):d(4):p(10):x(80)
printf("u(%x):l(%x):d(%x):p(%x):c(%x):x(%x):s(%x):p(%x):a(%x)\n"
, std::ctype_base::upper //  0x001
, std::ctype_base::lower //  0x002
, std::ctype_base::digit //  0x004
, std::ctype_base::punct //  0x010
, std::ctype_base::cntrl //  0x020
, std::ctype_base::xdigit // 0x080
, std::ctype_base::space //  0x048
, std::ctype_base::print //  0x1d7
, std::ctype_base::alpha //  0x103
);exit(1);
*/
	if (c & ~0xFFU) return false;
#if 1
	static struct Custom : public std::ctype<char> {
		const mask * table() const { return std::ctype<char>::table(); }
	} custom;
	static const mask* maskTbl = custom.table();
	return (maskTbl[(unsigned char)c] & m) != 0;
#else
	static const std::ctype<char>& cache = use_facet<ctype<char> >(std::locale());
	return cache.is(m, static_cast<char>(c));
#endif
}

#if 0
template<>
const cybozu::Char* ctype<cybozu::Char>::is(const cybozu::Char* low, const cybozu::Char* high, mask* vec) const
{
	return do_is(low, high, vec);
}

template<>
const cybozu::Char* ctype<cybozu::Char>::scan_is(std::ctype_base::mask m, const cybozu::Char* low, const cybozu::Char* high) const
{
	return do_scan_is(m, low, high);
}

template<>
const cybozu::Char* ctype<cybozu::Char>::scan_not(std::ctype_base::mask m, const cybozu::Char* low, const cybozu::Char* high) const
{
	return do_scan_not(m, low, high);
}
#endif

template<>
cybozu::Char ctype<cybozu::Char>::toupper(cybozu::Char c) const
{
	if ('a' <= c && c <= 'z') return c - 'a' + 'A';
	return c;
}

template<>
const cybozu::Char* ctype<cybozu::Char>::toupper(cybozu::Char* low, const cybozu::Char* high) const
{
	while (low != high) {
		*low = ctype<cybozu::Char>::toupper(*low);
		low++;
	}
	return high;
}

template<>
cybozu::Char ctype<cybozu::Char>::tolower(cybozu::Char c) const
{
	if ('A' <= c && c <= 'Z') return c - 'A' + 'a';
	return c;
}
template<>
const cybozu::Char* ctype<cybozu::Char>::tolower(cybozu::Char* low, const cybozu::Char* high) const
{
	while (low != high) {
		*low = ctype<cybozu::Char>::tolower(*low);
		low++;
	}
	return high;
}

template<>
cybozu::Char ctype<cybozu::Char>::widen(char c) const
{
	return c;
}

template<>
const char* ctype<cybozu::Char>::widen(const char* low, const char* high, cybozu::Char* to) const
{
	while (low != high) {
		*to++ = ctype<cybozu::Char>::widen(*low++);
	}
	return high;
}

template<>
char ctype<cybozu::Char>::narrow(cybozu::Char c, char dfault) const
{
	if ((0 <= c && c < 256) || (c == -1)) return static_cast<char>(c);
	return dfault;
}

template<>
const cybozu::Char* ctype<cybozu::Char>::narrow(const cybozu::Char* low, const cybozu::Char* high, char dfault, char* to) const
{
	while (low != high) {
		*to++ = ctype<cybozu::Char>::narrow(*low++, dfault);
	}
	return high;
}

//template<> __PURE_APPDOMAIN_GLOBAL locale::id collate<cybozu::Char>::id;

#if defined(_DLL_CPPLIB)
template __PURE_APPDOMAIN_GLOBAL locale::id collate<cybozu::Char>::id;
#endif /* defined(_DLL_CPPLIB) etc. */

} // std

#endif

