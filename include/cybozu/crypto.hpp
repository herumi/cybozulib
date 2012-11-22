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

namespace crypto {

class Hash {
public:
	enum Name {
		N_SHA1,
		N_SHA224,
		N_SHA256,
		N_SHA384,
		N_SHA512
	};
private:
	Name name_;
	union {
		SHA_CTX sha1;
		SHA256_CTX sha256;
		SHA512_CTX sha512;
	} ctx_;
public:
	static inline size_t getSize(Name name)
	{
		switch (name) {
		case N_SHA1:   return SHA_DIGEST_LENGTH;
		case N_SHA224: return SHA224_DIGEST_LENGTH;
		case N_SHA256: return SHA256_DIGEST_LENGTH;
		case N_SHA384: return SHA384_DIGEST_LENGTH;
		case N_SHA512: return SHA512_DIGEST_LENGTH;
		default:
			throw cybozu::Exception("crypto:Hash:getSize") << name;
		}
	}
	static inline const char *getName(Name name)
	{
		switch (name) {
		case N_SHA1:   return "sha1";
		case N_SHA224: return "sha224";
		case N_SHA256: return "sha256";
		case N_SHA384: return "sha384";
		case N_SHA512: return "sha512";
		default:
			throw cybozu::Exception("crypto:Hash:getName") << name;
		}
	}
	explicit Hash(Name name = N_SHA1)
		: name_(name)
	{
		reset();
	}
	void update(const char *buf, size_t bufSize)
	{
		switch (name_) {
		case N_SHA1:   SHA1_Update(&ctx_.sha1, buf, bufSize);     break;
		case N_SHA224: SHA224_Update(&ctx_.sha256, buf, bufSize); break;
		case N_SHA256: SHA256_Update(&ctx_.sha256, buf, bufSize); break;
		case N_SHA384: SHA384_Update(&ctx_.sha512, buf, bufSize); break;
		case N_SHA512: SHA512_Update(&ctx_.sha512, buf, bufSize); break;
		}
	}
	void update(const std::string& buf)
	{
		update(buf.c_str(), buf.size());
	}
	void reset()
	{
		switch (name_) {
		case N_SHA1:   SHA1_Init(&ctx_.sha1);     break;
		case N_SHA224: SHA224_Init(&ctx_.sha256); break;
		case N_SHA256: SHA256_Init(&ctx_.sha256); break;
		case N_SHA384: SHA384_Init(&ctx_.sha512); break;
		case N_SHA512: SHA512_Init(&ctx_.sha512); break;
		default:
			throw cybozu::Exception("crypto:Hash:rset") << name_;
		}
	}
	/*
		@note clear inner buffer after calling digest
	*/
	std::string digest(const char *buf, size_t bufSize)
	{
		update(buf, bufSize);
		unsigned char md[128];
		switch (name_) {
		case N_SHA1:   SHA1_Final(md, &ctx_.sha1);     break;
		case N_SHA224: SHA224_Final(md, &ctx_.sha256); break;
		case N_SHA256: SHA256_Final(md, &ctx_.sha256); break;
		case N_SHA384: SHA384_Final(md, &ctx_.sha512); break;
		case N_SHA512: SHA512_Final(md, &ctx_.sha512); break;
		default:
			throw cybozu::Exception("crypto:Hash:digest") << name_;
		}
		reset();
		return std::string(reinterpret_cast<const char*>(md), getSize(name_));
	}
	std::string digest(const std::string& buf = "")
	{
		return digest(buf.c_str(), buf.size());
	}
	static inline std::string digest(Name name, const char *buf, size_t bufSize)
	{
		unsigned char md[128];
		const unsigned char *src = reinterpret_cast<const unsigned char *>(buf);
		switch (name) {
		case N_SHA1:   SHA1(src, bufSize, md);   break;
		case N_SHA224: SHA224(src, bufSize, md); break;
		case N_SHA256: SHA256(src, bufSize, md); break;
		case N_SHA384: SHA384(src, bufSize, md); break;
		case N_SHA512: SHA512(src, bufSize, md); break;
		default:
			throw cybozu::Exception("crypt:Hash:digest") << name;
		}
		return std::string(reinterpret_cast<const char*>(md), getSize(name));
	}
	static inline std::string digest(Name name, const std::string& buf)
	{
		return digest(name, buf.c_str(), buf.size());
	}
};

class Hmac {
	const EVP_MD *evp_;
public:
	explicit Hmac(Hash::Name name = Hash::N_SHA1)
	{
		switch (name) {
		case Hash::N_SHA1: evp_ = EVP_sha1(); break;
		case Hash::N_SHA224: evp_ = EVP_sha224(); break;
		case Hash::N_SHA256: evp_ = EVP_sha256(); break;
		case Hash::N_SHA384: evp_ = EVP_sha384(); break;
		case Hash::N_SHA512: evp_ = EVP_sha512(); break;
		default:
			throw cybozu::Exception("crypto:Hmac:") << name;
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
		throw cybozu::Exception("crypto::Hamc::eval");
	}
};

class Cipher {
	const EVP_CIPHER *cipher_;
	EVP_CIPHER_CTX ctx_;
public:
	enum Name {
		N_AES128_CBC,
		N_AES256_CBC
	};
	enum Mode {
		Decoding,
		Encoding
	};
	explicit Cipher(Name name = N_AES128_CBC)
		: cipher_(0)
	{
		EVP_CIPHER_CTX_init(&ctx_);
		switch (name) {
		case N_AES128_CBC: cipher_ = EVP_aes_128_cbc(); break;
		case N_AES256_CBC: cipher_ = EVP_aes_256_cbc(); break;
		default:
			throw cybozu::Exception("crypto:Cipher:Cipher:name") << (int)name;
		}
	}
	~Cipher()
	{
		EVP_CIPHER_CTX_cleanup(&ctx_);
	}
	/*
		@note don't use padding = true
	*/
	void setup(Mode mode, const std::string& key, const std::string& iv, bool padding = false)
	{
		const int keyLen = static_cast<int>(key.size());
		const int expectedKeyLen = EVP_CIPHER_key_length(cipher_);
		if (keyLen != expectedKeyLen) {
			throw cybozu::Exception("crypto:Cipher:setup:keyLen") << keyLen << expectedKeyLen;
		}

		int ret = EVP_CipherInit_ex(&ctx_, cipher_, NULL, (const uint8_t*)&key[0], (const uint8_t*)&iv[0], mode == Encoding ? 1 : 0);
		if (ret != 1) {
			throw cybozu::Exception("crypto:Cipher:setup:EVP_CipherInit_ex") << ret;
		}
		ret = EVP_CIPHER_CTX_set_padding(&ctx_, padding ? 1 : 0);
		if (ret != 1) {
			throw cybozu::Exception("crypto:Cipher:setup:EVP_CIPHER_CTX_set_padding") << ret;
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
