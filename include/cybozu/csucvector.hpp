#pragma once
/**
	@file
	@brief compressed succinct vector
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
	@note use -msse4.2 option for popcnt
*/
#include <cybozu/sucvector.hpp>
#include <cybozu/bitvector.hpp>
#include <cybozu/serializer.hpp>
#include <vector>
#include <iosfwd>
#include <map>

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4351) // init buf in cstr
#endif

#define USE_BITBL

//#define USE_CLK
#ifdef USE_CLK
#include <xbyak/xbyak_util.h>
#endif

namespace cybozu {

namespace csucvector_util {

static const size_t tblBitLen = 4;

struct Encoding {
	uint64_t v;
	uint32_t len;
	uint32_t rk;
	Encoding(uint64_t v = 0, uint32_t len = 0)
		: v(v)
		, len(len)
		, rk(len <= 64 ? cybozu::popcnt<uint64_t>(v) : v == 0 ? 0 : len) { }
};

struct InputStream {
	const uint64_t *block_;
	size_t bitSize_;
	size_t blockSize_;
	size_t cur_;
	InputStream(const uint64_t *block, size_t bitSize)
		: block_(block), bitSize_(bitSize), blockSize_((bitSize + 63) / 64), cur_(0)
	{
	}
	uint64_t peek(size_t offset = 0) const
	{
		const size_t q = (cur_ + offset) / 64;
		const size_t r = (cur_ + offset) & 63;
		if (q >= blockSize_) return 0;
		if (r == 0) return block_[q];
		uint64_t L = block_[q];
		uint64_t H = q < blockSize_ - 1 ? block_[q + 1] : 0;
		return ((L >> r) & mask(64 - r)) | (H << (64 - r));
	}
	uint64_t mask(size_t r) const
	{
		return (uint64_t(1) << r) - 1;
	}
	void consume(size_t size)
	{
		if (!empty()) cur_ += size;
	}
	bool empty() const { return cur_ >= bitSize_; }
};

struct Bigram {
	struct Pair {
		uint32_t prev;
		uint32_t cur;
		Pair(uint32_t prev = 0, uint32_t cur = 0) : prev(prev), cur(cur) {}
	};
	const std::vector<Encoding>& encTbl_;
	uint32_t tblNum;
	std::vector<std::vector<uint32_t> > tbl;
	size_t prev;
	explicit Bigram(const std::vector<Encoding>& encTbl)
		: encTbl_(encTbl)
		, tblNum((uint32_t)encTbl.size())
		, tbl()
		, prev(tblNum) // first special value
	{
		tbl.resize(tblNum);
		for (uint32_t i = 0; i < tblNum; i++) {
			tbl[i].resize(tblNum);
		}
	}
	~Bigram(){ put(); }
	void append(uint32_t v)
	{
		if (v >= tblNum) throw cybozu::Exception("CSucVector:Bigram:bad v") << v;
		if (prev == tblNum) {
			prev = v;
			return;
		}
		tbl[prev][v]++;
		prev = v;
	}
	bool put(Pair *ppair = 0)
	{
		typedef std::multimap<uint32_t, Pair, std::greater<uint32_t> > Map;
		Map m;
		for (uint32_t i = 0; i < tblNum; i++) {
			for (uint32_t j = 0; j < tblNum; j++) {
				m.insert(Map::value_type(tbl[i][j], Pair(i, j)));
			}
		}
		int n = 0;
		for (Map::const_iterator i = m.begin(), ie = m.end(); i != ie; ++i) {
			if (i->first > 0) {
				printf("%u (%u, %u) ", i->first, i->second.prev, i->second.cur);
				{
					uint64_t L = encTbl_[i->second.prev].v;
					uint32_t Ln = encTbl_[i->second.prev].len;
					uint64_t H = encTbl_[i->second.cur].v;
					uint32_t Hn = encTbl_[i->second.cur].len;
					if (Ln + Hn <= 64) {
						printf(" { 0x%llx, %u }\n", (long long)((H << Ln) | L), Ln + Hn);
					} else {
						printf("over Ln=%u, Hn=%u\n", Ln, Hn);
					}
				}
				n++;
				if (n == 10) break;
			}
		}
		if (!m.empty() && ppair) {
			*ppair = m.begin()->second;
		}
		return !m.empty();
	}
private:
	Bigram(const Bigram&);
	void operator=(const Bigram&);
};

} // cybozu::csucvector_util

struct CSucVector {
#ifdef USE_CLK
	mutable Xbyak::util::Clock clkGet;
	mutable Xbyak::util::Clock clkRank;
	void putClkSub(const char *msg, const Xbyak::util::Clock& clk) const
	{
		if (clk.getCount() == 0) return;
		printf("%s:%6.2f %d\n", msg, clk.getClock() / double(clk.getCount()), clk.getCount());
	}
	void putClk() const
	{
		putClkSub("get   ", clkGet);
		putClkSub("rank  ", clkRank);
		puts("");
	}
#endif

	struct Block {
		uint32_t orgPos;
		uint32_t vecPos;
		uint32_t rk;
		Block(uint32_t orgPos = 0, uint32_t vecPos = 0, uint32_t rk = 0) : orgPos(orgPos), vecPos(vecPos), rk(rk) {}
	};
	static const uint32_t skip = 1024;
	typedef std::vector<uint64_t> Vec64;
	typedef std::vector<Block> BlockVec;
	typedef std::vector<csucvector_util::Encoding> EncodingTbl;
	typedef std::vector<uint32_t> Vec32;
	struct LenRank {
		uint32_t len;
		uint32_t rk;
	};
	EncodingTbl encTbl;
	LenRank biTbl[256];
	uint32_t bitSize_;
	Vec64 vec;
	BlockVec blkVec;
	uint32_t rk_;
	Vec32 freqTbl;

	struct OutputStream {
		Vec32& freqTbl; // output
		Vec64& vec; // output
		uint32_t rk_; // output
		csucvector_util::Bigram bi; // output
		uint64_t vsub; // tmp
		size_t vsubPos; // tmp
		const EncodingTbl& encTbl; // in
		OutputStream(Vec32& freqTbl, Vec64& vec, const uint64_t *buf, uint32_t bitSize, const EncodingTbl& encTbl)
			: freqTbl(freqTbl)
			, vec(vec)
			, rk_(0)
			, bi(encTbl)
			, vsub(0)
			, vsubPos(0)
			, encTbl(encTbl)
		{
			csucvector_util::InputStream is(buf, bitSize);
			freqTbl.clear();
			freqTbl.resize(encTbl.size());
			vec.clear();
			for (;;) {
				uint32_t s = append(is);
				is.consume(s);
				if (is.empty()) break;
			}
			printf("bitSize=%u\n",bitSize);
	
			if (vsubPos) {
				vec.push_back(vsub);
				vsub = 0;
				vsubPos = 0;
			}
		}
		uint32_t append(const csucvector_util::InputStream& is)
		{
			uint64_t v = is.peek();
			for (size_t i = 0; i < encTbl.size(); i++) {
				const uint32_t len = encTbl[i].len;
				bool found = false;
				if (len >= 64) {
					const size_t n = len / 64;
					const uint64_t target = encTbl[i].v;
					if (v == target) {
						found = true;
						for (size_t j = 1; j < n; j++) {
							if (is.peek(j * 64) != target) {
								found = false;
								break;
							}
						}
					}
				} else {
					const uint64_t mask = (uint64_t(1) << len) - 1;
					found = (v & mask) == encTbl[i].v;
				}
				if (found) {
					bi.append(i);
					freqTbl[i]++;
					rk_ += encTbl[i].rk;
					vsub |= uint64_t(i) << (csucvector_util::tblBitLen * vsubPos);
					vsubPos++;
					if (vsubPos == 16) {
						vec.push_back(vsub);
						vsub = 0;
						vsubPos = 0;
					}
					return len;
				}
			}
			printf("NOT HERE!!! in debug mode\n");
			for (size_t i = 0; i < 4; i++) {
				printf("of=%d %llx\n", (int)i, (long long)is.peek(i * 64));
			}
			exit(1);
		}
	};
	void initTable()
	{
		static const struct {
			uint64_t v;
			uint32_t len;
		} tbl[] = {
			{ 0, 64 * 8 },
			{ uint64_t(-1), 64 * 4 },
			{ 0, 24 },
			{ 0, 12 },
			{ 0xfff, 12 },
			{ 0, 9 },
			{ 0x20, 6 },
			{ 0x3f, 6 },
			{ 0, 3 },
			{ 1, 3 },
			{ 2, 3 },
			{ 3, 3 },
			{ 4, 3 },
			{ 5, 3 },
			{ 6, 3 },
			{ 7, 3 },
		};
		encTbl.clear();
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			encTbl.push_back(csucvector_util::Encoding(tbl[i].v, tbl[i].len));
		}
		for (uint32_t i = 0; i < 16; i++) {
			for (uint32_t j = 0; j < 16; j++) {
				uint32_t v = j * 16 + i;
				biTbl[v].len = encTbl[i].len + encTbl[j].len;
				biTbl[v].rk = encTbl[i].rk + encTbl[j].rk;
			}
		}
	}

	CSucVector() { clear(); }
	~CSucVector()
	{
		put();
#ifdef USE_CLK
		putClk();
#endif
	}
	CSucVector(const uint64_t *buf, uint64_t bitSize)
	{
		clear();
		init(buf, bitSize);
	}
	void clear()
	{
		bitSize_ = 0;
		rk_ = 0;
	}
	void init(const uint64_t *buf, uint64_t bitSize)
	{
		if (bitSize >= (uint64_t(1) << 32)) throw cybozu::Exception("CSucVector:init:big bitSize") << bitSize;
		bitSize_ = (uint32_t)bitSize;
		initTable();
		OutputStream os(freqTbl, vec, buf, bitSize_, encTbl);
		rk_ = os.rk_;
		initBlockVec();
	}
	void initBlockVec()
	{
		blkVec.reserve(bitSize_ / skip + 16);
		uint32_t orgPos = 0;
		uint32_t rk = 0;
		uint32_t samplingPos = 0;
		const uint8_t *const pv = (const uint8_t*)&vec[0];
		for (uint32_t vecPos = 0, n = (uint32_t)vec.size() * 8; vecPos < n; vecPos++) {
			uint8_t v = pv[vecPos];
			uint32_t next = orgPos + (uint32_t)encTbl[v & 15].len + (uint32_t)encTbl[v >> 4].len;

			while (samplingPos < next) {
				blkVec.push_back(Block(orgPos, vecPos, rk));
				samplingPos += skip;
			}
			orgPos = next;
			rk += encTbl[v & 15].rk + encTbl[v >> 4].rk;
		}
		printf("blkVec.size=%u, assume=%u\n", (int)blkVec.size(), bitSize_ / skip + 16);
		printf("Vec2: orgPos=%u, rk=%u\n", orgPos, rk);
	}
	void putSub() const
	{
		const uint32_t inSize = bitSize_ / 8;
		const size_t compSize = vec.size() * sizeof(vec[0]);
		const size_t idxSize = blkVec.size() * sizeof(blkVec[0]);
		const double cr = compSize * 100.0 / inSize;
		const double ir = idxSize * 100.0 / inSize;
		if (inSize == 0) return;
		printf("in   Size= %10lld, rank=%u\n", (long long)inSize, rk_);
		printf("comp Size= %10lld(vec.size=%7lld)\n", (long long)compSize, (long long)vec.size());
		printf("idx  Size= %10lld(blkVec.size=%7lld)\n", (long long)idxSize, (long long)blkVec.size());
		printf("totalSize= %10lld\n", (long long)(compSize + idxSize));
		printf("rate=%5.2f%%(%5.2f%% + %5.2f%%)\n", cr + ir, cr, ir);
	}
	void put() const
	{
		putSub();
		if (freqTbl.empty()) return;
		const size_t compSize = vec.size() * sizeof(vec[0]);
		for (size_t i = 0; i < freqTbl.size(); i++) {
			printf("freqTbl[%2d] = %8d(%5.2f%%, %5.2f%%)\n", (int)i, freqTbl[i], freqTbl[i] * 0.5 * 100.0 / compSize, freqTbl[i] * encTbl[i].len * 100.0 / bitSize_);
		}
	}
	uint64_t getMask(size_t pos) const
	{
		assert(pos <= 64);
		if (pos == 64) return uint64_t(-1);
		return (uint64_t(1) << pos) - 1;
	}
	bool get(size_t pos) const
	{
		if (pos >= bitSize_) throw cybozu::Exception("CSucVector:get:bad pos") << pos;
#ifdef USE_CLK
clkGet.begin();
#endif
		size_t cur = blkVec[pos / skip].orgPos;
		size_t vecPos = blkVec[pos / skip].vecPos;
		const uint8_t *p = (const uint8_t*)(&vec[0]) + vecPos;
		pos -= cur;
		for (;;) {
			uint8_t v = *p;
#ifdef USE_BITBL
			uint32_t len = biTbl[v].len;
			if (len <= pos) {
				pos -= len;
				p++;
				continue;
			}
#endif
			for (size_t j = 0; j < 2; j++) {
				int v4 = int(v & 15);
				size_t len = encTbl[v4].len;
				if (len > pos) {
					const bool b = (pos >= 64) ? encTbl[v4].v != 0 :(encTbl[v4].v & (size_t(1) << pos)) != 0 ;
#ifdef USE_CLK
clkGet.end();
#endif
					return b;
				}
				pos -= len;
				v >>= 4;
			}
			p++;
		}
	}
	size_t rank1(size_t pos) const
	{
		if (pos >= bitSize_) return rk_;
#ifdef USE_CLK
clkRank.begin();
#endif
		size_t cur = blkVec[pos / skip].orgPos;
		size_t vecPos = blkVec[pos / skip].vecPos;
		size_t rk = blkVec[pos / skip].rk;
		const uint8_t *p = (const uint8_t*)(&vec[0]) + vecPos;
		pos -= cur;
		for (;;) {
			uint8_t v = *p;
#ifdef USE_BITBL
			uint32_t len = biTbl[v].len;
			if (len <= pos) {
				pos -= len;
				rk += biTbl[v].rk;
				p++;
				continue;
			}
#endif
			for (size_t j = 0; j < 2; j++) {
				int v4 = int(v & 15);
				size_t len = encTbl[v4].len;
				if (len > pos) {
					uint64_t adj = 0;
					if (pos >= 64) {
						if (encTbl[v4].v != 0) adj = pos;
					} else {
						uint64_t x = encTbl[v4].v & ((uint64_t(1) << pos) - 1);
						adj = cybozu::popcnt<uint64_t>(x);
					}
					rk += adj;
#ifdef USE_CLK
clkRank.end();
#endif
					return rk;
				}
				pos -= len;
				rk += encTbl[v4].rk;
				v >>= 4;
			}
			p++;
		}
	}
	size_t rank0(size_t pos) const
	{
		return pos - rank1(pos);
	}
	size_t rank(bool b, size_t pos) const
	{
		if (b) return rank1(pos);
		return rank0(pos);
	}
	template<class OutputStream>
	void save(OutputStream& os) const
	{
		cybozu::save(os, bitSize_);
		cybozu::savePodVec(os, vec);
		cybozu::savePodVec(os, blkVec);
		cybozu::save(os, rk_);
	}
	template<class InputStream>
	void load(InputStream& is)
	{
		cybozu::load(bitSize_, is);
		cybozu::loadPodVec(vec, is);
		cybozu::loadPodVec(blkVec, is);
		cybozu::load(rk_, is);
		initTable();
	}
};

} // cybozu

#ifdef _WIN32
	#pragma warning(pop)
#endif
