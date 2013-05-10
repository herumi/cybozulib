#pragma once
/**
	@file
	@brief frequency of elements in a sequence
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <assert.h>
#include <vector>
#include <algorithm>
#include <functional>
#include <iostream>
#include <cybozu/exception.hpp>
#include <cybozu/unordered_map.hpp>

namespace cybozu {

namespace freq_local {

template<class T>
union ci {
	T i;
	char c[sizeof(T)];
};

template<class T> void load(T& t, std::istream& is) { t.load(is); }
template<class T> void save(std::ostream& os, const T& t) { t.save(os); }

template<class T>
void load(T *t, size_t n, std::istream& is, const char *msg)
{
	const std::streamsize size = sizeof(T) * n;
	if (!is.read((char*)t, size) || is.gcount() != size) {
		throw cybozu::Exception("Frequency:load") << msg;
	}
}
template<class T>
void save(std::ostream& os, const T *t, size_t n, const char *msg)
{
	if (!os.write((const char*)t, sizeof(T) * n)) {
		throw cybozu::Exception("Frequency:save") << msg;
	}
}

#define CYBOZU_FREQUENCY_DEFINE_LOAD_SAVE(type) \
template<>void load(type& t, std::istream& is) { load(&t, 1, is, #type); } \
template<>void save(std::ostream& os, const type& t) { save(os, &t, 1, #type); }

CYBOZU_FREQUENCY_DEFINE_LOAD_SAVE(char)
CYBOZU_FREQUENCY_DEFINE_LOAD_SAVE(unsigned char)
CYBOZU_FREQUENCY_DEFINE_LOAD_SAVE(int)
CYBOZU_FREQUENCY_DEFINE_LOAD_SAVE(unsigned int)
CYBOZU_FREQUENCY_DEFINE_LOAD_SAVE(long)
CYBOZU_FREQUENCY_DEFINE_LOAD_SAVE(unsigned long)
#ifdef _MSC_VER
CYBOZU_FREQUENCY_DEFINE_LOAD_SAVE(size_t)
#endif

#undef CYBOZU_FREQUENCY_DEFINE_LOAD_SAVE

template<>
void load(std::string& t, std::istream& is)
{
	size_t size;
	load(size, is);
	t.resize(size);
	load(&t[0], size, is, "string");
}
template<>
void save(std::ostream& os, const std::string& t)
{
	save(os, t.size());
	save(os, &t[0], t.size(), "string");
}

template<class Element, class Int = size_t>
class FrequencyVec {
	static const size_t N = size_t(1) << (sizeof(Element) * 8);
	size_t size_;
	Int freqTbl_[N];
	uint8_t char2idx_[N];
	uint8_t idx2char_[N];
	struct Greater {
		const Int *p_;
		explicit Greater(const Int *p) : p_(p) {}
		bool operator()(uint8_t lhs, uint8_t rhs) const
		{
			Int a = p_[lhs];
			Int b = p_[rhs];
			if (a > b) return true;
			if (a < b) return false;
			return a > b;
		}
	};
public:
	typedef Element value_type;
	typedef Int size_type;

	FrequencyVec() : size_(0) {}
	template<class Iter>
	FrequencyVec(Iter begin, Iter end)
	{
		init(begin, end);
	}
	template<class Iter>
	void init(Iter begin, Iter end)
	{
		memset(freqTbl_, 0, sizeof(freqTbl_));
		while (begin != end) {
			freqTbl_[uint8_t(*begin)]++;
			++begin;
		}
		for (size_t i = 0; i < N; i++) idx2char_[i] = uint8_t(i);
		Greater greater(freqTbl_);
		std::sort(idx2char_, idx2char_ + N, greater);
		size_ = 0;
		for (size_t i = 0; i < N; i++) {
			uint8_t c = idx2char_[i];
			char2idx_[c] = (uint8_t)i;
			if (freqTbl_[c]) size_++;
		}
	}
	/*
		element -> freq
	*/
	Int getFrequency(Element e) const { return freqTbl_[uint8_t(e)]; }
	/*
		element -> idx
	*/
	Int getIndex(Element e) const { return char2idx_[uint8_t(e)]; }
	/*
		idx -> element
	*/
	Element getElement(size_t idx) const
	{
//		if (idx >= N) throw cybozu::Exception("Frequency:getElement:bad idx") << idx;
		assert(idx < N);
		return Element(idx2char_[idx]);
	}
	size_t size() const { return size_; }
	void load(std::istream& is)
	{
		freq_local::load(&size_, 1u, is, "size");
		freq_local::load(freqTbl_, N, is, "freqTbl");
		freq_local::load(char2idx_, N, is, "char2idx");
		freq_local::load(idx2char_, N, is, "idx2char");
	}
	void save(std::ostream& os) const
	{
		freq_local::save(os, &size_, 1, "size");
		freq_local::save(os, freqTbl_, N, "freqTbl");
		freq_local::save(os, char2idx_, N, "char2idx");
		freq_local::save(os, idx2char_, N, "idx2char");
	}
};

} // cybozu::freq_local

/*
	count Element
	Element : type of element
	Int : type of counter
*/
template<class Element, class Int = size_t>
class Frequency {
	struct FreqIdx {
		Int freq;
		mutable Int idx;
		void load(std::istream& is)
		{
			freq_local::load(freq, is);
			freq_local::load(idx, is);
		}
		void save(std::ostream& os) const
		{
			freq_local::save(os, freq);
			freq_local::save(os, idx);
		}
	};
	typedef CYBOZU_NAMESPACE_STD::unordered_map<Element, FreqIdx> Map;
	typedef Element value_type;
	typedef Int size_type;
	typedef std::vector<typename Map::const_iterator> Idx2Ref;
	static inline bool greater(typename Map::const_iterator i, typename Map::const_iterator j)
	{
		const Int a = i->second.freq;
		const Int b = j->second.freq;
		if (a > b) return true;
		if (a < b) return false;
		return i->first > j->first;
	}
	Map m_;
	Idx2Ref idx2ref_;
	void initIdx2Ref()
	{
		idx2ref_.resize(m_.size());
		size_t pos = 0;
		for (typename Map::const_iterator i = m_.begin(), ie = m_.end(); i != ie; ++i) {
			idx2ref_[pos++] = i;
		}
		std::sort(idx2ref_.begin(), idx2ref_.end(), greater);
	}
public:
	Frequency(){}
	template<class Iter>
	Frequency(Iter begin, Iter end)
	{
		init(begin, end);
	}
	template<class Iter>
	void init(Iter begin, Iter end)
	{
		while (begin != end) {
			m_[*begin].freq++;
			++begin;
		}
		initIdx2Ref();
		for (size_t i = 0, ie = idx2ref_.size(); i < ie; i++) {
			idx2ref_[i]->second.idx = (Int)i;
		}
	}
	/*
		element -> freq
	*/
	Int getFrequency(const Element& e) const
	{
		typename Map::const_iterator i = m_.find(e);
		return (i != m_.end()) ? i->second.freq : 0;
	}
	/*
		element -> idx
	*/
	Int getIndex(const Element& e) const
	{
		typename Map::const_iterator i = m_.find(e);
		if (i == m_.end()) throw cybozu::Exception("Frequency:getIndex:not found") << e;
		return i->second.idx;
	}
	/*
		idx -> element
	*/
	const Element& getElement(size_t idx) const
	{
		if (idx >= idx2ref_.size()) throw cybozu::Exception("Frequency:getElement:bad idx") << idx;
		return idx2ref_[idx]->first;
	}
	size_t size() const { return idx2ref_.size(); }
	void load(std::istream& is)
	{
		size_t size;
		freq_local::load(size, is);
		for (size_t i = 0; i < size; i++) {
			typename Map::key_type k;
			freq_local::load(k, is);
			FreqIdx freqIdx;
			freq_local::load(freqIdx, is);
			m_.insert(typename Map::value_type(k, freqIdx));
		}
		initIdx2Ref();
	}
	void save(std::ostream& os) const
	{
		freq_local::save(os, m_.size());
		for (typename Map::const_iterator i = m_.begin(), ie = m_.end(); i != ie; ++i) {
			freq_local::save(os, i->first);
			freq_local::save(os, i->second);
		}
	}
};

template<class Int>
struct Frequency<uint8_t, Int> : freq_local::FrequencyVec<uint8_t, Int> {
	Frequency() {}
	template<class Iterator>
	Frequency(Iterator begin, Iterator end) : freq_local::FrequencyVec<uint8_t, Int>(begin, end) {}
};
template<class Int>
struct Frequency<char, Int> : freq_local::FrequencyVec<char, Int> {
	Frequency() {}
	template<class Iterator>
	Frequency(Iterator begin, Iterator end) : freq_local::FrequencyVec<char, Int>(begin, end) {}
};

} // cybozu
