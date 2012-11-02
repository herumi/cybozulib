#pragma once
/**
	@file
	@brief wrap openssl
	Copyright (C) 2012 Cybozu Labs, Inc., all rights reserved.
*/

#include <cybozu/exception.hpp>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#ifdef _MSC_VER
	#pragma comment(lib, "libeay32.lib")
#endif

namespace cybozu {

struct cryptoException : public cybozu::Exception {
	cryptoException() : cybozu::Exception("crypt") { }
};

namespace crypto {

class Hmac {
	const EVP_MD *evp_;
public:
	explicit Hmac(int mode = 1)
	{
		switch (mode) {
		case 1: evp_ = EVP_sha1(); break;
		case 224: evp_ = EVP_sha224(); break;
		case 256: evp_ = EVP_sha256(); break;
		case 384: evp_ = EVP_sha384(); break;
		case 512: evp_ = EVP_sha512(); break;
		default:
			cryptoException e; e << "Hmac" << mode;
			throw e;
		}
	}
	std::string eval(const std::string& key, const std::string& data)
	{
		std::string out(evp_->md_size + 1, 0);
		unsigned int outLen = 0;
		if (HMAC(evp_, key.c_str(), static_cast<int>(key.size()),
			(const uint8_t *)data.c_str(), data.size(), (uint8_t *)&out[0], &outLen)) {
			out.resize(outLen);
			return out;
		}
		cryptoException e; e << "Hmac::eval";
		throw e;
	}
};

class Sha {
	enum {
		M_SHA1,
		M_SHA224,
		M_SHA256,
		M_SHA384,
		M_SHA512
	} sel_;
	union {
		SHA_CTX sha1;
		SHA256_CTX sha256;
		SHA512_CTX sha512;
	} ctx_;
public:
	explicit Sha(int mode = 1)
	{
		switch (mode) {
		case 1:
			sel_ = M_SHA1;
			SHA1_Init(&ctx_.sha1);
			break;
		case 224:
			sel_ = M_SHA224;
			SHA224_Init(&ctx_.sha256);
			break;
		case 256:
			sel_ = M_SHA256;
			SHA256_Init(&ctx_.sha256);
			break;
		case 384:
			sel_ = M_SHA384;
			SHA384_Init(&ctx_.sha512);
			break;
		case 512:
			sel_ = M_SHA512;
			SHA512_Init(&ctx_.sha512);
			break;
		default:
			cryptoException e; e << "Sha" << mode;
			throw e;
		}
	}
	void update(const char *buf, size_t bufSize)
	{
		switch (sel_) {
		case M_SHA1:   SHA1_Update(&ctx_.sha1, buf, bufSize);     break;
		case M_SHA224: SHA224_Update(&ctx_.sha256, buf, bufSize); break;
		case M_SHA256: SHA256_Update(&ctx_.sha256, buf, bufSize); break;
		case M_SHA384: SHA384_Update(&ctx_.sha512, buf, bufSize); break;
		case M_SHA512: SHA512_Update(&ctx_.sha512, buf, bufSize); break;
		}
	}
	void update(const std::string& buf)
	{
		update(&buf[0], buf.size());
	}
	std::string digest(const char *buf, size_t bufSize)
	{
		update(buf, bufSize);
		unsigned char md[128];
		const char *p = reinterpret_cast<const char*>(md);
		switch (sel_) {
		case M_SHA1:   SHA1_Final(md, &ctx_.sha1);     return std::string(p, SHA_DIGEST_LENGTH);
		case M_SHA224: SHA224_Final(md, &ctx_.sha256); return std::string(p, SHA224_DIGEST_LENGTH);
		case M_SHA256: SHA256_Final(md, &ctx_.sha256); return std::string(p, SHA256_DIGEST_LENGTH);
		case M_SHA384: SHA384_Final(md, &ctx_.sha512); return std::string(p, SHA384_DIGEST_LENGTH);
		case M_SHA512: SHA512_Final(md, &ctx_.sha512); return std::string(p, SHA512_DIGEST_LENGTH);
		}
		cryptoException e; e << "Sha::digest";
		throw e;
	}
	std::string digest(const std::string& str)
	{
		return digest(&str[0], str.size());
	}
};

class Cipher {
	EVP_CIPHER_CTX ctx_;
public:
	enum Mode {
		Decoding,
		Encoding
	} mode_;
	Cipher()
		: mode_(Encoding)
	{
		EVP_CIPHER_CTX_init(&ctx_);
	}
	~Cipher()
	{
		EVP_CIPHER_CTX_cleanup(&ctx_);
	}
	/*
		don't use padding = true
	*/
	void setup(Mode mode, const std::string& key, const std::string& iv, bool padding = false)
	{
		mode_ = mode;
		const EVP_CIPHER *cipher = 0;
		const int keyLen = static_cast<int>(key.size());
		switch (keyLen) {
		case 128/8:
			cipher = EVP_aes_128_cbc();
			break;
		case 256/8:
			cipher = EVP_aes_256_cbc();
			break;
		default:
			cryptoException e; e << "Aes::setup:keyLen" << keyLen;
			throw e;
		}
		const int expectedKeyLen = EVP_CIPHER_key_length(cipher);
		if (keyLen != expectedKeyLen) {
			cryptoException e; e << "Aes::setup::keyLen" << keyLen << expectedKeyLen;
			throw e;
		}

		int ret = EVP_CipherInit_ex(&ctx_, cipher, NULL, (const uint8_t*)&key[0], (const uint8_t*)&iv[0], mode == Encoding ? 1 : 0);
		if (ret != 1) {
			cryptoException e; e << "Aes::setup:init" << ret;
		}
		ret = EVP_CIPHER_CTX_set_padding(&ctx_, padding ? 1 : 0);
		if (ret != 1) {
			cryptoException e; e << "Aes::setup:padding" << ret;
		}
	}
	/*
		the size of outBuf must be larger than inBufSize + blockSize
		@retval positive or 0 : writeSize(+blockSize)
		@retval -1 : error
	*/
	int update(char *outBuf, const char *inBuf, int inBufSize)
	{
		int outLen = 0;
		int ret = EVP_CipherUpdate(&ctx_, (uint8_t*)outBuf, &outLen, (const uint8_t*)inBuf, inBufSize);
		if (ret != 1) return -1;
		return outLen;
	}
	/*
		return -1 if padding
		@note don't use
	*/
	int finalize(char *outBuf)
	{
		int outLen = 0;
		int ret = EVP_CipherFinal_ex(&ctx_, (uint8_t*)outBuf, &outLen);
		if (ret != 1) return -1;
		return outLen;
	}
};

} }	// cybozu::crypto
