#pragma once
/**
	@file
	@brief tiny wrapper of openssl class

	Copyright (C) 2007 Cybozu Labs, Inc., all rights reserved.
*/
#include <cybozu/socket.hpp>
#include <cybozu/exception.hpp>
#include <openssl/engine.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
//#include <openssl/crypto.h>
//#include <openssl/conf.h>
#ifdef _WIN32
//	#pragma warning(push)
//	#pragma warning(disable : 4996)
//	#include <openssl/applink.c>
//	#pragma warning(pop)
	#pragma comment(lib, "ssleay32.lib")
	#pragma comment(lib, "libeay32.lib")
#endif

namespace cybozu {

namespace ssl {

struct Engine {
	Engine()
	{
		SSL_load_error_strings();
		SSL_library_init();
	}
	~Engine()
	{
		ERR_remove_state(0);
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
		ENGINE_cleanup();
		ERR_free_strings();
//		CONF_modules_unload(1); // openssl/conf.h
		/* this decreases one leak, but is this correct? */
		OPENSSL_free(SSL_COMP_get_compression_methods());
	}
	static Engine& getInstance()
	{
		static Engine engine;
		return engine;
	}
	void putError()
	{
		ERR_print_errors_fp(stderr);
	}
};

class Ctx {
	Engine& engine_;
	SSL_CTX *ctx_;
	Ctx(const Ctx&);
	void operator=(const Ctx&);
public:
	explicit Ctx()
		: engine_(Engine::getInstance())
		, ctx_(SSL_CTX_new(SSLv3_client_method()))
	{
		if (ctx_ == 0) {
			engine_.putError();
			fprintf(stderr, "SSL_CTX_new:err");
			exit(1);
		}
#if 0
		if (SSL_CTX_set_cipher_list(ctx_, "ALL:eNULL") == 0) {
			engine_.putError();
			fprintf(stderr, "SSL_CTX_set_cipher_list:err");
			exit(1);
		}
#endif
	}
	~Ctx()
	{
		if (ctx_) SSL_CTX_free(ctx_);
	}
	SSL_CTX* get() const { return ctx_; }
};

namespace impl {
template<int dummy = 0>
struct InstanceIsHere { static Ctx ctx_; };

template<int dummy>
Ctx InstanceIsHere<dummy>::ctx_;

struct DummyCall {
	DummyCall() { InstanceIsHere<>::ctx_.get(); }
};

} // impl

class ClientSocket {
	cybozu::Socket soc_;
	SSL *ssl_;

	ClientSocket(const ClientSocket&);
	void operator=(const ClientSocket&);
public:
	ClientSocket()
		: ssl_(SSL_new(impl::InstanceIsHere<>::ctx_.get()))
	{
		if (ssl_ == 0) {
			throw cybozu::Exception("ssl:ClientSocket");
		}
	}
	bool connect(const std::string& address, unsigned short port)
	{
		SocketAddr addr;
		if (!addr.setName(address)) {
			return false;
		}
		addr.setPort(port);
		return connect(addr);
	}
	bool connect(const cybozu::SocketAddr& addr)
	{
		if (!soc_.connect(addr)) return false;
		bool isOK;
#ifdef _WIN32
		if (soc_.sd_ > INT_MAX) {
			throw cybozu::Exception("ssl:large socket handle") << soc_.sd_;
		}
#endif
		isOK = SSL_set_fd(ssl_, static_cast<int>(soc_.sd_)) != 0;
		if (!isOK) goto ERR_EXIT;
		isOK = SSL_connect(ssl_) == 1;
		if (!isOK) goto ERR_EXIT;
		return true;
	ERR_EXIT:
		Engine::getInstance().putError();
		return false;
	}
	ssize_t read(char *buf, size_t bufSize)
	{
		if (bufSize > 0x7fffffffU) {
			return -1;
		}
		ssize_t ret = SSL_read(ssl_, buf, static_cast<int>(bufSize));
		if (ret < 0) {
			Engine::getInstance().putError();
		}
		return ret;
	}
	ssize_t write(const char *buf, size_t bufSize)
	{
		if (bufSize > 0x7fffffffU) {
			return -1;
		}
		ssize_t ret = SSL_write(ssl_, buf, static_cast<int>(bufSize));
		if (ret < 1) {
			Engine::getInstance().putError();
		}
		return ret;
	}
	bool close()
	{
		bool isOK = true;
		if (soc_.isValid()) {
			isOK = SSL_shutdown(ssl_) == 1;
			if (!isOK) {
				Engine::getInstance().putError();
			}
			soc_.close();
		}
		return isOK;
	}
	~ClientSocket()
	{
		if (ssl_) {
			close();
			SSL_free(ssl_);
			ssl_ = 0;
		}
	}
};

} // cybozu::ssl

} // cybozu
