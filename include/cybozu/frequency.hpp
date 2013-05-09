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
#include <cybozu/exception.hpp>
#include <cybozu/unordered_map.hpp>

namespace cybozu {

namespace freq_local {

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
			char2idx_[c] = (Int)i;
			if (freqTbl_[c]) size_++;
		}
	}
	/*
		element -> freq
	*/
	Int getFreq(Element e) const { return freqTbl_[uint8_t(e)]; }
	/*
		element -> idx
	*/
	Int getIdx(Element e) const { return char2idx_[uint8_t(e)]; }
	/*
		idx -> element
	*/
	Element getElem(size_t idx) const
	{
//		if (idx >= N) throw cybozu::Exception("Frequency:getElem:bad idx") << idx;
		assert(idx < N);
		return Element(idx2char_[idx]);
	}
	size_t size() const { return size_; }
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
		idx2ref_.resize(m_.size());
		size_t pos = 0;
		for (typename Map::const_iterator i = m_.begin(), ie = m_.end(); i != ie; ++i) {
			idx2ref_[pos++] = i;
		}
		std::sort(idx2ref_.begin(), idx2ref_.end(), greater);
		for (size_t i = 0, ie = idx2ref_.size(); i < ie; i++) {
			idx2ref_[i]->second.idx = (Int)i;
		}
	}
	/*
		element -> freq
	*/
	Int getFreq(const Element& e) const
	{
		typename Map::const_iterator i = m_.find(e);
		return (i != m_.end()) ? i->second.freq : 0;
	}
	/*
		element -> idx
	*/
	Int getIdx(const Element& e) const
	{
		typename Map::const_iterator i = m_.find(e);
		if (i == m_.end()) throw cybozu::Exception("Frequency:getIdx:not found") << e;
		return i->second.idx;
	}
	/*
		idx -> element
	*/
	const Element& getElem(size_t idx) const
	{
		if (idx >= idx2ref_.size()) throw cybozu::Exception("Frequency:getElem:bad idx") << idx;
		return idx2ref_[idx]->first;
	}
	size_t size() const { return idx2ref_.size(); }
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
