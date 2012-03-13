#pragma once
/**
	@file
	@brief tiny wrapper of openssl class

	Copyright (C) 2007-2012 Cybozu Labs, Inc., all rights reserved.
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
	#pragma comment(lib, "ssleay32.lib")
	#pragma comment(lib, "libeay32.lib")
#endif

namespace cybozu {

struct SslException : public cybozu::Exception {
	SslException() : cybozu::Exception("ssl") { }
};

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
			cybozu::SslException e;
			e << "Ctx";
			throw e;
		}
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
			cybozu::SslException e;
			e << "ClientSocket";
			throw e;
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
		isOK = SSL_set_fd(ssl_, soc_.sd_) != 0;
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
		ssize_t ret = SSL_read(ssl_, buf, bufSize);
		if (ret < 0) {
			Engine::getInstance().putError();
		}
		return ret;
	}
	ssize_t write(const char *buf, size_t bufSize)
	{
		ssize_t ret = SSL_write(ssl_, buf, bufSize);
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
