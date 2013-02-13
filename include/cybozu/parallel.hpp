#pragma once
/**
	@file
	@brief parallel for
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <cybozu/exception.hpp>
#include <cybozu/thread.hpp>
#include <cybozu/atomic.hpp>
#include <cybozu/array.hpp>

namespace cybozu {

namespace parallel_util {

template<class T, class F, class N, class PN>
struct Thread : public cybozu::ThreadBase {
	T *t_;
	F f_;
	N begin_;
	N end_;
	PN *processedNum_;
	bool *quit_;
	Thread()
		: t_(0)
		, f_(0)
		, begin_(0)
		, end_(0)
		, processedNum_(0)
		, quit_(0)
	{
	}
	void init(T& t, F f, N begin, N end, PN *processedNum, bool *quit)
	{
		t_ = &t;
		f_ = f;
		begin_ = begin;
		end_ = end;
		processedNum_ = processedNum;
		quit_ = quit;
	}
	void threadEntry()
	{
		std::string err;
		for (N i = begin_; i < end_; i++) {
			if (quit_ && *quit_) break;
			try {
				(t_->*f_)(i);
				if (processedNum_) cybozu::AtomicAdd<PN>(processedNum_, 1);
			} catch (std::exception& e) {
				err = e.what();
				if (quit_) {
					*quit_ = true;
					cybozu::mfence();
				}
			}
		}
		if (!err.empty()) throw cybozu::Exception() << err;
	}
};

} // parallel_util
/*
	void T::f(N i);
*/
template<class T, class F, class N, class PN>
void parallel_for(T& target, F f, N n, int threadNum, PN *processedNum = 0, bool *quit = 0)
{
	if (threadNum <= 0) throw cybozu::Exception("cybozu:parallel_for:bad threadNum") << threadNum;
	if ((N)threadNum > n) threadNum = (int)n;
	typedef parallel_util::Thread<T, F, N, PN> Thread;
	{
		cybozu::ScopedArray<Thread> thread(threadNum);
		const N q = N(n / threadNum);
		const N r = N(n % threadNum);
		N begin = 0;
		for (N i = 0; i < threadNum; i++) {
			N end = begin + q;
			if (i < r) end++;
			thread[i].init(target, f, begin, end, processedNum, quit);
			begin = end;
		}
		for (N i = 0; i < threadNum; i++) {
			if (!thread[i].beginThread()) throw cybozu::Exception("cybozu:parallel_for:can't beginThread") << i;
		}
		for (N i = 0; i < threadNum; i++) {
			if (!thread[i].joinThread()) throw cybozu::Exception("cybozu:parallel_for:can't joinThread") << i;
		}
	}
}

} // cybozu
