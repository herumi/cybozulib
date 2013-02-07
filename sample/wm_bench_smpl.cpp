#include <cybozu/wavelet_matrix.hpp>
#include <cybozu/xorshift.hpp>
#include <algorithm>
#include <time.h>
#include <stdlib.h>

struct Naive {
private:
	Naive(const Naive&);
	void operator=(const Naive&);
public:
	const std::vector<uint8_t>& v_;
	explicit Naive(const std::vector<uint8_t>& v)
		: v_(v)
	{
	}
	size_t get(size_t pos) const
	{
		return v_[pos];
	}
	size_t rank(uint32_t val, size_t pos) const
	{
		return std::count(v_.begin(), v_.begin() + pos, val);
	}
	size_t rankLt(uint32_t val, size_t pos) const
	{
		size_t ret = 0;
		for (size_t i = 0, n = pos; i < n; i++) {
			if (v_[i] < val) ret++;
		}
		return ret;
	}
	size_t select(uint32_t val, size_t rank) const
	{
		const size_t N = v_.size();
		rank++;
		size_t pos = 0;
		while (rank > 0) {
			if (pos == N) return N;
			if (v_[pos] == val) {
				rank--;
			}
			pos++;
		}
		return pos - 1;
	}
};

template<class T, class RG>
void bench_get(const T& wm, RG& rg, size_t C, size_t N)
{
	size_t ret = 0;
	clock_t begin = clock();
	for (size_t i = 0; i < C; i++) {
		size_t pos = rg() & (N - 1);
		ret += wm.get(pos);
	}
	double t = (clock() - begin) / double(CLOCKS_PER_SEC);
	printf("get     %08x %9.2fusec\n", (int)ret, t / C * 1e6);
}

template<class T, class RG>
void bench_rank(const T& wm, RG& rg, size_t C, size_t N)
{
	size_t ret = 0;
	clock_t begin = clock();
	for (size_t i = 0; i < C; i++) {
		size_t pos = rg() & (N - 1);
		uint8_t c = uint8_t(rg());
		ret += wm.rank(c, pos);
	}
	double t = (clock() - begin) / double(CLOCKS_PER_SEC);
	printf("rank    %08x %9.2fusec\n", (int)ret, t / C * 1e6);
}

template<class T, class RG>
void bench_rankLt(const T& wm, RG& rg, size_t C, size_t N)
{
	size_t ret = 0;
	clock_t begin = clock();
	for (size_t i = 0; i < C; i++) {
		size_t pos = rg() & (N - 1);
		uint8_t c = uint8_t(rg());
		ret += wm.rankLt(c, pos);
	}
	double t = (clock() - begin) / double(CLOCKS_PER_SEC);
	printf("rankLt  %08x %9.2fusec\n", (int)ret, t / C * 1e6);
}

template<class T, class RG>
void bench_select(const T& wm, RG& rg, size_t C)
{
	size_t ret = 0;
	std::vector<int> maxTbl;
	maxTbl.resize(256);
	for (int i = 0; i < 256; i++) {
		maxTbl[i] = (int)wm.size(i);
	}
	clock_t begin = clock();
	for (size_t i = 0; i < C; i++) {
		uint8_t c = uint8_t(rg());
		size_t pos = rg() % maxTbl[c];
		ret += wm.select(c, pos);
	}
	double t = (clock() - begin) / double(CLOCKS_PER_SEC);
	printf("select  %08x %9.2fusec\n", (int)ret, t / C * 1e6);
#if 0
	begin = clock();
	for (size_t i = 0; i < C; i++) {
		uint8_t c = uint8_t(rg());
		size_t pos = maxTbl[c] + 1;
		ret += wm.select(c, pos);
	}
	t = (clock() - begin) / double(CLOCKS_PER_SEC);
	printf("none    %08x %9.2fusec\n", (int)ret, t / C * 1e6);
#endif
}

void bench(const cybozu::WaveletMatrix& wm, size_t N)
{
	cybozu::XorShift rg;
	puts("wm");
	bench_get(wm, rg, 1000000, N);
	bench_rank(wm, rg, 1000000, N);
	bench_rankLt(wm, rg, 1000000, N);
	bench_select(wm, rg, 100000);
#if 0
	bench_get(nv, "nv", 1000000, N);
	bench_rank(nv, "nv", 10, N);
	bench_rank_lt(nv, "nv", 10, N);
	bench_select(nv, "nv", 10);
#endif
}

void run(size_t bitLen)
{
	cybozu::XorShift rg;
	cybozu::WaveletMatrix wm;
	const size_t N = size_t(1) << bitLen;
	printf("%09llx\n", (long long)N);
	puts("init");
	std::vector<uint8_t> v;
	v.resize(N);
	for (size_t i = 0; i < N; i++) {
		v[i] = uint8_t(rg());
	}
	wm.init(v, 8);
	puts("start");
	bench(wm, N);
}

int main(int argc, char *argv[])
	try
{
	size_t bitLen = 26;
	argc--, argv++;
	while (argc > 0) {
		if (argc > 1 && strcmp(*argv, "-b") == 0) {
			argc--, argv++;
			bitLen = atoi(*argv);
		} else
		{
			printf("usage wm_bench_smpl.exe [-b bitLen]\n");
			return 1;
		}
		argc--, argv++;
	}
	run(bitLen);
} catch (std::exception& e) {
	printf("err %s\n", e.what());
}
