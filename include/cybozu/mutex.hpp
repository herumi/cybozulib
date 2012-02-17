#pragma once
/**
	@file
	@brief mutex

	Copyright (C) 2007-2012 Cybozu Inc., all rights reserved.
	@author MITSUNARI Shigeo
*/

#ifdef _WIN32
	#include <windows.h>
#else
	#include <pthread.h>
#endif
#include <assert.h>

namespace cybozu {

class ConditionVariable;

namespace thread {

#ifdef _WIN32
	typedef HANDLE MutexHandle;
	inline void MutexInit(MutexHandle& mutex)
	{
		mutex = CreateSemaphore(NULL /* no security */, 1 /* init */, 0x7FFFFFFF /* max */, NULL /* no name */);
	}
	inline void MutexLock(MutexHandle& mutex) { WaitForSingleObject(mutex, INFINITE); }
	/*
		return false if timeout
		@param msec [in] msec
	*/
	inline bool MutexLockTimeout(MutexHandle& mutex, int msec)
	{
		DWORD ret = WaitForSingleObject(mutex, msec);
		if (ret == WAIT_OBJECT_0) {
			return true;
		}
		if (ret == WAIT_TIMEOUT) {
			return false;
		}
		/* ret == WAIT_ABANDONED */
		assert(0);
		return false;
	}
	inline void MutexUnlock(MutexHandle& mutex)
	{
		ReleaseSemaphore(mutex, 1, NULL);
	}
	inline void MutexTerm(MutexHandle& mutex) { CloseHandle(mutex); }
#else
	typedef pthread_mutex_t MutexHandle;
	inline void MutexInit(MutexHandle& mutex)
	{
		pthread_mutex_init(&mutex, NULL);
	}
	inline void MutexLock(MutexHandle& mutex) { pthread_mutex_lock(&mutex); }
	inline bool MutexLockTimeout(MutexHandle& mutex, int msec)
	{
		timespec absTime;
		clock_gettime(CLOCK_REALTIME, &absTime);
		absTime.tv_sec += msec / 1000;
		absTime.tv_nsec += msec % 1000;
		bool ret = pthread_mutex_timedlock(&mutex, &absTime) == 0;
		return ret;
	}
	inline void MutexUnlock(MutexHandle& mutex) { pthread_mutex_unlock(&mutex); }
	inline void MutexTerm(MutexHandle& mutex) { pthread_mutex_destroy(&mutex); }
#endif

template<class T>
class AutoLockT {
public:
	explicit AutoLockT(T &t)
		: t_(t)
	{
		t_.lock();
	}
	~AutoLockT()
	{
		t_.unlock();
	}
private:
	T& t_;
	AutoLockT& operator=(const AutoLockT&);
};

} // cybozu::thread

class Mutex {
	friend class cybozu::ConditionVariable;
public:
	Mutex()
	{
		thread::MutexInit(hdl_);
	}
	~Mutex()
	{
		thread::MutexTerm(hdl_);
	}
	void lock()
	{
		thread::MutexLock(hdl_);
	}
	bool lockTimeout(int msec)
	{
		return thread::MutexLockTimeout(hdl_, msec);
	}
	void unlock()
	{
		thread::MutexUnlock(hdl_);
	}
private:
	Mutex(const Mutex&);
	Mutex& operator=(const Mutex&);
	thread::MutexHandle hdl_;
};

typedef cybozu::thread::AutoLockT<cybozu::Mutex> AutoLock;

} // cybozu
