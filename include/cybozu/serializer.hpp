#pragma once
/**
	@file
	@brief serializer for vector, list, map and so on

	Copyright (C) 2010-2012 Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include <vector>
#include <list>
#include <set>
#include <cybozu/atoi.hpp>

namespace cybozu {

struct SerializerException : public cybozu::Exception {
	SerializerException() : cybozu::Exception("serializer") { }
};

template<class OutputStream, class T>
void serialize(OutputStream& os, const T& x)
{
	x.serialize(os);
}

template<class InputStream, class T>
void deserialize(InputStream& is, T& x)
{
	x.deserialize(is);
}

// specialization of serialize and deserialize

namespace serializer_detail {

const char sep = ',';
const char escape = '\\';
const char LF = '\n';

// ignore character in reading
inline bool ignoreChar(char c) { return c == '\n' || c == '\r'; }

template<class OutputStream>
void write(OutputStream& os, const char *str, size_t size)
{
	if ((size_t)os.write(str, size) != size) {
		cybozu::SerializerException e;
		e << "write" << cybozu::exception::makeString(str, size);
		throw e;
	}
}

template<class OutputStream>
void write(OutputStream& os, const std::string& str)
{
	write(os, &str[0], str.size());
}

template<class OutputStream>
void write(OutputStream& os, char c)
{
	write(os, &c, 1U);
}

template<class InputStream>
void read(InputStream& is, char *str, size_t size)
{
	if ((size_t)is.read(str, size) != size) {
		cybozu::SerializerException e;
		e << "read" << cybozu::exception::makeString(str, size);
		throw e;
	}
}

template<class InputStream>
size_t readWithoutEscape(InputStream& is, char *str, size_t size)
{
	size_t i = 0;
	while (i < size) {
		char c;
		if (is.read(&c, 1) != 1) break;
		if (i == 0 && ignoreChar(c)) continue;
		if (c == sep) return i;
		str[i++] = c;
	}
	cybozu::SerializerException e;
	e << "readWithoutEscape" << "no comma" << cybozu::exception::makeString(str, i);
	throw e;
}

template<class OutputStream, class V>
void serialize1(OutputStream& os, const V& x)
{
	serialize(os, x.size());
	for (typename V::const_iterator i = x.begin(), end = x.end(); i != end; ++i) {
		serialize(os, *i);
	}
}

} // cybozu::serializer_detail

// specialization integer

#define CYBOZU_NLP_SERIALIZE_INTEGER(TYPE) \
template<class OutputStream> \
void serialize(OutputStream& os, const TYPE& x) \
{ \
	serializer_detail::write(os, cybozu::itoa(x)); \
	serializer_detail::write(os, serializer_detail::sep); \
} \
template<class InputStream> \
void deserialize(InputStream& is, TYPE& x) \
{ \
	const size_t maxLen = 22; \
	char str[maxLen]; \
	size_t readSize = serializer_detail::readWithoutEscape(is, str, maxLen); \
	x = cybozu::atoi(str, readSize); \
}

CYBOZU_NLP_SERIALIZE_INTEGER(int)
CYBOZU_NLP_SERIALIZE_INTEGER(unsigned int)
CYBOZU_NLP_SERIALIZE_INTEGER(int64_t)
CYBOZU_NLP_SERIALIZE_INTEGER(uint64_t)

#undef CYBOZU_NLP_SERIALIZE_INTEGER

template<class OutputStream>
void serialize(OutputStream& os, const bool& x)
{
	serializer_detail::write(os, cybozu::itoa(x ? 1 : 0));
	serializer_detail::write(os, serializer_detail::sep);
}
template<class InputStream>
void deserialize(InputStream& is, bool& x)
{
	char c;
	serializer_detail::read(is, &c, 1);
	x = c == '1';
}

#define CYBOZU_NLP_SERIALIZE_FLOAT(Float, Int) \
template<class OutputStream> \
void serialize(OutputStream& os, const Float& x) \
{ \
	std::string str; \
	union { \
		Int i; \
		Float f; \
	} u; \
	u.f = x; \
	cybozu::itohex(str, u.i, false); \
	serializer_detail::write(os, str); \
	serializer_detail::write(os, serializer_detail::sep); \
} \
template<class InputStream> \
void deserialize(InputStream& is, Float& x) \
{ \
	const size_t size = sizeof(Float) * 2 + 1; \
	char str[size]; \
	serializer_detail::read(is, str, size); \
	if (str[size - 1] != serializer_detail::sep) { \
		cybozu::SerializerException e; \
		e << "deserialize" << "no comma" << std::string(str, size); \
		throw e; \
	} \
	union { \
		Int i; \
		Float f; \
	} u; \
	u.i = cybozu::hextoi(str, size - 1); \
	x = u.f; \
}

CYBOZU_NLP_SERIALIZE_FLOAT(float, unsigned int)
CYBOZU_NLP_SERIALIZE_FLOAT(double, uint64_t)

#undef CYBOZU_NLP_SERIALIZE_FLOAT

template<class OutputStream>
void serialize(OutputStream& os, const std::string& str)
{
	char buf[2] = { '\\', 0 };
	for (size_t i = 0, n = str.size(); i < n; i++) {
		char c = str[i];
		switch (c) {
		case '\n':
			buf[1] = 'n';
			break;
		case '\r':
			buf[1] = 'r';
			break;
		case '\\':
			buf[1] = '\\';
			break;
		case ',':
			buf[1] = ',';
			break;
		default:
			serializer_detail::write(os, c);
			continue;
		}
		serializer_detail::write(os, buf, 2);
	}
	serializer_detail::write(os, serializer_detail::sep);
}

/*
	recover
	\ + n => \n
	\ + r => \r
	\ + \ => \
	\ + , => ,
*/
template<class InputStream>
void deserialize(InputStream& is, std::string& str)
{
	str.clear();
	for (;;) {
		char c;
		if (is.read(&c, 1) != 1) {
			cybozu::SerializerException e;
			e << "deserialize" << "no separator" << str;
			throw e;
		}
		if (str.empty() && serializer_detail::ignoreChar(c)) continue;
		if (c == serializer_detail::sep) return;
		if (c == serializer_detail::escape) {
			if (is.read(&c, 1) != 1) {
				cybozu::SerializerException e;
				e << "deserialize" << "escape not terminate" << str;
				throw e;
			}
			switch (c) {
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case '\\':
				c = '\\';
				break;
			case ',':
				c = ',';
				break;
			default:
				{
					cybozu::SerializerException e;
					e << "deserialize" << "bad escape" << c;
					throw e;
				}
			}
		}
		str += c;
	}
}

// for vector, list
template<class OutputStream, class T, class Alloc, template<class T, class Alloc>class Container>
void serialize(OutputStream& os, const Container<T, Alloc>& x)
{
	typedef Container<T, Alloc> V;
	serialize(os, x.size());
	for (typename V::const_iterator i = x.begin(), end = x.end(); i != end; ++i) {
		serialize(os, *i);
	}
}

template<class InputStream, class T, class Alloc, template<class T, class Alloc>class Container>
void deserialize(InputStream& is, Container<T, Alloc>& x)
{
	size_t len;
	deserialize(is, len);
	for (size_t i = 0; i < len; i++) {
		x.push_back(T());
		T& t = x.back();
		deserialize(is, t);
	}
}

// set
template<class OutputStream, class K, class Pred, class Alloc, template<class K, class Pred, class Alloc>class Container>
void serialize(OutputStream& os, const Container<K, Pred, Alloc>& x)
{
	typedef Container<K, Pred, Alloc> Set;
	serialize(os, x.size());
	for (typename Set::const_iterator i = x.begin(), end = x.end(); i != end; ++i) {
		serialize(os, *i);
	}
}

template<class InputStream, class K, class Pred, class Alloc, template<class K, class Pred, class Alloc>class Container>
void deserialize(InputStream& is, Container<K, Pred, Alloc>& x)
{
	size_t len;
	deserialize(is, len);
	for (size_t i = 0; i < len; i++) {
		K t;
		deserialize(is, t);
		x.insert(t);
	}
}

// map
template<class OutputStream, class K, class V, class Pred, class Alloc, template<class K, class V, class Pred, class Alloc>class Container>
void serialize(OutputStream& os, const Container<K, V, Pred, Alloc>& x)
{
	typedef Container<K, V, Pred, Alloc> Map;
	serialize(os, x.size());
	for (typename Map::const_iterator i = x.begin(), end = x.end(); i != end; ++i) {
		serialize(os, i->first);
		serialize(os, i->second);
	}
}

template<class InputStream, class K, class V, class Pred, class Alloc, template<class K, class V, class Pred, class Alloc>class Container>
void deserialize(InputStream& is, Container<K, V, Pred, Alloc>& x)
{
	typedef Container<K, V, Pred, Alloc> Map;
	size_t len;
	deserialize(is, len);
	for (size_t i = 0; i < len; i++) {
		std::pair<typename Map::key_type, typename Map::mapped_type> vt;
		deserialize(is, vt.first);
		deserialize(is, vt.second);
		x.insert(vt);
	}
}

// unordered_map
template<class OutputStream, class K, class V, class Hash, class Pred, class Alloc, template<class K, class V, class Hash, class Pred, class Alloc>class Container>
void serialize(OutputStream& os, const Container<K, V, Hash, Pred, Alloc>& x)
{
	typedef Container<K, V, Hash, Pred, Alloc> Map;
	serialize(os, x.size());
	for (typename Map::const_iterator i = x.begin(), end = x.end(); i != end; ++i) {
		serialize(os, i->first);
		serialize(os, i->second);
	}
}

template<class InputStream, class K, class V, class Hash, class Pred, class Alloc, template<class K, class V, class Hash, class Pred, class Alloc>class Container>
void deserialize(InputStream& is, Container<K, V, Hash, Pred, Alloc>& x)
{
	typedef Container<K, V, Hash, Pred, Alloc> Map;
	size_t len;
	deserialize(is, len);
	for (size_t i = 0; i < len; i++) {
		std::pair<typename Map::key_type, typename Map::mapped_type> vt;
		deserialize(is, vt.first);
		deserialize(is, vt.second);
		x.insert(vt);
	}
}

} // cybozu
