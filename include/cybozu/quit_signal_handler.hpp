#pragma once
/**
	@file
	@brief quit signal handler

	Copyright (C) 2009 Cybozu Labs, Inc., all rights reserved.
*/
#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#endif
#include <stdio.h>
#include <stdlib.h>

namespace cybozu {

/*
	class App must have quit() method
*/
template<class App>
class QuitSignalHandler {
	static App *app_;
#ifdef _WIN32
	static inline BOOL WINAPI ctrlHandler(DWORD) throw()
	{
		if (app_) app_->quit();
		return TRUE;
	}
#else
	static void ctrlHandler(int) throw()
	{
		app_->quit();
//		signal(SIGINT, SIG_IGN);
	}
#endif
public:
	explicit QuitSignalHandler(App& app)
	{
		app_ = &app;
#ifdef _WIN32
		bool isOK = SetConsoleCtrlHandler(ctrlHandler, TRUE) != 0;
#else
		struct sigaction sa;
		sa.sa_handler = ctrlHandler;
		sigfillset(&sa.sa_mask);
		sa.sa_flags = 0;
		bool isOK = (sigaction(SIGINT, &sa, NULL) == 0)
		         && (sigaction(SIGQUIT, &sa, NULL) == 0)
		         && (sigaction(SIGABRT, &sa, NULL) == 0)
		         && (sigaction(SIGTERM, &sa, NULL) == 0);
#endif
		if (!isOK) {
			fprintf(stderr, "can't setup ctrl handler\n");
			exit(1);
		}
	}
};

template<class App>
App *QuitSignalHandler<App>::app_;

} // cybozu
