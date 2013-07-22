#pragma once
/**
	@file
	@brief logger

	Copyright (C) Cybozu Labs, Inc., all rights reserved.
*/
#include <cybozu/format.hpp>
#include <cybozu/time.hpp>
#ifdef _WIN32
#else
	#include <syslog.h>
#endif

namespace cybozu {

enum LogPriority {
	LogDebug = 0,
	LogInfo = 1,
	LogWarning = 2,
	LogError = 3
};

namespace log_local {

class Logger {
	int priority_;
	FILE *fp_;
public:
	Logger()
		: priority_(cybozu::LogDebug)
		, fp_(NULL)
	{
	}
	~Logger()
	{
		closeFile();
	}
	void put(LogPriority priority, const char *format, va_list args)
	{
		if (priority <= priority_) return;
		std::string str;
		cybozu::vformat(str, format, args);
		if (fp_) {
			cybozu::Time cur(true);
			if (fprintf(fp_, "%s %s\n", cur.toString(false).c_str(), str.c_str()) < 0) {
				fprintf(stderr, "ERR:cybozu:Logger:put:fprintf:%s\n", str.c_str());
			}
			if (fflush(fp_) < 0) {
				fprintf(stderr, "ERR:cybozu:Logger:put:fflush:%s\n", str.c_str());
			}
		} else {
#ifdef __linux__
			int pri = LOG_CRIT;
			if (priority == LogDebug) {
				pri = LOG_DEBUG;
			} else if (priority == LogInfo) {
				pri = LOG_INFO;
			} else if (priority == LogWarning) {
				pri = LOG_WARNING;
			}
			::syslog(pri, "%s\n", str.c_str());
#endif
		}
	}

	static Logger& getInstance()
	{
		static Logger logger;
		return logger;
	}
	/*
		@note : not thread safe
	*/
	void openFile(const std::string& path)
	{
		closeFile();
		fp_ = fopen(path.c_str(), "a+b");
		if (fp_ == 0) throw cybozu::Exception("cybozu:Logger:openFile") << path;
	}
	bool closeFile() throw()
	{
		if (fp_ == 0) return true;
		bool isOK = fclose(fp_) == 0;
		fp_ = NULL;
		return isOK;
	}
	void setPriority(LogPriority priority)
	{
		priority_ = priority;
	}
};

} // cybozu::log_local

/*
	write log to path
	@note this function is not thread safe
*/
inline void OpenLogFile(const std::string& path)
{
	return log_local::Logger::getInstance().openFile(path);
}

/*
	set priority(default cybozu::LogDebug)
	does not show the message of which is less or equal to the priority
*/
inline void SetLogPriority(LogPriority priority)
{
	log_local::Logger::getInstance().setPriority(priority);
}
/*
	write log
	Linux : default is syslog
	Windows : default is file(use openFile)
*/
inline void PutLog(LogPriority priority, const char *format, ...) throw()
	try
{
	va_list args;
	va_start(args, format);
	log_local::Logger::getInstance().put(priority, format, args);
	va_end(args);
} catch (std::exception& e) {
	fprintf(stderr, "faital error in cybozu::PutLog %s\n", e.what());
} catch (...) {
	fprintf(stderr, "faital error in cybozu::PutLog\n");
}

} // cybozu
