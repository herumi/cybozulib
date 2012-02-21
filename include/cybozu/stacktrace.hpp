#pragma once
/**
	@file
	@brief stacktrace class
	Copyright (C) 2008-2012 Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include <vector>
#include <cybozu/itoa.hpp>
#include <stdlib.h>

#ifdef _WIN32
	#include <winsock2.h>
	#include <windows.h>
	#include <windows.h>
	#include <imagehlp.h>
	#include <stdio.h>
	#pragma comment(lib, "imagehlp.lib")
#else
	#include <execinfo.h>
	#include <string.h>
	#include <cxxabi.h>
	#include <stdint.h>
#endif

#ifndef NDEBUG
	#define CYBOZU_STACKTRACE_RESOLVE_SYMBOL
#endif

namespace cybozu {

namespace stacktrace_local {

#ifdef _WIN32

struct InitSymbol {
	HANDLE hdl_;
	InitSymbol()
		: hdl_(GetCurrentProcess())
	{
#ifdef CYBOZU_STACKTRACE_RESOLVE_SYMBOL
		SymInitialize(hdl_, NULL, TRUE);
#endif
	}
	~InitSymbol()
	{
#ifdef CYBOZU_STACKTRACE_RESOLVE_SYMBOL
		SymCleanup(hdl_);
#endif
	}
	HANDLE getHandle() const { return hdl_; }
};

template<int dummy = 0>
struct InstanceIsHere { static InitSymbol is_; };

template<int dummy>
InitSymbol InstanceIsHere<dummy>::is_;

struct DummyCall {
	DummyCall() { InstanceIsHere<>::is_.getHandle(); }
};

#endif

class AutoFree {
	void *p_;
public:
	AutoFree(void *p)
		: p_(p)
	{
	}
	~AutoFree()
	{
		free(p_);
	}
};

} // stacktrace_local

class StackTrace {
	std::vector<void*> data_;
public:
	/**
		set current stack trace
	*/
	StackTrace()
	{
		data_.clear();

		const int maxTraceNum = 32;

#ifdef _WIN32

		CONTEXT ctx;
		memset(&ctx, 0, sizeof(ctx));
		ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
		RtlCaptureContext(&ctx);

		STACKFRAME64 sf;
		memset(&sf, 0, sizeof(sf));

#ifdef _WIN64
		sf.AddrPC.Offset = ctx.Rip;
		sf.AddrFrame.Offset = ctx.Rbp;
		sf.AddrStack.Offset = ctx.Rsp;
		DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
#else
		sf.AddrPC.Offset = ctx.Eip;
		sf.AddrFrame.Offset = ctx.Ebp;
		sf.AddrStack.Offset = ctx.Esp;
		DWORD machineType = IMAGE_FILE_MACHINE_I386;
#endif
		sf.AddrPC.Mode = AddrModeFlat;
		sf.AddrFrame.Mode = AddrModeFlat;
		sf.AddrStack.Mode = AddrModeFlat;

		HANDLE threadHdl = GetCurrentThread();

		HANDLE processHdl = stacktrace_local::InstanceIsHere<>::is_.getHandle();
		for(;;) {
			BOOL ret;
			ret = StackWalk64(machineType, processHdl, threadHdl, &sf, &ctx, NULL,
					SymFunctionTableAccess64, SymGetModuleBase64, NULL);
			if (!ret || sf.AddrFrame.Offset == 0) return;

			data_.push_back((void*)sf.AddrPC.Offset);
			if (data_.size() == maxTraceNum) return;
		}
#else
		data_.resize(maxTraceNum);
		int traceNum = backtrace(&data_[0], maxTraceNum);
		data_.resize(traceNum);
#endif
	}
	/**
		get stack trace
	*/
	std::string toString() const
	{
		size_t traceNum = data_.size();
		if (traceNum == 0) return "";

		std::string out;

#ifdef _WIN32
#ifdef CYBOZU_STACKTRACE_RESOLVE_SYMBOL
		HANDLE processHdl = stacktrace_local::InstanceIsHere<>::is_.getHandle();
#endif
		if (traceNum > 1) traceNum--; // last information is unnecessary
		for (size_t i = 0; i < traceNum; i++) {
			uintptr_t ptr = (uintptr_t)data_[i];

#ifdef CYBOZU_STACKTRACE_RESOLVE_SYMBOL
			DWORD disp;
			IMAGEHLP_LINE64 line;
			memset(&line, 0, sizeof(line));
			line.SizeOfStruct = sizeof(line);
			if (SymGetLineFromAddr64(processHdl, ptr, &disp, &line)) {
				out += line.FileName;
				out += ':';
				out += cybozu::itoa((int)line.LineNumber);
				out += ' ';
			}
#endif
			out += "(" + cybozu::itohex(ptr, false) + ")";
			if (i < traceNum - 1) out += "\n";
		}
#else

#ifdef CYBOZU_STACKTRACE_RESOLVE_SYMBOL
		char **symbol = backtrace_symbols(&data_[0], traceNum);
		stacktrace_local::AutoFree freeSymbol(symbol);
#endif
		for (size_t i = 0; i < traceNum; i++) {
#ifdef CYBOZU_STACKTRACE_RESOLVE_SYMBOL
			if (symbol) {
				bool demangled = false;
				std::string str(symbol[i]);
				size_t p = str.find('(');
				if (p != std::string::npos) {
					size_t q = str.find('+', p + 1);
					if (q != std::string::npos) {
						std::string sep1 = str.substr(0, p);
						std::string sep2 = str.substr(p + 1, q - p - 2);
						std::string sep3 = str.substr(q);
						int status;
						char *demangle = abi::__cxa_demangle(sep2.c_str(), 0, 0, &status);
						stacktrace_local::AutoFree afDemangle(demangle);
						if (status == 0) {
							out += sep1 + '(' + demangle + sep3;
							demangled = true;
						}
					}
				}
				if (!demangled) {
					out += symbol[i];
				}
			} else
#endif
			{
				out += " (" + cybozu::itohex(reinterpret_cast<uintptr_t>(data_[i]), false) + ")";
			}
			if (i < traceNum - 1) out += "\n";
		}
#endif
		return out;
	}
};

} // cybozu

#undef CYBOZU_STACKTRACE_RESOLVE_SYMBOL
