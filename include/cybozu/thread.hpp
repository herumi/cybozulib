#pragma once
/**
	@file
	@brief tiny thread class

	Copyright (C) 2007 Cybozu Labs, Inc., all rights reserved.
	@author MITSUNARI Shigeo
*/

#include <cybozu/atomic.hpp>
#include <assert.h>
#ifdef _WIN32
	#include <windows.h>
	#include <process.h>
	#pragma warning(disable : 4127)
#else
	#include <unistd.h>
	#include <pthread.h>
	#include <sys/time.h> // for gettimeofday
	#include <errno.h>
	#include <stdio.h>
#endif

namespace cybozu {

/**
	sleep
	@param [in] msec time out(msec)
*/
inline void Sleep(int msec)
{
#ifdef _WIN32
	::Sleep(msec);
#else
	struct timespec req;
	req.tv_sec = msec / 1000;
	req.tv_nsec = (msec % 1000) * 1000000;
	nanosleep(&req, 0);
#endif
}

/**
	get pid
*/
inline unsigned int GetProcessId()
{
#ifdef _WIN32
	return GetCurrentProcessId();
#else
	assert(sizeof(pid_t) == 4);
	return getpid();
#endif
}

namespace thread {

#ifdef _WIN32
	typedef HANDLE ThreadHandle;
	const int Infinite = INFINITE;
	typedef unsigned __stdcall ThreadEntryCallback(void*);
#else
	typedef pthread_t ThreadHandle;
	const int Infinite = 0x7fffffff;
	typedef void* ThreadEntryCallback(void*);
#endif

/**
	generate thread handle
	@param hdl [out] thread handle
	@param entryFct [in] thread entry
	@param arg [in] thread parameter
*/
inline bool Begin(ThreadHandle& hdl, ThreadEntryCallback entryFct, void *arg, int stackSize = 0)
{
#ifdef _WIN32
	unsigned int threadId = 0;
	hdl = reinterpret_cast<ThreadHandle>(_beginthreadex(0, stackSize, entryFct, arg, 0, &threadId));
	return hdl != 0;
#else
	pthread_attr_t attr;
	if (pthread_attr_init(&attr)) return false;
	if (stackSize) if (pthread_attr_setstacksize(&attr, stackSize)) return false;
	return pthread_create(&hdl, &attr, entryFct, arg) != 0;
#endif
}

inline bool Detach(ThreadHandle hdl)
{
#ifdef _WIN32
	int ret = CloseHandle(hdl);
	return ret != 0;
#else
	int ret = pthread_detach(hdl);
	return ret == 0;
#endif
}

/**
	join thread
	@param hdl [in] thread handle
	@param timeoutMsec [in] time out(msec)
*/
inline bool Join(ThreadHandle hdl, int timeoutMsec)
{
#ifdef _WIN32
	bool waitRet = WaitForSingleObject(hdl, timeoutMsec) == WAIT_OBJECT_0;
	bool closeRet = CloseHandle(hdl) != 0;
	return waitRet && closeRet;
#else
	if (timeoutMsec != Infinite) {
		fprintf(stderr, "not support timeout\n");
	}
	return pthread_join(hdl, NULL) == 0;
#endif
}

} // cybozu::thread

/**
	thread generator class
*/
class ThreadBase {
private:
	ThreadBase(const ThreadBase&);
	void operator=(const ThreadBase&);
	virtual void threadEntry() = 0;
protected:
	thread::ThreadHandle threadHdl_;

	/*
		thread loop interface
	*/
	static void threadLoopIF(void *arg)
	{
		ThreadBase* main = reinterpret_cast<ThreadBase*>(arg);
		main->threadEntry();
		// end of thread
//		main->detachThread();
	}
public:
	ThreadBase()
		: threadHdl_(0)
	{
	}
	virtual ~ThreadBase()
	{
		joinThread();
	}

	bool beginThread(int stackSize = 0)
	{
		return thread::Begin(threadHdl_, (thread::ThreadEntryCallback*)threadLoopIF, this, stackSize);
	}

	bool detachThread()
	{
		thread::ThreadHandle hdl = (thread::ThreadHandle)AtomicExchangeSize_t((size_t*)&threadHdl_, 0);
		if (hdl) {
			return thread::Detach(hdl);
		}
		return false;
	}

	bool joinThread(int waitMsec = thread::Infinite)
	{
		thread::ThreadHandle hdl = (thread::ThreadHandle)AtomicExchangeSize_t((size_t*)&threadHdl_, 0);
		if (hdl) {
			if (thread::Join(hdl, waitMsec)) {
				return false;
			}
		}
		return true;
	}
};

} // cybozu
