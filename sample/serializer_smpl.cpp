/*
	how to serialize user data
*/
#include <stdio.h>
#include <cybozu/file.hpp>
#include <cybozu/serializer.hpp>
#include <cybozu/mmap.hpp>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <cybozu/string.hpp>

struct Data {
	typedef std::map<std::string, double> Str2Double;
	typedef std::set<int> IntSet;
	typedef std::vector<float> FloatVec;
	typedef std::vector<cybozu::String> StrVec;
	Str2Double sd_;
	IntSet is_;
	FloatVec fv_;
	StrVec sv_;
	bool operator==(const Data& rhs) const
	{
		return sd_ == rhs.sd_ && is_ == rhs.is_ && fv_ == rhs.fv_ && sv_ == rhs.sv_;
	}
	// friend declaration of load and save function
	template<class InputStream>
	friend void load(Data& x, InputStream& is);
	template<class OutputStream>
	friend void save(OutputStream& os, const Data& x);
};

/*
	define load and save function in cybozu namespace
*/
namespace cybozu {

template<class InputStream>
void load(Data& x, InputStream& is)
{
	cybozu::load(x.sd_, is);
	cybozu::load(x.is_, is);
	cybozu::load(x.fv_, is);
	cybozu::load(x.sv_, is);
}

template<class OutputStream>
void save(OutputStream& os, const Data& x)
{
	cybozu::save(os, x.sd_);
	cybozu::save(os, x.is_);
	cybozu::save(os, x.fv_);
	cybozu::save(os, x.sv_);
}
} // cybozu

int main()
{
	const char *file = "test.data";
	Data x;
	x.sd_["abc"] = 9.3;
	x.sd_["999"] = -123459.3;
	x.sd_["AAAA"] = 7;
	x.is_.insert(5);
	x.is_.insert(55);
	x.is_.insert(555);
	x.is_.insert(5555);
	x.fv_.push_back(9.2);
	x.fv_.push_back(-0.1);
	x.sv_.push_back("aiueo");
	x.sv_.push_back("xxx");
	x.sv_.push_back("doremi");

	// save data to std::ofstream
	{
		std::ofstream os(file, std::ios::binary);
		cybozu::save(os, x);
	}
	// load data from std::ifstream
	{
		std::ifstream is(file, std::ios::binary);

		Data y;
		cybozu::load(y, is);
		puts(x == y ? "ok" : "ng");
	}
	// load data from cybozu::Mmap
	{
		cybozu::Mmap m(file);
		cybozu::MemoryInputStream is(m.get(), m.size());

		Data y;
		cybozu::load(y, is);
		puts(x == y ? "ok" : "ng");
	}
}

