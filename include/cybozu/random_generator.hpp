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
	union cu32 {
		unsigned char c[4];
		uint32_t i;
	};
	union cu64 {
		unsigned char c[8];
		uint64_t i;
	};
public:
	uint32_t operator()()
	{
		return get32();
	}
	uint32_t get32()
	{
		cu32 cu;
		read(cu.c, 4);
		return cu.i;
	}
	uint64_t get64()
	{
		cu64 cu;
		read(cu.c, 8);
		return cu.i;
	}
#ifdef _WIN32
	RandomGenerator()
		: prov_(0)
	{
		DWORD flagTbl[] = { 0, CRYPT_NEWKEYSET };
		for (int i = 0; i < 2; i++) {
			if (CryptAcquireContext(&prov_, NULL, NULL, PROV_RSA_FULL, flagTbl[i]) != 0) return;
		}
		throw cybozu::Exception("randomgenerator");
	}
	void read(void *buf, int byteSize)
	{
		if (CryptGenRandom(prov_, byteSize, static_cast<BYTE*>(buf)) == 0) {
			throw cybozu::Exception("randomgenerator:read") << byteSize;
		}
	}
	~RandomGenerator()
	{
		if (prov_) {
			CryptReleaseContext(prov_, 0);
		}
	}
private:
	HCRYPTPROV prov_;
#else
	RandomGenerator()
		: fd_(::open("/dev/urandom", O_RDONLY, 0))
	{
		if (fd_ < 0) throw cybozu::Exception("randomgenerator");
	}
	~RandomGenerator()
	{
		if (fd_ >= 0) ::close(fd_);
	}
	void read(void *buf, int byteSize)
	{
		if (::read(fd_, buf, byteSize) != byteSize) {
			throw cybozu::Exception("randomgenerator:read") << byteSize;
		}
	}
#endif
private:
	int fd_;
};

} // cybozu
