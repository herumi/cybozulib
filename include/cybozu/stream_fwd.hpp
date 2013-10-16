#pragma once
/**
	@file
	@brief definition of InputStreamTag/OutputStreamTag

	Copyright (C) Cybozu Labs, Inc., all rights reserved.
*/
#include <cybozu/exception.hpp>

namespace cybozu {

template<class InputStream>
struct InputStreamTag;

template<class OutputStream>
struct OutputStreamTag;

namespace stream {

template<typename InputStream>
void read(InputStream& is, void *buf, size_t size)
{
	char *p = static_cast<char*>(buf);
	while (size > 0) {
		size_t readSize = cybozu::InputStreamTag<InputStream>::readSome(is, p, size);
		if (readSize == 0) throw cybozu::Exception("stream:InputStreamRead");
		p += readSize;
		size -= readSize;
	}
}

} } // cybozu::stream

