#pragma once
/**
	@file
	@brief succinct vector
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <assert.h>
#include <vector>
#include <cybozu/exception.hpp>
#include <cybozu/bit_operation.hpp>
#include <cybozu/select8.hpp>
#include <iosfwd>

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4127)
#endif

namespace cybozu {

const uint64_t NotFound = ~uint64_t(0);

namespace sucvector_util {

inline uint32_t rank64(uint64_t v, size_t i)
{
    return cybozu::popcnt<uint64_t>(v & cybozu::makeBitMask64(i));
}

template<class T>
T getBlockNum(T x, T block)
{
	return (x + block - 1) / block;
}

inline uint32_t select64(uint64_t v, size_t r)
{
	assert(r <= 64);
	if (r > popcnt(v)) return 64;
	uint32_t pos = 0;
	uint32_t c = popcnt(uint32_t(v));
	if (r > c) {
		r -= c;
		pos = 32;
		v >>= 32;
	}
	c = popcnt<uint32_t>(uint16_t(v));
	if (r > c) {
		r -= c;
		pos += 16;
		v >>= 16;
	}
	c = popcnt<uint32_t>(uint8_t(v));
	if (r > c) {
		r -= c;
		pos += 8;
		v >>= 8;
	}
	if (r == 8 && uint8_t(v) == 0xff) return pos + 7;
	assert(r <= 8);
	c = cybozu::select8_util::select8(uint8_t(v), r);
	return pos + c;
}

union ci {
	uint64_t i;
	char c[8];
};

inline uint64_t load64bit(std::istream& is, const char *msg)
{
	ci ci;
	if (is.read(ci.c, sizeof(ci.c)) && is.gcount() == sizeof(ci.c)) {
		return ci.i;
	}
	throw cybozu::Exception("sucvector_util:load64bit") << msg;
}

inline void save64bit(std::ostream& os, uint64_t val, const char *msg)
{
	ci ci;
	ci.i = val;
	if (!os.write(ci.c, sizeof(ci.c))) {
		throw cybozu::Exception("sucvector_util:save64bit") << msg;
	}
}

template<class V>
void loadVec(V& v, std::istream& is, const char *msg)
{
	const uint64_t size64 = sucvector_util::load64bit(is, msg);
	assert(size64 <= ~size_t(0));
	const size_t size = size_t(size64);
	v.resize(size);
	if (is.read(cybozu::cast<char*>(&v[0]), size * sizeof(v[0]))) return;
	throw cybozu::Exception("sucvector_util:loadVec") << msg;
}

template<class V>
void saveVec(std::ostream& os, const V& v, const char *msg)
{
	save64bit(os, v.size(), msg);
	const size_t size = v.size() * sizeof(v[0]);
	if (os.write(cybozu::cast<const char*>(&v[0]), size) && os.flush()) return;
	throw cybozu::Exception("sucvector_util:saveVec") << msg;
}

} // cybozu::sucvector_util

/*
	extra memory
	(32 + 8 * 4) / 256 = 1/4 bit per bit for rank
*/
class SucVector {
	static const uint64_t maxBitLen = 32 + 8; // don't increase this value
	static const uint64_t maxBitSize = uint64_t(1) << maxBitLen;
	struct B {
		uint64_t org[4];
		union {
			uint64_t a64;
			struct {
				uint32_t a;
				uint8_t b[4]; // b[0] is used for (b[0] << 32) | a
			} ab;
		};
	};
	uint64_t bitSize_;
	uint64_t numTbl_[2];
	std::vector<B> blk_;
	typedef std::vector<uint32_t> Uint32Vec;
	static const uint64_t posUnit = 1024;
	Uint32Vec selTbl_[2];

	template<int b>
	uint64_t rank_a(size_t i) const
	{
		assert(i < blk_.size());
		uint64_t ret = blk_[i].a64 & makeBitMask64(maxBitLen);
		if (!b) ret = i * uint64_t(256) - ret;
		return ret;
	}
	template<bool b>
	size_t get_b(size_t L, size_t i) const
	{
		assert(L < blk_.size());
		assert(0 < i && i < 4);
		size_t r = blk_[L].ab.b[i];
		if (!b) r = 64 * i - r;
		return r;
	}
	// call after blk_, numTbl_ are initialized
	void initSelTbl()
	{
		initSelTblSub<false>(selTbl_[0]);
		initSelTblSub<true>(selTbl_[1]);
	}
	template<bool b>
	void initSelTblSub(Uint32Vec& tbl)
	{
		const int tablePos = b ? 1 : 0;
		assert(numTbl_[tablePos] / posUnit < 0x7fffffff - 1);
		const size_t size = size_t(sucvector_util::getBlockNum(numTbl_[tablePos], posUnit));
		tbl.resize(size);
		uint32_t pos = 0;
		for (size_t i = 0; i < size; i++) {
			uint64_t r = i * posUnit;
			assert(pos < 0x7fffffff);
			while (rank_a<b>(pos) < r) {
				pos++;
			}
			tbl[i] = pos;
		}
	}
public:
	SucVector() : bitSize_(0) { numTbl_[0] = numTbl_[1] = 0; }
	/*
		data format(endian is depend on CPU:eg. little endian for x86/x64)
		bitSize  : 8
		numTbl_[0]  : 8
		numTbl_[1]  : 8
		blkSize  : 8
		blk data : blkSize * sizeof(B)
	*/
	void save(std::ostream& os) const
	{
		sucvector_util::save64bit(os, bitSize_, "bitSize");
		sucvector_util::save64bit(os, numTbl_[0], "num0");
		sucvector_util::save64bit(os, numTbl_[1], "num1");
		sucvector_util::saveVec(os, blk_, "blk");
	}
	void load(std::istream& is)
	{
		bitSize_ = sucvector_util::load64bit(is, "bitSize");
		numTbl_[0] = sucvector_util::load64bit(is, "num0");
		numTbl_[1] = sucvector_util::load64bit(is, "num1");
		sucvector_util::loadVec(blk_, is, "blk");
		initSelTbl();
	}
	/*
		@param blk [in] bit pattern block
		@param bitSize [in] bitSize ; blk size = (bitSize + 63) / 64
	*/
	SucVector(const uint64_t *blk, uint64_t bitSize)
	{
		init(blk, bitSize);
	}
	void init(const uint64_t *blk, uint64_t bitSize)
	{
		if (bitSize > maxBitSize) throw cybozu::Exception("SucVector:too large") << bitSize;
		assert((bitSize + 63) / 64 <= ~size_t(0));
		bitSize_ = bitSize;
		const size_t blkNum = size_t((bitSize + 63) / 64);
		const size_t tblNum = (blkNum + 3) / 4; // tblNum <= 2^32
		blk_.resize(tblNum);

		uint64_t av = 0;
		size_t pos = 0;
		for (size_t i = 0; i < tblNum; i++) {
			B& b = blk_[i];
			b.a64 = av % maxBitSize;
			uint32_t bv = 0;
			for (size_t j = 0; j < 4; j++) {
				uint64_t v = pos < blkNum ? blk[pos++] : 0;
				b.org[j] = v;
				uint32_t c = cybozu::popcnt(v);
				av += c;
				if (j > 0) {
					b.ab.b[j] = (uint8_t)bv;
				}
				bv += c;
			}
		}
		numTbl_[0] = blkNum * 64 - av;
		numTbl_[1] = av;
		initSelTbl();
	}
	uint64_t rank1(uint64_t i) const
	{
		if (i >= bitSize_) return numTbl_[1];
		assert(i / 256 <= ~size_t(0));
		size_t q = size_t(i / 256);
		size_t r = size_t((i / 64) & 3);
		assert(q < blk_.size());
		const B& b = blk_[q];
		uint64_t ret = b.a64 % maxBitSize;
		if (r > 0) {
//			ret += b.ab.b[r];
			ret += uint8_t(b.a64 >> (32 + r * 8));
		}
		ret += cybozu::popcnt<uint64_t>(b.org[r] & cybozu::makeBitMask64(i & 63));
		return ret;
	}
	uint64_t size() const { return bitSize_; }
	uint64_t size(bool b) const { return numTbl_[b ? 1 : 0]; }
	uint64_t rank0(uint64_t i) const
	{
		return i - rank1(i);
	}
	uint64_t rank(bool b, uint64_t i) const
	{
		if (b) return rank1(i);
		return rank0(i);
	}
	bool get(uint64_t i) const
	{
		assert(i < bitSize_);
		size_t q = size_t(i / 256);
		size_t r = size_t((i / 64) & 3);
		assert(q < blk_.size());
		const B& b = blk_[q];
		return (b.org[r] & (1ULL << (i & 63))) != 0;
	}
	uint64_t select0(uint64_t rank) const { return selectSub<false>(rank); }
	uint64_t select1(uint64_t rank) const { return selectSub<true>(rank); }
	uint64_t select(bool b, uint64_t rank) const
	{
		if (b) return select1(rank);
		return select0(rank);
	}

	/*
		0123456789
		0100101101
		 ^  ^ ^^
		 0  1 23
		select(v, r) = min { i - 1 | rank(v, i) = r + 1 }
		select(3) = 7
	*/
	template<bool b>
	uint64_t selectSub(uint64_t rank) const
	{
		const int tablePos = b ? 1 : 0;
		if (rank >= numTbl_[tablePos]) return NotFound;
		const Uint32Vec& tbl = selTbl_[tablePos];
		assert(rank / posUnit < tbl.size());
		const size_t pos = (size_t)(rank / posUnit);
		size_t L = tbl[pos];
		size_t R = pos >= tbl.size() - 1 ? blk_.size() : tbl[pos + 1];
		rank++;
		while (L < R) {
			size_t M = (L + R) / 2; // (R - L) / 2 + L;
			if (rank_a<b>(M) < rank) {
				L = M + 1;
			} else {
				R = M;
			}
		}
		if (L > 0) L--;
		rank -= rank_a<b>(L);

		size_t i = 0;
		while (i < 3) {
			size_t r = get_b<b>(L, i + 1);
			if (r >= rank) {
				break;
			}
			i++;
		}
		if (i > 0) {
			size_t r = get_b<b>(L, i);
			rank -= r;
		}
		uint64_t v = blk_[L].org[i];
		if (!b) v = ~v;
		assert(rank <= 64);
		uint64_t ret = cybozu::sucvector_util::select64(v, size_t(rank));
		ret += L * 256 + i * 64;
		return ret;
   	}
};

} // cybozu

#ifdef _WIN32
	#pragma warning(pop)
#endif
