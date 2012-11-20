#include <stdio.h>
#include <vector>
#include <iostream>
#include <cybozu/stacktrace.hpp>

void putStackTrace()
{
	std::cout << "no memory!!\n" << cybozu::StackTrace() << std::endl;
	exit(1);
}

void func()
{
	puts("func");
	size_t size = 1024 * 1024;
	std::vector<double> v;
	for (int i = 0; i < 10240; i++) {
		std::cout << "i=" << i << ", size=" << size << std::endl;
		v.resize(size);
		size *= 16;
	}
}

void g()
{
	func();
}

void h()
{
	g();
}

int main()
	try
{
	std::set_new_handler(putStackTrace);
	h();
} catch (std::exception& e) {
	printf("e=%s\n", e.what());
}

