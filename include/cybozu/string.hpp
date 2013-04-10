#pragma once
/**
	@file
	@brief unicode string class like std::string
	support char*, std::string with UTF-8

	Copyright (C) 2008 Cybozu Labs, Inc., all rights reserved.
*/

#include <string>
#include <cstring>
#include <assert.h>
#include <stddef.h>
#include <stdio.h> // for printf
#include <iosfwd> // for istream, ostream
#include <functional> // for unary_function

#include <cybozu/exception.hpp>
#include <cybozu/hash.hpp>

namespace cybozu {

#ifdef __GNUC__
	/* avoid to use uint32_t because compiling boost::regex fails */
	typedef int Char; //!< Char for Linux
	typedef unsigned short Char16; /* unsigned is necessary for gcc */
#else
	/* can't compile with singed */
	typedef int Char; //!< Char for Windows
	typedef wchar_t Char16;
#endif

typedef std::basic_string<Char16> String16;

/**
	utility function
*/
namespace string {

/*
	 code point[a, b] 1byte  2ybte  3byte  4byte
	  U+0000   U+007f 00..7f                      ; 128
	  U+0080   U+07ff c2..df 80..bf               ; 30 x 64 = 1920

	  U+0800   U+0fff e0     a0..bf 80..bf        ;  1 x 32 x 64 = 2048
	  U+1000   U+cfff e1..ec 80..bf 80..bf        ; 12 x 64 x 64 = 49152
	  U+d000   U+d7ff ed     80..9f 80..bf        ;  1 x 32 x 64 = 2048

	  U+e000   U+ffff ee..ef 80..bf 80..bf        ;  2 x 64 x 64 = 8192

	 U+10000  U+3ffff f0     90..bf 80..bf 80..bf ;  1 x 48 x 64 x 64 = 196608
	 U+40000  U+fffff f1..f3 80..bf 80..bf 80..bf ;  3 x 64 x 64 x 64 = 786432
	U+100000 U+10ffff f4     80..8f 80..bf 80..bf ;  1 x 16 x 64 x 64 = 65536
*/
inline int GetCharSize(Char c)
{
	if (c <= 0x7f) return 1;
	if (c <= 0x7ff) return 2;
	if (c <= 0xd7ff) return 3;
	if (c <= 0xdfff || c > 0x10ffff) return 0;
	if (c <= 0xffff) return 3;
	return 4;
}

// for Char/char
inline bool IsValidChar(Char c)
{
	return GetCharSize(c) != 0;
}

namespace local {

/* true if c in [min, max] */
inline bool in(unsigned char c, int min, int max)
{
//	  return min <= c && c <= max;
	return static_cast<unsigned int>(c - min) <= static_cast<unsigned int>(max - min);
}

template<class T>
struct IsInt { enum { value = false }; };
template<>struct IsInt<int> { enum { value = true }; };
template<>struct IsInt<unsigned int> { enum { value = true }; };
template<>struct IsInt<size_t> { enum { value = true }; };

template <bool b, class T = void>
struct disable_if { typedef T type; };

template <class T>
struct disable_if<true, T> {};

} // local

/*
	get one character from UTF-8 string and seek begin to next char
	@note begin != end
	@note begin is not determined if false
*/
template<class Iterator>
bool GetCharFromUtf8(Char *c, Iterator& begin, const Iterator& end)
{
	unsigned char c0 = *begin++;
	if (c0 <= 0x7f) {
		*c = c0;
		return true;
	}
	if (local::in(c0, 0xc2, 0xdf)) {
		if (begin != end) {
			unsigned char c1 = *begin++;
			if (local::in(c1, 0x80, 0xbf)) {
				*c = ((c0 << 6) | (c1 & 0x3f)) - 0x3000;
				return true;
			}
		}
	} else if (c0 <= 0xef) {
		if (begin != end) {
			unsigned char c1 = *begin++;
			if (begin != end) {
				unsigned char c2 = *begin++;
				if (local::in(c2, 0x80, 0xbf)) {
					if ((c0 == 0xe0 && local::in(c1, 0xa0, 0xbf))
					 || (local::in(c0, 0xe1, 0xec) && local::in(c1, 0x80, 0xbf))
					 || (c0 == 0xed && local::in(c1, 0x80, 0x9f))
					 || (local::in(c0, 0xee, 0xef) && local::in(c1, 0x80, 0xbf))) {
						*c = ((c0 << 12) | ((c1 & 0x3f) << 6) | (c2 & 0x3f)) - 0xe0000;
						return true;
					}
				}
			}
		}
	} else if (local::in(c0, 0xf0, 0xf4)) {
		if (begin != end) {
			unsigned char c1 = *begin++;
			if (begin != end) {
				unsigned char c2 = *begin++;
				if (begin != end) {
					unsigned char c3 = *begin++;
					if (local::in(c2, 0x80, 0xbf) && local::in(c3, 0x80, 0xbf)) {
						if ((c0 == 0xf0 && local::in(c1, 0x90, 0xbf))
						 || (local::in(c0, 0xf1, 0xf3) && local::in(c1, 0x80, 0xbf))
						 || (c0 == 0xf4 && local::in(c1, 0x80, 0x8f))) {
							*c = ((c0 << 18) | ((c1 & 0x3f) << 12) | ((c2 & 0x3f) << 6) | (c3 & 0x3f)) - 0x3c00000;
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

/*
	get one character from UTF-16 string and seek begin to next char
	@note begin != end
	@note begin is not determined if false
*/
template<class Iterator>
bool GetCharFromUtf16(Char& c, Iterator& begin, const Iterator& end)
{
	struct local {
		static inline bool isLead(Char c) { return (c & 0xfffffc00) == 0xd800; }
		static inline bool isTrail(Char c) { return (c & 0xfffffc00) == 0xdc00; }
	};
	Char16 c0 = *begin++;
	if (!local::isLead(c0)) {
		c = c0;
		return true;
	}
	if (begin != end) {
		Char16 c1 = *begin++;
		if (local::isTrail(c1)) {
			const Char offset = (0xd800 << 10UL) + 0xdc00 - 0x10000;
			c = (c0 << 10) + c1 - offset;
			return true;
		}
	}
	return false;
}

inline int toUtf8(char out[4], Char c)
{
	if (c <= 0x7f) {
		out[0] = static_cast<char>(c);
		return 1;
	} else if (c <= 0x7ff) {
		out[0] = static_cast<char>((c >> 6) | 0xc0);
		out[1] = static_cast<char>((c & 0x3f) | 0x80);
		return 2;
	} else if (c <= 0xffff) {
		if (0xd7ff < c && c <= 0xdfff) {
			return 0;
		}
		out[0] = static_cast<char>((c >> 12) | 0xe0);
		out[1] = static_cast<char>(((c >> 6) & 0x3f) | 0x80);
		out[2] = static_cast<char>((c & 0x3f) | 0x80);
		return 3;
	} else if (c <= 0x10ffff) {
		out[0] = static_cast<char>((c >> 18) | 0xf0);
		out[1] = static_cast<char>(((c >> 12) & 0x3f) | 0x80);
		out[2] = static_cast<char>(((c >> 6) & 0x3f) | 0x80);
		out[3] = static_cast<char>((c & 0x3f) | 0x80);
		return 4;
	}
	return 0;
}

inline int toUtf16(Char16 out[2], Char c)
{
	if (c <= 0xffff) {
		out[0] = static_cast<Char16>(c);
		return 1;
	} else if (c <= 0x0010ffff) {
		out[0] = static_cast<Char16>((c >> 10) + 0xd7c0);
		out[1] = static_cast<Char16>((c & 0x3ff) | 0xdc00);
		return 2;
	}
	return 0;
}

inline bool AppendUtf8(std::string& out, Char c)
{
	char buf[4];
	int len = toUtf8(buf, c);
	if (len > 0) {
		out.append(buf, len);
		return true;
	}
	return false;
}

inline bool AppendUtf16(String16& out, Char c)
{
	Char16 buf[2];
	int len = toUtf16(buf, c);
	if (len > 0) {
		out.append(buf, len);
		return true;
	}
	return false;
}

} } // cybozu::string

namespace std {

template<>
class basic_string<cybozu::Char> {
	struct Box {
		cybozu::Char c;
		bool operator==(const Box& rhs) const { return c == rhs.c; }
		bool operator<(const Box& rhs) const { return c < rhs.c; }
	};
	typedef std::basic_string<Box> inString;
public:
	typedef cybozu::Char value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef cybozu::Char& reference;
	typedef const cybozu::Char& const_reference;
	typedef cybozu::Char* pointer;
	typedef const cybozu::Char* const_pointer;
	typedef cybozu::Char* iterator;
	typedef const cybozu::Char* const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	static const size_t npos = size_t(-1);

	/**
		dump unicode of string for debug
		@param msg [local::in] user message
	*/
	void dump(const char *msg = 0) const
	{
		if (msg) printf("%s", msg);
		for (size_t i = 0; i < size(); i++) {
			printf("%08x ", str_[i].c);
		}
		printf("\n");
	}

	/**
		construct empty string
	*/
	basic_string() { }

	/**
		construct from str [off, off + count)
		@param str [local::in] original string
		@param off [local::in] offset
		@param count [local::in] count of character(default npos)
	*/
	basic_string(const basic_string& str, size_type off, size_type count = npos)
		: str_(str.str_, off, count)
	{ }

	/**
		construct from [str, str + count)
		@param str [local::in] original string
		@param count [local::in] count of character
	*/
	basic_string(const cybozu::Char *str, size_type count)
	{
		append(str, count);
	}

	/**
		construct from [str, NUL)
		@param str [local::in] original string
	*/
	basic_string(const cybozu::Char *str)
	{
		append(str);
	}
	/**
		construct from count * c
		@param count [local::in] count of character
		@param c [local::in] initial character
	*/
	basic_string(size_type count, cybozu::Char c)
	{
		append(count, c);
	}

	/**
		construct from [begin, end)
		@param begin [local::in] begin of iterator
		@param end [local::in] end of iterator
	*/
	template<class Iterator>
	basic_string(Iterator begin, Iterator end, typename cybozu::string::local::disable_if<cybozu::string::local::IsInt<Iterator>::value>::type* = 0)
	{
		append(begin, end);
	}

	// construct from [begin, end), const pointers
//  basic_string(const_pointer begin, const_pointer end);
	// construct from [begin, end), const_iterators
//  basic_string(const_iterator begin, const_iterator end);

	/**
		construct by copying str
		@param str [local::in] original string
	*/
	basic_string(const basic_string& str)
		: str_(str.str_)
	{
	}

	/**
		construct by [str, str + count)
		@param str [local::in] original string
		@param count [local::in] count of character
	*/
	basic_string(const char *str, size_type count) // A
	{
		append(str, count);
	}

	/**
		construct from [str, NUL)
		@param str [local::in] original string
	*/
	basic_string(const char *str) // A
	{
		append(str);
	}
	/**
		construct by copying str
		@param str [local::in] original string
	*/
	basic_string(const std::string& str) // A
	{
		append(str);
	}

	/**
		construt by cybozu::Char16(same ICU::UChar)
		@param str [local::in] UTF-16 format string
	*/
	basic_string(const cybozu::String16& str) // A
	{
		cybozu::String16::const_iterator begin = str.begin(), end = str.end();
		while (begin != end) {
			Box b;
			if (!cybozu::string::GetCharFromUtf16(b.c, begin, end)) {
				throw cybozu::Exception("string:basic_string:UTF-16");
			}
			str_ += b;
		}
	}
	/**
		construct from [str, NUL)
		@param str [local::in] original string
	*/
	basic_string(const cybozu::Char16 *buf)
	{
		const cybozu::String16 str(buf);
		cybozu::String16::const_iterator begin = str.begin(), end = str.end();
		while (begin != end) {
			Box b;
			if (!cybozu::string::GetCharFromUtf16(b.c, begin, end)) {
				throw cybozu::Exception("string:basic_string:UTF-16");
			}
			str_ += b;
		}
	}

	/**
		assign str
		@param str [local::in] assign string
	*/
	basic_string& operator=(const basic_string& str)
	{
		return assign(str);
	}

	/**
		assign [str, NUL)
		@param str [local::in] assign string
	*/
	basic_string& operator=(const cybozu::Char *str)
	{
		return assign(str);
	}

	/**
		assign 1 * c
		@param c [local::in] initial character
	*/
	basic_string& operator=(cybozu::Char c)
	{
		return assign(1u, c);
	}

	/**
		assign [str, NUL)
		@param str [local::in] assign string
	*/
	basic_string& operator=(const char *str) // A
	{
		return assign(str);
	}
	/**
		assign str
		@param str [local::in] assign string
	*/
	basic_string& operator=(const std::string& str) // A
	{
		return assign(str);
	}

	/**
		append str
		@param str [local::in] append string
	*/
	basic_string& operator+=(const basic_string& str)
	{
		return append(str);
	}

	/**
		append [str, NUL)
		@param str [local::in] append string
	*/
	basic_string& operator+=(const cybozu::Char *str)
	{
		return append(str);
	}

	/**
		append 1 * c
		@param c [local::in] append character
	*/
	basic_string& operator+=(cybozu::Char c)
	{
		return append(1u, c);
	}

	/**
		append str
		@param str [local::in] append string
	*/
	basic_string& append(const basic_string& str)
	{
		str_.append(str.str_); return *this;
	}

	/**
		append str [off, off + count)
		@param str [local::in] append string
		@param off [local::in] string offset
		@param count [local::in] count of character
	*/
	basic_string& append(const basic_string& str, size_type off, size_type count)
	{
		str_.append(str.str_, off, count); return *this;
	}

	/**
		append [str, str + count)
		@param str [local::in] append string
		@param count [local::in] count of character
	*/
	basic_string& append(const cybozu::Char *str, size_type count)
	{
		return append(str, str + count);
	}

	/**
		append [str, NUL)
		@param str [local::in] append string
	*/
	basic_string& append(const cybozu::Char *str)
	{
		str_.append((const Box*)str); return *this;
	}

	/**
		append count * c
		@param count [local::in] count of character
		@param c [local::in] initial character
	*/
	basic_string& append(size_type count, cybozu::Char c)
	{
		Box b; b.c = c;
		str_.append(count, b); return *this;
	}

	/**
		append [begin, end)
		@param begin [local::in] begin of iterator
		@param end [local::in] end of iterator
	*/
	template<class Iterator>
	basic_string& append(Iterator begin, Iterator end, typename cybozu::string::local::disable_if<cybozu::string::local::IsInt<Iterator>::value>::type* = 0)
	{
		while (begin != end) {
			Box b;
			b.c = getOneChar(begin, end);
			str_.push_back(b);
		}
		return *this;
	}

	// append [begin, end), const pointers
//  basic_string& append(const_pointer begin, const_pointer end);
	// append [begin, end), const_iterators
//  basic_string& append(const_iterator begin, const_iterator end);

	/**
		append [str, str + count)
		@param str [local::in] append string
		@param count [local::in] count of character
	*/
	basic_string& append(const char *str, size_type count) // A
	{
		return append(str, str + count);
	}

	/**
		append [str, NUL)
		@param str [local::in] append string
	*/
	basic_string& append(const char *str) // A
	{
		return append(str, std::strlen(str));
	}
	/**
		append str
		@param str [local::in] append string
	*/
	basic_string& append(const std::string& str) // A
	{
		return append(str.begin(), str.end());
	}

	/**
		assign str
		@param str [local::in] assign str
	*/
	basic_string& assign(const basic_string& str)
	{
		clear(); return append(str);
	}

	/**
		assign str [off, off + count)
		@param str [local::in] assign string
		@param off [local::in] offset
		@param count [local::in] count of character
	*/
	basic_string& assign(const basic_string& str, size_type off, size_type count)
	{
		clear(); return append(str, off, count);
	}

	/**
		assign [str, str + count)
		@param str [local::in] assign string
		@param count [local::in] count of character
	*/
	basic_string& assign(const cybozu::Char *str, size_type count)
	{
		return assign(str, str + count);
	}

	/**
		assign [str, NUL)
		@param str [local::in] assign string
	*/
	basic_string& assign(const cybozu::Char *str)
	{
		clear(); return append(str);
	}

	/**
		assign count * c
		@param count [local::in] count of character
		@param c [local::in] initial character
	*/
	basic_string& assign(size_type count, cybozu::Char c)
	{
		clear(); return append(count, c);
	}

	/**
		assign [First, end)
		@param begin [local::in] begin of iterator
		@param end [local::in] end of iterator
	*/
	template<class Iterator>
	basic_string& assign(Iterator begin, Iterator end)
	{
		clear(); return append(begin, end);
	}

	// assign [First, end), const pointers
//  basic_string& assign(const_pointer begin, const_pointer end);

	// assign [First, end), const_iterators
//  basic_string& assign(const_iterator begin, const_iterator end);

	/**
		assign [str, str + count)
		@param str [local::in] original string
		@param count [local::in] count of character
	*/
	basic_string& assign(const char *str, size_type count) // A
	{
		return assign(str, str + count);
	}

	/**
		assign [str, NUL)
		@param str [local::in] original string
	*/
	basic_string& assign(const char *str) // A
	{
		clear(); return append(str);
	}
	/**
		assign str
		@param str [local::in] original string
	*/
	basic_string& assign(const std::string& str) // A
	{
		clear(); return append(str);
	}

	/**
		insert str at off
		@param off [local::in] offset
		@param str [local::in] insert str
	*/
	basic_string& insert(size_type off, const basic_string& str)
	{
		str_.insert(off, str.str_); return *this;
	}

	/**
		insert str [off, off + count) at off
		@param off [local::in] offset of destination
		@param rhs [local::in] source str
		@param rhsOff [local::in] offset of source str
		@param count [local::in] count of source str
	*/
	basic_string& insert(size_type off, const basic_string& rhs, size_type rhsOff, size_type count)
	{
		str_.insert(off, rhs.str_, rhsOff, count); return *this;
	}

	/**
		insert [str, str + count) at off
		@param off [local::in] offset of destination
		@param str [local::in] source str
		@param count [local::in] count of source str
	*/
	basic_string& insert(size_type off, const cybozu::Char *str, size_type count)
	{
		str_.insert(off, (const Box*)str, count); return *this;
	}

	/**
		insert [str, NUL) at off
		@param off [local::in] offset of destination
		@param str [local::in] source str
	*/
	basic_string& insert(size_type off, const cybozu::Char *str)
	{
		str_.insert(off, (const Box*)str); return *this;
	}

	/**
		insert count * c at off
		@param off [local::in] offset of destination
		@param count [local::in] count of source str
		@param c [local::in] initial character
	*/
	basic_string& insert(size_type off, size_type count, cybozu::Char c)
	{
		Box b; b.c = c;
		str_.insert(off, count, b); return *this;
	}
	/**
		insert c at here
		@param here [local::in] offset of destination
		@param c [local::in] initial character(default 0)
	*/
	iterator insert(iterator here, cybozu::Char c = 0)
	{
		Box b; b.c = c;
		return cvt(str_.insert(cvt(here), b));
	}

	/**
		insert count * cybozu::Char at here
		@param here [local::in] offset of destination
		@param count [local::in] count of str
		@param c [local::in] initial character
	*/
	void insert(iterator here, size_type count, cybozu::Char c)
	{
		Box b; b.c = c;
		str_.insert(cvt(here), count, b);
	}

	/**
		insert [begin, end) at here
		@param here [local::in] offset of destination
		@param begin [local::in] begin of iterator
		@param end [local::in] end of iterator
	*/
	template<class Iterator>
	void insert(iterator here, Iterator begin, Iterator end)
	{
		basic_string str(begin, end);
		str_.insert(cvt(here), str.str_.begin(), str.str_.end());
	}

	// insert [begin, end) at here, const pointers
//  void insert(iterator here, const_pointer begin, const_pointer end);
	// insert [begin, end) at here, const_iterators
//  void insert(iterator here, const_iterator begin, const_iterator end);

	/**
		erase elements [off, off + count)
		@param off [local::in] offset
		@param count [local::in] count of character(default npos)
	*/
	basic_string& erase(size_type off = 0, size_type count = npos)
	{
		str_.erase(off, count); return *this;
	}

	/**
		erase element at here
		@param here [local::in] erase from here
	*/
	iterator erase(iterator here)
	{
		return cvt(str_.erase(cvt(here)));
	}

	/**
		erase substring [begin, end)
		@param begin [local::in] begin of iterator
		@param end [local::in] end of iterator
	*/
	iterator erase(iterator begin, iterator end)
	{
		return cvt(str_.erase(cvt(begin), cvt(end)));
	}

	/**
		erase all
	*/
	void clear() { str_.clear(); }

	/**
		replace [off, off + n) with rhs
		@param off [local::in] start offset
		@param n [local::in] count of remove character
		@param rhs [local::in] append string
	*/
	basic_string& replace(size_type off, size_type n, const basic_string& rhs)
	{
		str_.replace(off, n, rhs.str_); return *this;
	}

	/**
		replace [off, off + n) with rhs [rhsOff, rhsOff + count)
		@param off [local::in] start offset
		@param n [local::in] count of remove character
		@param rhs [local::in] append string
		@param rhsOff [local::in] append from
		@param count [local::in] count of append
	*/
	basic_string& replace(size_type off, size_type n, const basic_string& rhs, size_type rhsOff, size_type count)
	{
		str_.replace(off, n, rhs.str_, rhsOff, count); return *this;
	}

	/**
		replace [off, off + n) with [str, str + count)
		@param off [local::in] start offset
		@param n [local::in] count of remove character
		@param str [local::in] append string
		@param count [local::in] count of append
	*/
	basic_string& replace(size_type off, size_type n, const cybozu::Char *str, size_type count)
	{
		str_.replace(off, n, (const Box*)str, count); return *this;
	}

	/**
		replace [off, off + n) with [str, NUL)
		@param off [local::in] start offset
		@param n [local::in] count of remove character
		@param str [local::in] append string
	*/
	basic_string& replace(size_type off, size_type n, const cybozu::Char *str)
	{
		str_.replace(off, n, (const Box*)str); return *this;
	}

	/**
		replace [off, off + n) with count * c
		@param off [local::in] start offset
		@param n [local::in] count of remove character
		@param count [local::in] count of append
		@param c [local::in] initial character
	*/
	basic_string& replace(size_type off, size_type n, size_type count, cybozu::Char c)
	{
		Box b; b.c = c;
		str_.replace(off, n, count, b); return *this;
	}

	/**
		replace [begin, end) with rhs
		@param begin [local::in] begin to remove
		@param end [local::in] end to remove
		@param rhs [local::in] append str
	*/
	basic_string& replace(iterator begin, iterator end, const basic_string& rhs)
	{
		str_.replace(cvt(begin), cvt(end), rhs.str_); return *this;
	}

	/**
		replace [begin, end) with [str, str + count)
		@param begin [local::in] begin to remove
		@param end [local::in] end to remove
		@param str local::in] append str
		@param count [local::in] count of append
	*/
	basic_string& replace(iterator begin, iterator end, const cybozu::Char *str, size_type count)
	{
		str_.replace(cvt(begin), cvt(end), (const Box*)str, count); return *this;
	}

	/**
		replace [begin, end) with [str, NUL)
		@param begin [local::in] begin to remove
		@param end [local::in] end to remove
		@param str local::in] append str
	*/
	basic_string& replace(iterator begin, iterator end, const cybozu::Char *str)
	{
		str_.replace(cvt(begin), cvt(end), (const Box*)str); return *this;
	}

	/**
		replace [begin, end) with count * c
		@param begin [local::in] begin to remove
		@param end [local::in] end to remove
		@param count [local::in] count of append
		@param c [local::in] initial character
	*/
	basic_string& replace(iterator begin, iterator end, size_type count, cybozu::Char c)
	{
		Box b; b.c = c;
		str_.replace(cvt(begin), cvt(end), count, b); return *this;
	}

	/**
		replace [begin, end) with [begin2, end2)
		@param begin [local::in] begin to remove
		@param end [local::in] end to remove
		@param begin2 [local::in] begin to append
		@param end2 [local::in] end to append
	*/
	template<class Iterator>
	basic_string& replace(iterator begin, iterator end, Iterator begin2, Iterator end2)
	{
		basic_string str2(begin2, end2);
		str_.replace(cvt(begin), cvt(end), str2.str_.begin(), str2.str_.end());
		return *this;
	}

	// replace [begin, end) with [begin2, end2), const pointers
//  basic_string& replace(iterator begin, iterator end, const_pointer begin2, const_pointer end2);

	// replace [begin, end) with [begin2, end2), const_iterators
//  basic_string& replace(iterator begin, iterator end, const_iterator begin2, const_iterator end2);

	/**
		return iterator for beginning of mutable sequence
	*/
	iterator begin() { return cvt(str_.begin()); }

	/**
		return iterator for beginning of nonmutable sequence
	*/
	const_iterator begin() const { return &(str_.begin()->c); }

	/**
		return iterator for end of mutable sequence
	*/
	iterator end() { return cvt(str_.end()); }

	/**
		return iterator for end of nonmutable sequence
	*/
	const_iterator end() const { return &(str_.end()->c); }

	/**
		return iterator for beginning of reversed mutable sequence
	*/
	reverse_iterator rbegin() { return reverse_iterator(cvt(str_.end())); }

	/**
		return iterator for beginning of reversed nonmutable sequence
	*/
	const_reverse_iterator rbegin() const { return const_reverse_iterator(&str_[0].c + str_.size()); }

	/**
		return iterator for end of reversed mutable sequence
	*/
	reverse_iterator rend() { return reverse_iterator(&str_[0].c); }

	/**
		return iterator for end of reversed nonmutable sequence
	*/
	const_reverse_iterator rend() const { return const_reverse_iterator(&str_[0].c); }

	/**
		subscript mutable sequence with checking
		@param off [local::in] offset
	*/
	reference at(size_type off) { return str_.at(off).c; }

	/**
		get element at off
		@param off [local::in] offset
	*/
	const_reference at(size_type off) const { return str_.at(off).c; }

	/**
		subscript mutable sequence
		@param off [local::in] offset
	*/
	reference operator[](size_type off) { return str_[off].c; }

	/**
		subscript nonmutable sequence
		@param off [local::in] offset
	*/
	const_reference operator[](size_type off) const { return str_[off].c; }

	/**
		insert element at end
		@param c [local::in] append character
	*/
	void push_back(cybozu::Char c)
	{
		Box b; b.c = c;
		str_.push_back(b);
	}

	/**
		return pointer to null-terminated nonmutable array
	*/
	const cybozu::Char *c_str() const { return &(str_.c_str()->c); }

	/**
		return pointer to nonmutable array
	*/
	const cybozu::Char *data() const { return &(str_.data()->c); }

	/**
		return length of sequence
	*/
	size_type length() const { return str_.length(); }

	/**
		return length of sequence
	*/
	size_type size() const { return str_.size(); }

	/**
		return maximum possible length of sequence
	*/
	size_type max_size() const { return str_.max_size(); }

	/**
		determine new length, padding with null elements as needed
	*/
	void resize(size_type newSize) { str_.resize(newSize); }

	/**
		determine new length, padding with c elements as needed
		@param newSize [local::in] new length
		@param c [local::in] initial character
	*/
	void resize(size_type newSize, cybozu::Char c)
	{
		Box b; b.c = c;
		str_.resize(newSize, b);
	}

	/**
		return current length of allocated storage
	*/
	size_type capacity() const { return str_.capacity(); }

	/**
		determine new minimum length of allocated storage
		@param newSize [local::in] reserve size
	*/
	void reserve(size_type newSize = 0) { str_.reserve(newSize); }

	/**
		test if sequence is empty
		@return true if empty
	*/
	bool empty() const { return str_.empty(); }

	/**
		copy [off, off + count) to [dest, dest + count)
		@param dest [local::in] destination
		@param count [local::in] count of copy
		@param off [local::in] copy from here
	*/
	size_type copy(cybozu::Char *dest, size_type count, size_type off = 0) const
	{
		return str_.copy((Box*)dest, count, off);
	}

	/**
		exchange contents with rhs
		@param rhs [local::in] swap string
	*/
	void swap(basic_string& rhs) { str_.swap(rhs.str_); }

	/**
		look for rhs beginnng at or after off
		@param rhs [local::in] target
		@param off [local::in] search from here
		@return position
	*/
	size_type find(const basic_string& rhs, size_type off = 0) const
	{
		return str_.find(rhs.str_, off);
	}

	/**
		look for [str, str + count) beginnng at or after off
		@param str [local::in] target
		@param off [local::in] search from here
		@param count [local::in] count of str
	*/
	size_type find(const cybozu::Char *str, size_type off, size_type count) const
	{
		return str_.find((const Box*)str, off, count);
	}

	/**
		look for [str, NUL) beginnng at or after off
		@param str [local::in] target
		@param off [local::in] search from here
	*/
	size_type find(const cybozu::Char *str, size_type off = 0) const
	{
		return str_.find((const Box*)str, off);
	}

	/**
		look for c at or after off
		@param c [local::in] target
		@param off [local::in] search from here
	*/
	size_type find(cybozu::Char c, size_type off = 0) const
	{
		Box b; b.c = c;
		return str_.find(b, off);
	}

	/**
		look for rhs beginning before off
		@param rhs [local::in] target
		@param off [local::in] search from here
	*/
	size_type rfind(const basic_string& rhs, size_type off = npos) const
	{
		return str_.rfind(rhs.str_, off);
	}

	/**
		look for [str, str + count) beginning before off
		@param str [local::in] target
		@param off [local::in] search from here
		@param count [local::in] count of character
	*/
	size_type rfind(const cybozu::Char *str, size_type off, size_type count) const
	{
		return str_.rfind((const Box*)str, off, count);
	}

	/**
		look for [str, NUL) beginning before off
		@param str [local::in] target
		@param off [local::in] search from here
	*/
	size_type rfind(const cybozu::Char *str, size_type off = npos) const
	{
		return str_.rfind((const Box*)str, off);
	}

	/**
		look for c before off
		@param c [local::in] target
		@param off [local::in] search from here
	*/
	size_type rfind(cybozu::Char c, size_type off = npos) const
	{
		Box b; b.c = c;
		return str_.rfind(b, off);
	}

	/**
		look for one of rhs at or after off
		@param rhs [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_first_of(const basic_string& rhs, size_type off = 0) const
	{
		return str_.find_first_of(rhs.str_, off);
	}

	/**
		look for one of [str, str + count) at or after off
		@param str [local::in] target
		@param off [local::in] search from here
		@param count [local::in] count of character
	*/
	size_type find_first_of(const cybozu::Char *str, size_type off, size_type count) const
	{
		return str_.find_first_of((const Box*)str, off, count);
	}

	/**
		look for one of [str, NUL) at or after off
		@param str [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_first_of(const cybozu::Char *str, size_type off = 0) const
	{
		return str_.find_first_of((const Box*)str, off);
	}

	/**
		look for c at or after off
		@param c [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_first_of(cybozu::Char c, size_type off = 0) const
	{
		Box b; b.c = c;
		return str_.find_first_of(b, off);
	}

	/**
		look for one of rhs before off
		@param rhs [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_last_of(const basic_string& rhs, size_type off = npos) const
	{
		return str_.find_last_of(rhs.str_, off);
	}

	/**
		look for one of [str, str + count) before off
		@param str [local::in] target
		@param off [local::in] search from here
		@param count [local::in] count of character
	*/
	size_type find_last_of(const cybozu::Char *str, size_type off, size_type count) const
	{
		return str_.find_last_of((const Box*)str, off, count);
	}

	/**
		look for one of [str, NUL) before off
		@param str [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_last_of(const cybozu::Char *str, size_type off = npos) const
	{
		return str_.find_last_of((const Box*)str, off);
	}

	/**
		look for c before off
		@param c [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_last_of(cybozu::Char c, size_type off = npos) const
	{
		Box b; b.c = c;
		return str_.find_last_of(b, off);
	}

	/**
		look for none of rhs at or after off
		@param rhs [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_first_not_of(const basic_string& rhs, size_type off = 0) const
	{
		return str_.find_first_not_of(rhs.str_, off);
	}

	/**
		look for none of [str, str + count) at or after off
		@param str [local::in] target
		@param off [local::in] search from here
		@param count [local::in] count of character
	*/
	size_type find_first_not_of(const cybozu::Char *str, size_type off, size_type count) const
	{
		return str_.find_first_not_of((const Box*)str, off, count);
	}

	/**
		look for one of [str, NUL) at or after off
		@param str [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_first_not_of(const cybozu::Char *str, size_type off = 0) const
	{
		return str_.find_first_not_of((const Box*)str, off);
	}

	/**
		look for non c at or after off
		@param c [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_first_not_of(cybozu::Char c, size_type off = 0) const
	{
		Box b; b.c = c;
		return str_.find_first_not_of(b, off);
	}

	/**
		look for none of rhs before off
		@param rhs [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_last_not_of(const basic_string& rhs, size_type off = npos) const
	{
		return str_.find_last_not_of(rhs.str_, off);
	}

	/**
		look for none of [str, str + count) before off
		@param str [local::in] target
		@param off [local::in] search from here
		@param count [local::in] count of character
	*/
	size_type find_last_not_of(const cybozu::Char *str, size_type off, size_type count) const
	{
		return str_.find_last_not_of((const Box*)str, off, count);
	}

	/**
		look for none of [str, NUL) before off
		@param str [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_last_not_of(const cybozu::Char *str, size_type off = npos) const
	{
		return str_.find_last_not_of((const Box*)str, off);
	}

	/**
		look for non c before off
		@param c [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_last_not_of(cybozu::Char c, size_type off = npos) const
	{
		Box b; b.c = c;
		return str_.find_last_not_of(b, off);
	}
	/**
		return [off, off + count) as new string
		@param off [local::in] from here
		@param count [local::in] count of substring
	*/
	basic_string substr(size_type off = 0, size_type count = npos) const
	{
		basic_string ret;
		ret.str_ = str_.substr(off, count);
		return ret;
	}
	/**
		compare *this with rhs
		@param rhs [local::in] target
	*/
	int compare(const basic_string& rhs) const
	{
		return str_.compare(rhs.str_);
	}

	/**
		compare [off, off + n) with rhs
		@param off [local::in] from here
		@param n [local::in] count of lhs
		@param rhs [local::in] target
	*/
	int compare(size_type off, size_type n, const basic_string& rhs) const
	{
		return str_.compare(off, n, rhs.str_);
	}

	/**
		compare [off, off + n) with rhs [rhsOff, rhsOff + count)
		@param off [local::in] from here
		@param n [local::in] count of lhs
		@param rhs [local::in] target
		@param rhsOff [local::in] target from here
		@param count [local::in] count of rhs
	*/
	int compare(size_type off, size_type n, const basic_string& rhs, size_type rhsOff, size_type count) const
	{
		return str_.compare(off, n, rhs.str_, rhsOff, count);
	}

	/**
		compare [0, _Mysize) with [str, NUL)
		@param str [local::in] target
	*/
	int compare(const cybozu::Char *str) const
	{
		return str_.compare((const Box*)str);
	}

	/**
		compare [off, off + n) with [str, NUL)
		@param off [local::in] from here
		@param n [local::in] count of lhs
		@param str [local::in] target
	*/
	int compare(size_type off, size_type n, const cybozu::Char *str) const
	{
		return str_.compare(off, n, (const Box*)str);
	}

	/**
		compare [off, off + n) with [str, str + count)
		@param off [local::in] from here
		@param n [local::in] count of lhs
		@param str [local::in] target
		@param count [local::in] count of rhs
	*/
	int compare(size_type off,size_type n, const cybozu::Char *str, size_type count) const
	{
		return str_.compare(off, n, (const Box*)str, count);
	}
	/**
		convert to std::string with UTF-8
	*/
	void toUtf8(std::string& str) const
	{
		for (size_t i = 0, n = str_.size(); i < n; i++) {
			if (!cybozu::string::AppendUtf8(str, str_[i].c)) {
				throw cybozu::Exception("string:toUtf8") << i;
			}
		}
	}
	std::string toUtf8() const
	{
		std::string str;
		toUtf8(str);
		return str;
	}
	/**
		convert to std::string with UTF-16LE
	*/
	void toUtf16(cybozu::String16& str) const
	{
		for (size_t i = 0, n = str_.size(); i < n; i++) {
			if (!cybozu::string::AppendUtf16(str, str_[i].c)) {
				throw cybozu::Exception("string:toUtf16") << i;
			}
		}
	}
	cybozu::String16 toUtf16() const
	{
		cybozu::String16 str;
		toUtf16(str);
		return str;
	}
	/**
		is this valid unicode string?
		@return true correct string
		@return false bad string
	*/
	bool isValid() const
	{
		for (size_t i = 0, n = str_.size(); i < n; i++) {
			if (!cybozu::string::IsValidChar(str_[i].c)) return false;
		}
		return true;
	}
	template<class T>bool operator==(const T& rhs) const { return compare(rhs) == 0; }
	template<class T>bool operator!=(const T& rhs) const { return compare(rhs) != 0; }
	template<class T>bool operator<=(const T& rhs) const { return compare(rhs) <= 0; }
	template<class T>bool operator>=(const T& rhs) const { return compare(rhs) >= 0; }
	template<class T>bool operator<(const T& rhs) const { return compare(rhs) < 0; }
	template<class T>bool operator>(const T& rhs) const { return compare(rhs) > 0; }
private:
	template<class Iterator>
	cybozu::Char getOneChar(Iterator& begin, const Iterator& end)
	{
		return getOneCharSub(begin, end, *begin);
	}
	// dispatch
	template<class Iterator>
	cybozu::Char getOneCharSub(Iterator& begin, const Iterator&, cybozu::Char)
	{
		return *begin++;
	}
	template<class Iterator>
	cybozu::Char getOneCharSub(Iterator& begin, const Iterator& end, char)
	{
		cybozu::Char c;
		if (!cybozu::string::GetCharFromUtf8(&c, begin, end)) {
			throw cybozu::Exception("string:getOneCharSub");
		}
		return c;
	}
	static inline size_t getSize(const cybozu::Char *str)
	{
		size_t len = 0;
		while (str[len]) len++;
		return len;
	}
	inString::iterator cvt(iterator i)
	{
		size_t offset = i - &str_[0].c;
		return str_.begin() + offset;
	}
	iterator cvt(inString::iterator i) const
	{
		return &(i->c);
	}
	inString str_;
};

inline std::istream& getline(std::istream& is, std::basic_string<cybozu::Char>& str, const char delim = '\n')
{
	std::string tmp;
	std::getline(is, tmp, delim);
	str.assign(tmp);
	return is;
}

inline std::istream& operator>>(std::istream& is, std::basic_string<cybozu::Char>& str)
{
	std::string tmp;
	is >> tmp;
	str.assign(tmp);
	return is;
}

inline std::ostream& operator<<(std::ostream& os, const std::basic_string<cybozu::Char>& str)
{
	return os << str.toUtf8();
}

inline bool operator==(const std::string& lhs, const std::basic_string<cybozu::Char>& rhs) { return rhs == lhs; }
inline bool operator!=(const std::string& lhs, const std::basic_string<cybozu::Char>& rhs) { return rhs != lhs; }
inline bool operator<=(const std::string& lhs, const std::basic_string<cybozu::Char>& rhs) { return rhs >= lhs; }
inline bool operator>=(const std::string& lhs, const std::basic_string<cybozu::Char>& rhs) { return rhs <= lhs; }
inline bool operator<(const std::string& lhs, const std::basic_string<cybozu::Char>& rhs) { return rhs > lhs; }
inline bool operator>(const std::string& lhs, const std::basic_string<cybozu::Char>& rhs) { return rhs < lhs; }
#ifdef _MSC_VER
inline bool operator==(const std::wstring& lhs, const std::basic_string<cybozu::Char>& rhs) { return rhs == lhs; }
inline bool operator!=(const std::wstring& lhs, const std::basic_string<cybozu::Char>& rhs) { return rhs != lhs; }
inline bool operator<=(const std::wstring& lhs, const std::basic_string<cybozu::Char>& rhs) { return rhs >= lhs; }
inline bool operator>=(const std::wstring& lhs, const std::basic_string<cybozu::Char>& rhs) { return rhs <= lhs; }
inline bool operator<(const std::wstring& lhs, const std::basic_string<cybozu::Char>& rhs) { return rhs > lhs; }
inline bool operator>(const std::wstring& lhs, const std::basic_string<cybozu::Char>& rhs) { return rhs < lhs; }
#endif

inline std::basic_string<cybozu::Char> operator+(const std::basic_string<cybozu::Char>& lhs, const std::basic_string<cybozu::Char>& rhs) { return std::basic_string<cybozu::Char>(lhs) += rhs; }
inline std::basic_string<cybozu::Char> operator+(const std::basic_string<cybozu::Char>& lhs, const cybozu::Char c) { return std::basic_string<cybozu::Char>(lhs) += c; }
inline std::basic_string<cybozu::Char> operator+(const std::basic_string<cybozu::Char>& lhs, const cybozu::Char* str) { return std::basic_string<cybozu::Char>(lhs) += str; }

} // std

namespace cybozu {

typedef std::basic_string<cybozu::Char> String;

inline bool ConvertUtf16ToUtf8(std::string *out, const cybozu::Char16 *begin, const cybozu::Char16 *end)
{
	out->clear();
	out->reserve((end - begin) * 3);
	while (begin != end) {
		cybozu::Char c;
		if (!string::GetCharFromUtf16(c, begin, end)) return false;
		if (!string::AppendUtf8(*out, c)) return false;
	}
	return true;
}
inline bool ConvertUtf16ToUtf8(std::string *out, const cybozu::String16& in)
{
	return ConvertUtf16ToUtf8(out, &in[0], &in[0] + in.size());
}

inline bool ConvertUtf8ToUtf16(cybozu::String16 *out, const char *begin, const char *end)
{
	out->clear();
	out->reserve((end - begin) / 2);
	while (begin != end) {
		cybozu::Char c;
		if (!string::GetCharFromUtf8(&c, begin, end)) return false;
		if (!string::AppendUtf16(*out, c)) return false;
	}
	return true;
}

inline bool ConvertUtf8ToUtf16(cybozu::String16 *out, const std::string& in)
{
	return ConvertUtf8ToUtf16(out, &in[0], &in[0] + in.size());
}

inline cybozu::String16 ToUtf16(const std::string& in)
{
	cybozu::String16 out;
	if (ConvertUtf8ToUtf16(&out, in)) return out;
	throw cybozu::Exception("string:ToUtf16:bad utf8") << in;
}

inline std::string ToUtf8(const cybozu::String16& in)
{
	std::string out;
	if (ConvertUtf16ToUtf8(&out, in)) return out;
	throw cybozu::Exception("string:ToUtf8:bad utf16");
}

template<class Iterator>
class Utf8refT {
	Iterator begin_;
	Iterator end_;
	bool ignoreBadChar_;
public:
	Utf8refT(Iterator begin, Iterator end, bool ignoreBadChar = false)
		: begin_(begin)
		, end_(end)
		, ignoreBadChar_(ignoreBadChar)
	{
	}
	/*
		get character and seek next pointer
	*/
	bool next(Char *c)
	{
	RETRY:
		if (begin_ == end_) return false;
		bool b = string::GetCharFromUtf8(c, begin_, end_);
		if (b) return true;
		if (ignoreBadChar_) goto RETRY;
		throw cybozu::Exception("string:Utf8ref:getAndNext");
	}
};

struct Utf8ref : Utf8refT<const char*> {
	Utf8ref(const char *begin, const char *end, bool ignoreBadChar = false) : Utf8refT<const char*>(begin, end, ignoreBadChar) {}
	Utf8ref(const char *str, size_t size, bool ignoreBadChar = false) : Utf8refT<const char*>(str, str + size, ignoreBadChar) {}
	explicit Utf8ref(const std::string& str, bool ignoreBadChar = false) : Utf8refT<const char*>(str.c_str(), str.c_str() + str.size(), ignoreBadChar) {}
};

} // cybozu

// specialization for boost::hash
namespace boost {

template<>
struct hash<cybozu::String> : public std::unary_function<cybozu::String, size_t> {
	size_t operator()(const cybozu::String& str) const
	{
		return static_cast<size_t>(cybozu::hash64(str.c_str(), str.size()));
	}
};

} // boost

namespace std { CYBOZU_NAMESPACE_TR1_BEGIN

template<>
struct hash<cybozu::String> : public std::unary_function<cybozu::String, size_t> {
	size_t operator()(const cybozu::String& str) const
	{
		return static_cast<size_t>(cybozu::hash64(str.c_str(), str.size()));
	}
};

CYBOZU_NAMESPACE_TR1_END } // std
