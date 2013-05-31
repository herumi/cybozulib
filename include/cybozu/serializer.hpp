#pragma once
/**
	@file
	@brief serializer for vector, list, map and so on

	Copyright (C) Cybozu Labs, Inc., all rights reserved.
*/
#include <cybozu/string.hpp>
#include <cybozu/stream.hpp>

namespace cybozu {

namespace serializer_local {

template<class T>
union ci {
	T i;
	char c[sizeof(T)];
};

} // serializer_local

template<class InputStream, class T>
void loadRange(T *p, size_t num, InputStream& is, const char *msg = "")
{
	cybozu::InputStreamTag<InputStream>::read(is, (char*)p, num * sizeof(T), msg);
}

template<class OutputStream, class T>
void saveRange(OutputStream& os, const T *p, size_t num, const char *msg = "")
{
	cybozu::OutputStreamTag<OutputStream>::write(os, (const char*)p, num * sizeof(T), msg);
}

template<class InputStream, class T>
void loadPod(T& x, InputStream& is, const char *msg = "")
{
	serializer_local::ci<T> ci;
	loadRange(ci.c, sizeof(ci.c), is, msg);
	x = ci.i;
}

template<class OutputStream, class T>
void savePod(OutputStream& os, const T& x, const char *msg = "")
{
	serializer_local::ci<T> ci;
	ci.i = x;
	saveRange(os, ci.c, sizeof(ci.c), msg);
}

template<class InputStream, class T>
void load(T& x, InputStream& is, const char *msg = "");

template<class OutputStream, class T>
void save(OutputStream& os, const T& x, const char *msg = "");

#define CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(type) \
template<class InputStream>void load(type& x, InputStream& is, const char *msg = "") { loadPod(x, is, msg); } \
template<class OutputStream>void save(OutputStream& os, type x, const char *msg = "") { savePod(os, x, msg); }

CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(bool)
CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(char)
CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(short)
CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(int)
CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(long)
CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(long long)
CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(unsigned char)
CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(unsigned short)
CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(unsigned int)
CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(unsigned long)
CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(unsigned long long)
CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(float)
CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER(double)

#undef CYBOZU_SERIALIZER_MAKE_POD_SERIALIZER

template<class V, class InputStream>
void loadPodVec(V& v, InputStream& is, const char *msg = "")
{
	size_t size;
	load(size, is, "loadPodVec:size");
	v.resize(size);
	loadRange(&v[0], size, is, msg);
}

template<class V, class OutputStream>
void savePodVec(OutputStream& os, const V& v, const char *msg = "")
{
	save(os, v.size(), "savePodVec");
	saveRange(os, &v[0], v.size(), msg);
}

template<class InputStream>
void load(std::string& str, InputStream& is, const char *msg = "")
{
	loadPodVec(str, is, msg);
}

template<class OutputStream>
void save(OutputStream& os, const std::string& str, const char *msg = "")
{
	savePodVec(os, str, msg);
}

template<class InputStream>
void load(cybozu::String& str, InputStream& is, const char *msg = "")
{
	loadPodVec(str, is, msg);
}

template<class OutputStream>
void save(OutputStream& os, const cybozu::String& str, const char *msg = "")
{
	savePodVec(os, str, msg);
}

// for vector, list
template<class InputStream, class T, class Alloc, template<class T_, class Alloc_>class Container>
void load(Container<T, Alloc>& x, InputStream& is, const char *msg = "")
{
	size_t size;
	load(size, is, "load(Container<T, Alloc>):size");
	for (size_t i = 0; i < size; i++) {
		x.push_back(T());
		T& t = x.back();
		load(t, is, msg);
	}
}

template<class OutputStream, class T, class Alloc, template<class T_, class Alloc_>class Container>
void save(OutputStream& os, const Container<T, Alloc>& x, const char *msg = "")
{
	typedef Container<T, Alloc> V;
	save(os, x.size(), "save(Container<T, Alloc>):size");
	for (typename V::const_iterator i = x.begin(), end = x.end(); i != end; ++i) {
		save(os, *i, msg);
	}
}

// for set
template<class InputStream, class K, class Pred, class Alloc, template<class K_, class Pred_, class Alloc_>class Container>
void load(Container<K, Pred, Alloc>& x, InputStream& is, const char *msg = "")
{
	size_t size;
	load(size, is, "load(Container<K, Pred, Alloc>):size");
	for (size_t i = 0; i < size; i++) {
		K t;
		load(t, is, msg);
		x.insert(t);
	}
}

template<class OutputStream, class K, class Pred, class Alloc, template<class K_, class Pred_, class Alloc_>class Container>
void save(OutputStream& os, const Container<K, Pred, Alloc>& x, const char *msg = "")
{
	typedef Container<K, Pred, Alloc> Set;
	save(os, x.size(), "save(Container<K, Pred, Alloc>):size");
	for (typename Set::const_iterator i = x.begin(), end = x.end(); i != end; ++i) {
		save(os, *i, msg);
	}
}

// for map
template<class InputStream, class K, class V, class Pred, class Alloc, template<class K_, class V_, class Pred_, class Alloc_>class Container>
void load(Container<K, V, Pred, Alloc>& x, InputStream& is, const char *msg = "")
{
	typedef Container<K, V, Pred, Alloc> Map;
	size_t size;
	load(size, is, "load(Container<K, V, Pred, Alloc>):size");
	for (size_t i = 0; i < size; i++) {
		std::pair<typename Map::key_type, typename Map::mapped_type> vt;
		load(vt.first, is, msg);
		load(vt.second, is, msg);
		x.insert(vt);
	}
}

template<class OutputStream, class K, class V, class Pred, class Alloc, template<class K_, class V_, class Pred_, class Alloc_>class Container>
void save(OutputStream& os, const Container<K, V, Pred, Alloc>& x, const char *msg = "")
{
	typedef Container<K, V, Pred, Alloc> Map;
	save(os, x.size(), "save(Container<K, V, Pred, Alloc>):size");
	for (typename Map::const_iterator i = x.begin(), end = x.end(); i != end; ++i) {
		save(os, i->first, msg);
		save(os, i->second, msg);
	}
}

// unordered_map
template<class InputStream, class K, class V, class Hash, class Pred, class Alloc, template<class K_, class V_, class Hash_, class Pred_, class Alloc_>class Container>
void load(Container<K, V, Hash, Pred, Alloc>& x, InputStream& is, const char *msg = "")
{
	typedef Container<K, V, Hash, Pred, Alloc> Map;
	size_t size;
	load(size, is, "load(Container<K, V, Hash, Pred, Alloc>):size");
	for (size_t i = 0; i < size; i++) {
		std::pair<typename Map::key_type, typename Map::mapped_type> vt;
		load(vt.first, is, msg);
		load(vt.second, is, msg);
		x.insert(vt);
	}
}

template<class OutputStream, class K, class V, class Hash, class Pred, class Alloc, template<class K_, class V_, class Hash_, class Pred_, class Alloc_>class Container>
void save(OutputStream& os, const Container<K, V, Hash, Pred, Alloc>& x, const char *msg = "")
{
	typedef Container<K, V, Hash, Pred, Alloc> Map;
	save(os, x.size(), "save(Container<K, V, Hash, Pred, Alloc>):size");
	for (typename Map::const_iterator i = x.begin(), end = x.end(); i != end; ++i) {
		save(os, i->first, msg);
		save(os, i->second, msg);
	}
}

} // cybozu
