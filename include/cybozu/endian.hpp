#pragma once

/**
	@file
	@brief deal with big and little endian

	Copyright (C) 2008 Cybozu Labs, Inc., all rights reserved.
*/
#include <cybozu/inttype.hpp>

namespace cybozu {

/**
	get 16bit integer as little endian
	@param src [in] pointer
*/
inline uint16_t Get16bitAsLE(const void *src)
{
	const uint8_t *p = reinterpret_cast<const uint8_t *>(src);
	return p[0] | (p[1] << 8);
}

/**
	get 32bit integer as little endian
	@param src [in] pointer
*/
inline uint32_t Get32bitAsLE(const void *src)
{
	const uint8_t *p = reinterpret_cast<const uint8_t *>(src);
	return Get16bitAsLE(p) | ((uint32_t)Get16bitAsLE(p + 2) << 16);
}

/**
	get 64bit integer as little endian
	@param src [in] pointer
*/
inline uint64_t Get64bitAsLE(const void *src)
{
	const uint8_t *p = reinterpret_cast<const uint8_t *>(src);
	return Get32bitAsLE(p) | (static_cast<uint64_t>(Get32bitAsLE(p + 4)) << 32);
}

/**
	get 16bit integer as bit endian
	@param src [in] pointer
*/
inline uint16_t Get16bitAsBE(const void *src)
{
	const uint8_t *p = reinterpret_cast<const uint8_t *>(src);
	return p[1] | (p[0] << 8);
}

/**
	get 32bit integer as bit endian
	@param src [in] pointer
*/
inline uint32_t Get32bitAsBE(const void *src)
{
	const uint8_t *p = reinterpret_cast<const uint8_t *>(src);
	return Get16bitAsBE(p + 2) | ((uint32_t)Get16bitAsBE(p) << 16);
}

/**
	get 64bit integer as big endian
	@param src [in] pointer
*/
inline uint64_t Get64bitAsBE(const void *src)
{
	const uint8_t *p = reinterpret_cast<const uint8_t *>(src);
	return Get32bitAsBE(p + 4) | (static_cast<uint64_t>(Get32bitAsBE(p)) << 32);
}

/**
	put 16bit integer as little endian
	@param src [out] pointer
	@param x [in] integer
*/
inline void Put16bitAsLE(void *src, uint16_t x)
{
	uint8_t *p = reinterpret_cast<uint8_t *>(src);
	p[0] = (uint8_t)x;
	p[1] = (uint8_t)(x >> 8);
}
/**
	put 32bit integer as little endian
	@param src [out] pointer
	@param x [in] integer
*/
inline void Put32bitAsLE(void *src, uint32_t x)
{
	uint8_t *p = reinterpret_cast<uint8_t *>(src);
	p[0] = (uint8_t)x;
	p[1] = (uint8_t)(x >> 8);
	p[2] = (uint8_t)(x >> 16);
	p[3] = (uint8_t)(x >> 24);
}
/**
	put 64bit integer as little endian
	@param src [out] pointer
	@param x [in] integer
*/
inline void Put64bitAsLE(void *src, uint64_t x)
{
	uint8_t *p = reinterpret_cast<uint8_t *>(src);
	Put32bitAsLE(p, (uint32_t)x);
	Put32bitAsLE(p + 4, (uint32_t)(x >> 32));
}
/**
	put 16bit integer as big endian
	@param src [out] pointer
	@param x [in] integer
*/
inline void Put16bitAsBE(void *src, uint16_t x)
{
	uint8_t *p = reinterpret_cast<uint8_t *>(src);
	p[0] = (uint8_t)(x >> 8);
	p[1] = (uint8_t)x;
}
/**
	put 32bit integer as big endian
	@param src [out] pointer
	@param x [in] integer
*/
inline void Put32bitAsBE(void *src, uint32_t x)
{
	uint8_t *p = reinterpret_cast<uint8_t *>(src);
	p[0] = (uint8_t)(x >> 24);
	p[1] = (uint8_t)(x >> 16);
	p[2] = (uint8_t)(x >> 8);
	p[3] = (uint8_t)x;
}
/**
	put 64bit integer as big endian
	@param src [out] pointer
	@param x [in] integer
*/
inline void Put64bitAsBE(void *src, uint64_t x)
{
	uint8_t *p = reinterpret_cast<uint8_t *>(src);
	Put32bitAsBE(p, (uint32_t)(x >> 32));
	Put32bitAsBE(p + 4, (uint32_t)x);
}

} // cybozu
