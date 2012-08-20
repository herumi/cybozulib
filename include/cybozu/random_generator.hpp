#pragma once
/**
	@file
	@brief pseudrandom generator
	@author MITSUNARI Shigeo

	Copyright (C) 2008 Cybozu Labs, Inc., all rights reserved.
*/

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <wincrypt.h>
#pragma comment (lib, "advapi32.lib")
#else
#include <sys/types.h>
#include <fcntl.h>
#endif
#include <stdexcept>

namespace cybozu {

class RandomGenerator {
	RandomGenerator(const RandomGenerator&);
	void operator=(const RandomGenerator&);
	union ci {
		unsigned char b[4];
		unsigned int i;
	};
public:
	unsigned int operator()()
	{
		ci ci;
		read(ci.b, 4);
		return ci.i;
	}
#ifdef _WIN32
	RandomGenerator()
		: prov_(0)
	{
		DWORD flagTbl[] = { 0, CRYPT_NEWKEYSET };
		for (int i = 0; i < 2; i++) {
			if (CryptAcquireContext(&prov_, NULL, NULL, PROV_RSA_FULL, flagTbl[i]) != 0) return;
		}
		throw std::runtime_error("RandomGenerator:can't init");
	}
	void read(void *buf, int byteSize)
	{
		if (CryptGenRandom(prov_, byteSize, reinterpret_cast<BYTE*>(buf)) == 0) {
			throw std::runtime_error("RandomGenerator::can't read");
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
		if (fd_ < 0) throw std::runtime_error("RandomGenerator:can't init");
	}
	~RandomGenerator()
	{
		if (fd_ >= 0) ::close(fd_);
	}
	void read(void *buf, int byteSize)
	{
		if (::read(fd_, buf, byteSize) != byteSize) {
			throw std::runtime_error("RandomGenerator::can't read");
		}
	}
#endif
private:
	int fd_;
};

} // cybozu

