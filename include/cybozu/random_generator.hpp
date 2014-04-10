#pragma once
/**
	@file
	@brief pseudrandom generator
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/

#include <cybozu/exception.hpp>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <wincrypt.h>
#pragma comment (lib, "advapi32.lib")
#else
#include <sys/types.h>
#include <fcntl.h>
#endif

namespace cybozu {

class RandomGenerator {
	RandomGenerator(const RandomGenerator&);
	void operator=(const RandomGenerator&);
public:
	uint32_t operator()()
	{
		return get32();
	}
	uint32_t get32()
	{
		uint32_t ret;
		read(&ret, sizeof(ret));
		return ret;
	}
	uint64_t get64()
	{
		uint64_t ret;
		read(&ret, sizeof(ret));
		return ret;
	}
#ifdef _WIN32
	RandomGenerator()
		: prov_(0)
		, pos_(bufSize)
	{
		DWORD flagTbl[] = { 0, CRYPT_NEWKEYSET };
		for (int i = 0; i < 2; i++) {
			if (CryptAcquireContext(&prov_, NULL, NULL, PROV_RSA_FULL, flagTbl[i]) != 0) return;
		}
		throw cybozu::Exception("randomgenerator");
	}
	void read_inner(void *buf, size_t byteSize)
	{
		if (CryptGenRandom(prov_, static_cast<DWORD>(byteSize), static_cast<BYTE*>(buf)) == 0) {
			throw cybozu::Exception("randomgenerator:read") << byteSize;
		}
	}
	~RandomGenerator()
	{
		if (prov_) {
			CryptReleaseContext(prov_, 0);
		}
	}
	void read(void *buf, size_t byteSize)
	{
		if (byteSize > bufSize) {
			read_inner(buf, byteSize);
		} else {
			if (pos_ + byteSize > bufSize) {
				read_inner(buf_, bufSize);
				pos_ = 0;
			}
			memcpy(buf, buf_ + pos_, byteSize);
			pos_ += byteSize;
		}
	}
private:
	HCRYPTPROV prov_;
	static const size_t bufSize = 1024;
	char buf_[bufSize];
	size_t pos_;
#else
	RandomGenerator()
		: fp_(::fopen("/dev/urandom", "rb"))
	{
		if (!fp_) throw cybozu::Exception("randomgenerator");
	}
	~RandomGenerator()
	{
		if (fp_) ::fclose(fp_);
	}
	void read(void *buf, size_t byteSize)
	{
		if (::fread(buf, 1, byteSize, fp_) != byteSize) {
			throw cybozu::Exception("randomgenerator:read") << byteSize;
		}
	}
#endif
private:
	FILE *fp_;
};

} // cybozu
