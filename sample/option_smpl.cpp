/*
	how to use
*/
#include <stdio.h>
#include <cybozu/option.hpp>
#include <vector>

struct A : public cybozu::Option {
	int x;
	bool b;
	double d;
	std::string y;
	std::vector<int> z;
	std::vector<std::string> w;
	std::string inName;
	std::vector<std::string> r;
	std::vector<int> vi;
	A()
	{
		appendOpt(&x, 5, "x", "int");
		appendOpt(&b, false, "b", "bool");
		appendMust(&d, "d", "double");
		appendMust(&y, "y", "string");
		appendVec(&z, "z", "int int int ...");
		appendVec(&w, "w", "str str str ...");
		appendParam(&inName, "input-file", "text file");
		appendParamVec(&vi, "remains", "sss");
	}
};

int main(int argc, char *argv[])
	try
{
	A a;
	if (a.parse(argc, argv)) {
		a.put();
	} else {
		a.usage();
	}
} catch (std::exception& e) {
	printf("ERR %s\n", e.what());
}
