#pragma once
/**
	@file
	@brief string operation for cybozu::String

	Copyright (C) 2008-2012 Cybozu Labs, Inc., all rights reserved.

	@note
	modifying functions are almost the following type:
	void function(String& out, String& in, bool doAppend = false);
	if doAppend is true then the function will append string to <out>.

*/

#include <algorithm>
#include <cybozu/string.hpp>

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4389) // compare signed and unsigned
#endif

namespace cybozu {

namespace string {

static const bool DoAppend = true;

} // string

/**
	verify whether c is a space or 0xfeff(BOM)
	the definition of space is
	http://unicode.org/Public/UNIDATA/PropList.txt
	0009..000D	; White_Space # Cc   [5] <control-0009>..<control-000D>
	0020		  ; White_Space # Zs	   SPACE
	0085		  ; White_Space # Cc	   <control-0085>
	00A0		  ; White_Space # Zs	   NO-BREAK SPACE
	1680		  ; White_Space # Zs	   OGHAM SPACE MARK
	180E		  ; White_Space # Zs	   MONGOLIAN VOWEL SEPARATOR
	2000..200A	; White_Space # Zs  [11] EN QUAD..HAIR SPACE
	2028		  ; White_Space # Zl	   LINE SEPARATOR
	2029		  ; White_Space # Zp	   PARAGRAPH SEPARATOR
	202F		  ; White_Space # Zs	   NARROW NO-BREAK SPACE
	205F		  ; White_Space # Zs	   MEDIUM MATHEMATICAL SPACE
	3000		  ; White_Space # Zs	   IDEOGRAPHIC SPACE
	+
	0xfeff // zero width no-break space
*/
inline bool IsSpace(Char c)
{
	if (c == 0x20 || c == 0x3000) return true;
	if (unsigned(c - 0x09) <= 0x0d - 0x09) return true;
	if (c == 0x85 || c == 0xa0) return true;
	if (c < 0x1680) return false;
	if (c == 0x1680 || c == 0x180e) return true;
	if (unsigned(c - 0x2000) <= 0x200a - 0x2000) return true;
	if (unsigned(c - 0x2028) <= 0x2029 - 0x2028) return true;
	if (c == 0x202f || c == 0x205f || c == 0xfeff) return true;
	return false;
}

/*
	c is space or tab(Don't modify this condition)
*/
inline bool IsSpace(char c)
{
	if (c == ' ' || c == '\t') return true;
	return false;
}

template<typename Iterator>
inline Iterator* SkipSpace(Iterator *begin, Iterator *end)
{
	while (begin < end) {
		if (!cybozu::IsSpace(*begin)) break;
		++begin;
	}
	return begin;
}

namespace string_local {

/**
	get trimed position [begin, end)
	@param begin [in] begin of string
	@param end [inout] end of string
	return new begin position
*/
template<typename Iterator>
inline Iterator* GetTrimPosition(Iterator *begin, Iterator *&end)
{
	begin = cybozu::SkipSpace(begin, end);
	while (begin < end) {
		if (!cybozu::IsSpace(end[-1])) break;
		--end;
	}
	return begin;
}

template<class StringT1, class StringT2, class Func>
void ChangeCase(StringT1& out, const StringT2& in, Func f, bool doAppend)
{
	size_t offset = doAppend ? out.size() : 0;
	out.resize(offset + in.size());
	std::transform(in.begin(), in.end(), out.begin() + offset, f);
}

template<class CharT>
inline CharT ToLowerC(CharT c)
{
	return ('A' <= c && c <= 'Z') ? c - 'A' + 'a' : c;
}

template<class CharT>
inline CharT ToUpperC(CharT c)
{
	return ('a' <= c && c <= 'z') ? c - 'a' + 'A' : c;
}

} // string_local
/**
	trim space
	@param str [inout] string to be trimed
*/
template<class CharT, class Traits, class Alloc, template<class CharT, class Traits, class Alloc>class StringT>
inline void Trim(StringT<CharT, Traits, Alloc>& str)
{
	if (str.empty()) return;
	CharT *begin = &str[0];
	CharT *end = begin + str.size();
	CharT *newBegin = cybozu::string_local::GetTrimPosition(begin, end);
	size_t size = end - newBegin;
	if (begin != newBegin) {
		for (size_t i = 0; i < size; i++) {
			*begin++ = *newBegin++;
		}
	}
	str.resize(size);
}

template<class CharT, class Traits, class Alloc, template<class CharT, class Traits, class Alloc>class StringT>
inline StringT<CharT, Traits, Alloc> TrimCopy(const StringT<CharT, Traits, Alloc>& str)
{
	if (str.empty()) return "";
	const CharT *begin = &str[0];
	const CharT *end = begin + str.size();
	const CharT *newBegin = cybozu::string_local::GetTrimPosition(begin, end);
	return StringT<CharT, Traits, Alloc>(newBegin, end);
}

template<class StringT1, class StringT2>
void ToLower(StringT1& out, const StringT2& in, bool doAppend = false)
{
	string_local::ChangeCase(out, in, cybozu::string_local::ToLowerC<typename StringT1::value_type>, doAppend);
}

template<class StringT1, class StringT2>
void ToUpper(StringT1& out, const StringT2& in, bool doAppend = false)
{
	string_local::ChangeCase(out, in, cybozu::string_local::ToUpperC<typename StringT1::value_type>, doAppend);
}

template<class StringT>
void ToLower(StringT& str)
{
	ToLower(str, str);
}

template<class StringT>
void ToUpper(StringT& str)
{
	ToUpper(str, str);
}

/**
	case insensitive compare function
	not depend on locale, not depend on compiler
*/
template<class CharT>
int CaseCompare(const CharT *lhs, size_t lhsSize, const CharT *rhs, size_t rhsSize)
{
	size_t n = std::min(lhsSize, rhsSize);
	for (size_t i = 0; i < n; i++) {
		CharT cR = cybozu::string_local::ToLowerC(lhs[i]);
		CharT cL = cybozu::string_local::ToLowerC(rhs[i]);
		if (cR < cL) return -1;
		if (cR > cL) return 1;
	}
	if (lhsSize < rhsSize) return -1;
	if (lhsSize > rhsSize) return 1;
	return 0;
}

template<class CharT1, class CharT2>
bool CaseEqual(const CharT1 *lhs, size_t lhsSize, const CharT2 *rhs, size_t rhsSize)
{
	if (lhsSize != rhsSize) return false;
	for (size_t i = 0; i < lhsSize; i++) {
		CharT1 cR = cybozu::string_local::ToLowerC(lhs[i]);
		CharT2 cL = cybozu::string_local::ToLowerC(rhs[i]);
		if (cR != cL) return false;
	}
	return true;
}

/**
	case insensitive compare lhs with rhs
	@param lhs [in] left side string
	@param rhs [in] right side string
	@retval  1 if lhs > rhs
	@retval  0 if lhs == rhs
	@retval -1 if lhs < rhs
*/
template<class CharT, class Traits, class Alloc, template<class CharT, class Traits, class Alloc>class StringT>
int CaseCompare(const StringT<CharT, Traits, Alloc>& lhs, const StringT<CharT, Traits, Alloc>& rhs)
{
	return cybozu::CaseCompare(&lhs[0], lhs.size(), &rhs[0], rhs.size());
}

/**
	whether lhs is equal to rhs with case insensitive
	@param lhs [in] left side string
	@param rhs [in] right side string
*/
template<class CharT, class Traits, class Alloc, template<class CharT, class Traits, class Alloc>class StringT>
bool CaseEqual(const StringT<CharT, Traits, Alloc>& lhs, const StringT<CharT, Traits, Alloc>& rhs)
{
	return cybozu::CaseEqual(&lhs[0], lhs.size(), &rhs[0], rhs.size());
}

template<class CharT, class Traits, class Alloc, template<class CharT, class Traits, class Alloc>class StringT>
bool CaseEqual(const StringT<CharT, Traits, Alloc>& lhs, const char *rhs)
{
	return cybozu::CaseEqual(&lhs[0], lhs.size(), rhs, strlen(rhs));
}

/**
	find target in [begin, end) with case insensitive
	@param begin [in] begin of string
	@param end [in] end of string
	@param targetBegin [in] begin of *small* target
	@param targetEnd [in] end of *small* target string(if NULL then use null terminate string)
	@retval !0 if found
	@retval 0 if not found
*/
template<class CharT>
const CharT *CaseFind(const CharT *begin, const CharT *end, const char *targetBegin, const char *targetEnd = 0)
{
	const size_t n = targetEnd ? targetEnd - targetBegin : strlen(targetBegin);
	std::basic_string<CharT> t(targetBegin, n);
	ToLower(t);
	while (begin + n <= end) {
		bool found = true;
		for (size_t i = 0; i < n; i++) {
			if (string_local::ToLowerC(begin[i]) != t[i]) {
				found = false;
				break;
			}
		}
		if (found) return begin;
		begin++;
	}
	return 0;
}

template<class CharT, class Traits, class Alloc, template<class CharT, class Traits, class Alloc>class StringT>
size_t CaseFind(const StringT<CharT, Traits, Alloc>& str, const char *targetBegin, const char *targetEnd = 0)
{
	if (!str.empty()) {
		const CharT *p = CaseFind(&str[0], &str[0] + str.size(), targetBegin, targetEnd);
		if (p) {
			return p - &str[0];
		}
	}
	return std::string::npos;
}

/**
	whether lhs is equal to rhs[0..N) with case insensitive
	@param lhs [in] left side string
	@param rhs [in] right side string
*/
template<class CharT, class Traits, class Alloc, template<class CharT, class Traits, class Alloc>class StringT, size_t N>
bool CaseEqualStartWith(const StringT<CharT, Traits, Alloc>& lhs, const char (&rhs)[N])
{
	if (lhs.size() < N - 1) return false;
	return cybozu::CaseEqual(&lhs[0], N - 1, rhs, N - 1);
}

/**
	split inStr into out at splitChar
	maxNum is maximum number of split str
	out must have clear() and push_back()
*/
template<class Out, class CharT, class Traits, class Alloc, template<class CharT, class Traits, class Alloc>class StringT>
size_t Split(Out& out, const StringT<CharT, Traits, Alloc>& inStr, CharT splitChar = ',', size_t maxNum = 0x7fffffff)
{
	typedef StringT<CharT, Traits, Alloc> String;
	size_t splitNum = 0;
	size_t cur = 0;
	out.clear();
	for (;;) {
		size_t pos = inStr.find_first_of(splitChar, cur);
		if (pos == String::npos || splitNum == maxNum - 1) {
			out.push_back(String(&inStr[cur], &inStr[0] + inStr.size()));
			splitNum++;
			break;
		}
		out.push_back(String(&inStr[cur], &inStr[pos]));
		cur = pos + 1;
		splitNum++;
	}
	return splitNum;
}


} // cybozu

#ifdef _MSC_VER
	#pragma warning(pop)
#endif
