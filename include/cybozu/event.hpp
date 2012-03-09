#pragma once
/**
	@file
	@brief event class

	Copyright (C) 2007-2012 Cybozu Labs, Inc., all rights reserved.
*/

#ifdef _WIN32
	#include <windows.h>
#else
	#include <cybozu/condition_variable.hpp>
#endif

namespace cybozu {

#ifdef _WIN32
class Event {
	HANDLE event_;
public:
	Event()
	{
		event_ = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	~Event()
	{
		if (event_) ::CloseHandle(event_);
	}
	void wakeup()
	{
		if (event_) SetEvent(event_);
	}
	bool wait()
	{
		const DWORD msec = INFINITE;
		if (event_) {
			return WaitForSingleObject(event_, msec) == WAIT_OBJECT_0;
		}
		return false;
	}
};
#else
class Event {
	cybozu::Mutex mutex_;
	cybozu::ConditionVariable cv_;
public:
	Event()
	{
		mutex_.lock();
	}
	~Event()
	{
		mutex_.unlock();
	}
	void wakeup()
	{
		cv_.notifyOne();
	}
	bool wait()
	{
		cv_.wait(mutex_);
		return true;
	}
};
#endif

} // cybozu
