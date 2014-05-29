/*
	sample how to use stacktrace
	g++ -I ../include/ stacktrace_smpl.cpp -g -rdynamic && ./a.out
*/
#include <stdio.h>
#include <iostream>
#include <cybozu/stacktrace.hpp>

void putStackTrace()
{
	std::cout << "dump stack trace\n" << cybozu::StackTrace() << std::endl;
	exit(1);
}

struct A {
	void f() {
		putStackTrace();
	}
};

struct B {
	A a;
	void g() {
		a.f();
	}
};

void func()
{
	puts("func");
	B b;
	b.g();
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
	h();
} catch (std::exception& e) {
	printf("e=%s\n", e.what());
}

