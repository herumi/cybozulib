#pragma once
/**
	@file
	@brief tiny socket class

	Copyright (C) 2007 Cybozu Labs, Inc., all rights reserved.
	@author MITSUNARI Shigeo
*/
#include <errno.h>
#include <assert.h>
#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h> // for socklen_t
	#pragma comment(lib, "ws2_32.lib")
	#pragma comment(lib, "iphlpapi.lib")
	#pragma warning(push)
	#pragma warning(disable : 4127) // constant condition
#else
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netinet/tcp.h>
	#include <netdb.h>
	#include <memory.h>
	#include <signal.h>
#endif
#ifndef NDEBUG
	#include <stdio.h>
#endif

#include <cybozu/atomic.hpp>
#include <cybozu/exception.hpp>
#include <string>

namespace cybozu {

namespace ssl {
class ClientSocket;
};

namespace socket_local {

#ifdef _WIN32
	typedef SOCKET SocketHandle;
#else
	typedef int SocketHandle;
#endif

inline ssize_t write(SocketHandle sd, const char *buf, size_t len)
{
#ifdef _WIN32
	assert(len < (1U << 31));
	return ::send(sd, buf, (int)len, 0);
#else
	ssize_t ret;
	do {
		ret = ::write(sd, buf, len);
	} while (ret < 0 && errno == EINTR);
	return ret;
#endif
}
inline ssize_t read(SocketHandle sd, char *buf, size_t len)
{
#ifdef _WIN32
	assert(len < (1U << 31));
	return ::recv(sd, buf, (int)len, 0);
#else
	ssize_t ret;
	do {
		ret = ::read(sd, buf, len);
	} while (ret < 0 && errno == EINTR);
	return ret;
#endif
}
inline bool close(SocketHandle sd)
{
#ifdef _WIN32
	// ::shutdown(sd, SD_SEND);
	// shutdown is called in closesocket
	return ::closesocket(sd) == 0;
#else
	return ::close(sd) == 0;
#endif
}

struct InitTerm {
	/** call once for init */
	InitTerm()
	{
#ifdef _WIN32
		WSADATA data;
		::WSAStartup(MAKEWORD(2, 2), &data);
#else
		::signal(SIGPIPE, SIG_IGN);
#endif
	}
	/** call once for term */
	~InitTerm()
	{
#ifdef _WIN32
		::WSACleanup();
#endif
	}
	void dummyCall() { }
};

template<int dummy = 0>
struct InstanceIsHere { static InitTerm it_; };

template<int dummy>
InitTerm InstanceIsHere<dummy>::it_;

struct DummyCall {
	DummyCall() { InstanceIsHere<>::it_.dummyCall(); }
};

} // cybozu::socket_local

class SocketAddr {
	struct sockaddr_storage addr_;
	socklen_t addrlen_;
	int family_;
public:
	SocketAddr()
	{
	}
	SocketAddr(const std::string& address, uint16_t port)
	{
		set(address, port);
	}
	bool set(const std::string& address, uint16_t port)
	{
		char portStr[32];
		CYBOZU_SNPRINTF(portStr, sizeof(portStr), "%d", port);
		memset(&addr_, 0, sizeof(addr_));
		addrlen_ = 0;
		family_ = 0;

		struct addrinfo *result = 0;
		struct addrinfo hints;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = 0; // AI_PASSIVE;
		int s = getaddrinfo(address.c_str(), portStr, &hints, &result);
		// s == EAI_AGAIN
		if (s) {
			hints.ai_family = AF_INET6;
			hints.ai_flags |= AI_V4MAPPED;
			int s = getaddrinfo(address.c_str(), portStr, &hints, &result);
			if (s) {
				return false;
			}
		}
		bool found = false;
		for (const struct addrinfo *p = result; p; p = p->ai_next) {
			const int family = p->ai_family;
			if (family == AF_INET || family == AF_INET6) {
				if (p->ai_addrlen > sizeof(addr_)) {
					throw cybozu::Exception("SocketAddr:setName") << address << p->ai_addrlen;
				}
				memcpy(&addr_, p->ai_addr, p->ai_addrlen);
				addrlen_ = (socklen_t)p->ai_addrlen;
				family_ = family;
				found = true;
				break;
			}
		}
		freeaddrinfo(result);
		return found;
	}
	socklen_t getSize() const { return addrlen_; }
	int getFamily() const { return family_; }
	const struct sockaddr *get() const { return (const struct sockaddr*)&addr_; }
};
/*
	socket class
	@note ower is moved if copied
*/
class Socket {
	friend class cybozu::ssl::ClientSocket;
private:
	cybozu::socket_local::SocketHandle sd_;
#ifdef WIN32
	bool setTimeout(int type, int msec)
	{
		return setSocketOption(type, msec);
	}
	int getTimeout(int type) const
	{
		int val;
		if (getSocketOption(type, &val)) {
			return val;
		} else {
			return -1;
		}
	}
#else
	bool setTimeout(int type, int msec)
	{
		struct timeval t;
		t.tv_sec = msec / 1000;
		t.tv_usec = (msec % 1000) * 1000;
		return ::setsockopt(sd_, SOL_SOCKET, type, cybozu::cast<const char*>(&t), sizeof(t)) == 0;
	}
	int getTimeout(int type) const
	{
		struct timeval t;
		socklen_t len = sizeof(t);
		if (::getsockopt(sd_, SOL_SOCKET, type, cybozu::cast<char*>(&t), &len) == 0) {
			return t.tv_sec * 1000 + t.tv_usec / 1000; /* msec */
		} else {
			return -1;
		}
	}
#endif
	int getError() const
	{
#ifdef _WIN32
		return (::WSAGetLastError() == WSAETIMEDOUT) ? TIMEOUT : ERR;
#else
		return (errno == EAGAIN || errno == EWOULDBLOCK) ? TIMEOUT : ERR;
#endif
	}
public:
	enum {
		NOERR = 1,
		CLOSED = 0,
		TIMEOUT = -1,
		ERR = -2
	};
#ifndef _WIN32
	static const int INVALID_SOCKET = -1;
#endif
	Socket()
		: sd_(INVALID_SOCKET)
	{
	}

	bool isValid() const { return sd_ != INVALID_SOCKET; }

	Socket(Socket& rhs)
		: sd_(rhs.sd_)
	{
		rhs.sd_ = INVALID_SOCKET;
	}
	void operator=(Socket& rhs)
	{
		close();
		sd_ = cybozu::AtomicExchange(&rhs.sd_, INVALID_SOCKET);
	}

	~Socket()
	{
		close();
	}

	bool close()
	{
		cybozu::socket_local::SocketHandle sd = cybozu::AtomicExchange(&sd_, INVALID_SOCKET);
		if (sd == INVALID_SOCKET) return true;
		return cybozu::socket_local::close(sd);
	}

	/*!
		receive data
		@param buf [out] receive buffer
		@param bufSize [in] receive buffer size(byte)
		@retval > 0 recived size
		@retval TIMEOUT timeout
		@retval CLOSED socket is closed
		@retval ERR otherwise
	*/
	ssize_t read(char *buf, size_t bufSize)
	{
		ssize_t ret = cybozu::socket_local::read(sd_, buf, bufSize);
		if (ret >= 0) return ret;
		return getError();
	}

	/*!
		read all data unless timeout
		@param buf [out] receive buffer
		@param bufSize [in] receive buffer size(byte)
	*/
	void readAll(void *buf, size_t bufSize)
	{
		char *p = (char *)buf;
		while (bufSize > 0) {
			ssize_t ret = read(p, bufSize);
			if (ret <= 0) {
				throw cybozu::Exception("Socket:readAll") << ret;
			}
			p += ret;
			bufSize -= ret;
		}
	}
	/*!
		write all data unless timeout
		@param buf [out] send buffer
		@param bufSize [in] send buffer size(byte)
	*/
	void write(const void *buf, size_t bufSize)
	{
		const char *p = (const char *)buf;
		if (bufSize == 0) {
			ssize_t ret = cybozu::socket_local::write(sd_, p, 0);
			if (ret < 0) throw cybozu::Exception("Socket:write:bufSize = 0");
			return;
		}
		while (bufSize > 0) {
			ssize_t ret = cybozu::socket_local::write(sd_, p, bufSize);
			if (ret <= 0) {
				throw cybozu::Exception("Socket:writeAll") << ret;
			}
			p += ret;
			bufSize -= ret;
		}
	}
	/**
		connect to address:port
		@param address [in] address
		@param port [in] port
	*/
	bool connect(const std::string& address, uint16_t port)
	{
		SocketAddr addr;
		if (!addr.set(address, port)) {
			return false;
		}
		return connect(addr);
	}
	/**
		connect to resolved socket addr
	*/
	bool connect(const cybozu::SocketAddr& addr)
	{
		sd_ = socket(addr.getFamily(), SOCK_STREAM, 0);
		if (!isValid()) {
			return false;
		}
		if (::connect(sd_, addr.get(), addr.getSize())) {
			int keep = errno;
			close();
			errno = keep;
			return false;
		}
		return true;
	}

	/**
		init for server
		@param port [in] port number
	*/
	bool bind(uint16_t port, bool onlyIpv4 = false)
	{
		const int family = onlyIpv4 ? AF_INET : AF_INET6;
		sd_ = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
		if (!isValid()) {
			return false;
		}
		if (!setSocketOption(SO_REUSEADDR, 1)) {
			return false;
		}
		struct sockaddr_in6 addr6;
		struct sockaddr_in addr4;
		struct sockaddr *addr;
		socklen_t addrLen;
		if (onlyIpv4) {
			memset(&addr4, 0, sizeof(addr4));
			addr4.sin_family = AF_INET;
			addr4.sin_port = htons(port);
			addr = (struct sockaddr*)&addr4;
			addrLen = sizeof(addr4);
		} else {
			memset(&addr6, 0, sizeof(addr6));
			addr6.sin6_family = AF_INET6;
			addr6.sin6_port = htons(port);
			addr = (struct sockaddr*)&addr6;
			addrLen = sizeof(addr6);
		}
		if (::bind(sd_, addr, addrLen) == 0) {
			if (::listen(sd_, SOMAXCONN) == 0) {
				return true;
			}
		}
		cybozu::socket_local::close(sd_);
		sd_ = INVALID_SOCKET;
		return false;
	}

	/**
		return true if acceptable, otherwise false
		return false if one second passed
		while (!server.queryAccept()) {
		}
		client.accept(server);
	*/
	bool queryAccept()
	{
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET((unsigned)sd_, &fds);
		int ret = ::select((int)sd_ + 1, &fds, 0, 0, &timeout);
		if (ret > 0) {// && ret != SOCKET_ERROR) {
			return FD_ISSET(sd_, &fds) != 0;
		}
		return false;
	}

	/**
		accept for server
	*/
	bool accept(Socket& client, struct sockaddr_storage *paddr = 0) const
	{
		struct sockaddr_storage addr;
		socklen_t len = sizeof(addr);
		if (paddr == 0) paddr = &addr;
		client.sd_ = ::accept(sd_, (struct sockaddr *)paddr, &len);
		return client.isValid();
	}

	/**
		setsockopt() for int
	*/
	bool setSocketOption(int level, int optname, int value)
	{
		return setsockopt(sd_, level, optname, cybozu::cast<const char*>(&value), sizeof(value)) == 0;
	}
	bool setSocketOption(int optname, int value)
	{
		return setSocketOption(SOL_SOCKET, optname, value);
	}
	/**
		getsockopt() for int
	*/
	bool getSocketOption(int level, int optname, int* value) const
	{
		socklen_t len = sizeof(*value);
		return getsockopt(sd_, level, optname, cybozu::cast<char*>(value), &len) == 0;
	}
	bool getSocketOption(int optname, int* value) const
	{
		return getSocketOption(SOL_SOCKET, optname, value);
	}
	template<typename T>
	bool setSocketOption(int optname, const T& value)
	{
		return setsockopt(sd_, SOL_SOCKET, optname, cybozu::cast<const char*>(&value), sizeof(T)) == 0;
	}
	template<typename T>
	bool getSocketOption(int optname, T* value) const
	{
		socklen_t len = sizeof(T);
		return getsockopt(sd_, SOL_SOCKET, optname, cybozu::cast<char*>(value), &len) == 0;
	}
	/**
		setup linger
	*/
	bool setLinger(uint16_t l_onoff, uint16_t l_linger)
	{
		struct linger linger;
		linger.l_onoff = l_onoff;
		linger.l_linger = l_linger;
		return setSocketOption(SO_LINGER, &linger);
	}
	/**
		get receive buffer size
		@retval positive buffer size(byte)
		@retval -1 error
	*/
	int getReceiveBufferSize() const
	{
		int val;
		if (getSocketOption(SO_RCVBUF, &val)) {
			return val;
		} else {
			return -1;
		}
	}
	/**
		set receive buffer size
		@param size [in] buffer size(byte)
	*/
	bool setReceiveBufferSize(int size)
	{
		return setSocketOption(SO_RCVBUF, size);
	}
	/**
		get send buffer size
		@retval positive buffer size(byte)
		@retval -1 error
	*/
	int getSendBufferSize() const
	{
		int val;
		if (getSocketOption(SO_SNDBUF, &val)) {
			return val;
		} else {
			return -1;
		}
	}
	/**
		sed send buffer size
		@param size [in] buffer size(byte)
	*/
	bool setSendBufferSize(int size)
	{
		return setSocketOption(SO_SNDBUF, size);
	}
	/**
		set send timeout
		@param msec [in] msec
	*/
	bool setSendTimeout(int msec)
	{
		return setTimeout(SO_SNDTIMEO, msec);
	}
	/**
		set receive timeout
		@param msec [in] msec
	*/
	bool setReceiveTimeout(int msec)
	{
		return setTimeout(SO_RCVTIMEO, msec);
	}
	/**
		get send timeout
	*/
	int getSendTimeout() const
	{
		return getTimeout(SO_SNDTIMEO);
	}
	/**
		get receive timeout
	*/
	int getReceiveTimeout() const
	{
		return getTimeout(SO_RCVTIMEO);
	}
};

} // cybozu

#ifdef _WIN32
	#pragma warning(pop)
#endif
