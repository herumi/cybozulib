#pragma once
/**
	@file
	@brief stacktrace class
	Copyright (C) 2008 Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include <vector>
#include <cybozu/itoa.hpp>
#include <stdlib.h>
#include <iosfwd>

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
	// with -lbfd ; libbfd.a is in binutils-dev
	#ifdef CYBOZU_STACKTRACE_WITH_BFD_GPL
		#include <bfd.h>
	#endif
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

#elif defined(CYBOZU_STACKTRACE_WITH_BFD_GPL)

struct Bfd {
	struct bfd *bfd;
	explicit Bfd(const std::string& fileName = "")
		: bfd(0)
	{
		bfd_init();

		const char *file = fileName.c_str();
#if 0
		if (*file == '\0') {
			file = "/proc/self/exe";
		}
#else
		/*
			/proc/self/exe does not point to self-binary on valgrind,
			so use readlink to get correct self-binary
		*/
		std::string path;
		if (*file == '\0') {
			path.resize(4096);
			int ret = readlink("/proc/self/exe", &path[0], path.size() - 2);
			if (ret <= 0) {
				perror("ERR:cybozu:StackTrace:Bfd:readlink");
				return;
			}
			path.resize(ret);
			file = path.c_str();
		}
#endif
		bfd = bfd_openr(file, 0);
		if (bfd == 0) {
			perror("ERR:cybozu:StackTrace:Bfd:bfd_opener");
			return;
		}
		if (!bfd_check_format(bfd, bfd_object)) {
			perror("ERR:cybozu:StackTrace:Bfd:bfd_check_format");
			return;
		}
	}
	~Bfd()
	{
		if (bfd == 0) return;
		if (!bfd_close(bfd)) {
			fprintf(stderr, "ERR:cybozu:StackTrace:Bfd:bfd_close\n");
		}
	}
	bool getInfo(std::string* pFile, std::string* pFunc, int *pLine, const void *addr)
	{
		if (bfd == 0) return false;
		Data data(addr, pFile, pFunc);
		bfd_map_over_sections(bfd, findAddress, &data);
		*pLine = data.line;
		return data.found;
	}
	static inline Bfd& getInstance() {
		static Bfd bfd;
		return bfd;
	}
private:
	struct Data {
		bfd_vma pc;
		std::string *pFile;
		std::string *pFunc;
		unsigned int line;
		bool found;
		Data(const void *addr, std::string *pFile, std::string *pFunc)
			: pc(bfd_vma(addr))
			, pFile(pFile)
			, pFunc(pFunc)
			, line(0)
			, found(false)
		{
		}
	};
	static inline void findAddress(struct bfd *bfd, asection *section, void *self)
	{
		Data *data = (Data*)self;
		if (data->found) return;
		if (section == 0) return;
		bfd_vma vma = bfd_get_section_vma(bfd, section);
		if (data->pc < vma) return;
		bfd_size_type size = bfd_get_section_size(section);
		if (data->pc >= vma + size) return;
		const char *file;
		const char *func;
		data->found = bfd_find_nearest_line(bfd, section, NULL, data->pc - vma, &file, &func, &data->line);
		if (!data->found) return;
		if (file) *data->pFile = file;
		if (func) *data->pFunc = func;
	}
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
#ifdef __GNUC__
	void demangle(std::string& out, const char *func) const
	{
		int status;
		char *demangle = abi::__cxa_demangle(func, 0, 0, &status);
		stacktrace_local::AutoFree afDemangle(demangle);
		if (status == 0) {
			out = demangle;
		} else {
			out = func;
		}
	}
#endif
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
			std::string funcName;
#ifdef CYBOZU_STACKTRACE_WITH_BFD_GPL
			{
				std::string fileName;
				int line;
				if (stacktrace_local::Bfd::getInstance().getInfo(&fileName, &funcName, &line, data_[i])) {
					demangle(funcName, funcName.c_str());
					out += fileName;
					out += ':';
					out += cybozu::itoa(line);
					out += ' ';
				}
			}
#endif
#ifdef CYBOZU_STACKTRACE_RESOLVE_SYMBOL
			if (symbol) {
				std::string str(symbol[i]);
				str += '\0';
				size_t p = str.find('(');
				const char *obj = 0, *addr = 0;
				if (p != std::string::npos) {
					size_t q = str.find('+', p + 1);
					if (q != std::string::npos) {
						obj = &str[0]; str[p] = '\0';
						const char *const func = &str[p + 1]; str[q] = '\0';
						addr = &str[q + 1];
						if (funcName.empty()) {
							demangle(funcName, func);
						}
					}
				}
				bool doPrint = false;
				if (obj && addr) {
					out += obj;
					out += '(';
					out += funcName;
					out += '+';
					out += addr;
					out += ' ';
					doPrint = true;
				} else if (!funcName.empty()) {
					out += funcName;
					out += ' ';
				}
				if (!doPrint) {
					out += symbol[i];
					out += ' ';
				}
			} else
#endif
			{
				out += "(" + cybozu::itohex(reinterpret_cast<uintptr_t>(data_[i]), false) + ")";
			}
			if (i < traceNum - 1) out += "\n";
		}
#endif
		return out;
	}
	bool empty() const throw() { return data_.empty(); }
	void clear() throw() { data_.clear(); }
};

inline std::ostream& operator<<(std::ostream& os, const StackTrace& self)
{
	return os << self.toString();
}

} // cybozu

#undef CYBOZU_STACKTRACE_RESOLVE_SYMBOL
