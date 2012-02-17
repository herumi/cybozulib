#include <cybozu/test.hpp>
#include <cybozu/stream.hpp>
#include <queue>

const size_t maxLineSize = 20;

struct Socket : public std::queue<std::string> {
	size_t read(char *buf, size_t size)
	{
		if (empty()) {
			return 0;
		} else {
			size_t dataSize = std::min(front().size(), size);
			memcpy(buf, &front()[0], dataSize);
			pop();
			return dataSize;
		}
	}
};

typedef cybozu::LineStreamT<Socket, maxLineSize> LineStream;

CYBOZU_TEST_AUTO(test0)
{
	Socket socket;
	LineStream lineStream(socket);
	const std::string *line = lineStream.next();
	CYBOZU_TEST_EQUAL_POINTER(line, 0);
}

CYBOZU_TEST_AUTO(test1)
{
	Socket socket;
	socket.push("0123456789012345678\n");
	socket.push("abcdefghijklmnopqrst\n");
	socket.push("ABCEF5678901234567890\n");
	LineStream lineStream(socket);
	const std::string *line;
	line = lineStream.next();
	CYBOZU_TEST_EQUAL(*line, "0123456789012345678");

	line = lineStream.next();
	CYBOZU_TEST_EQUAL(*line, "abcdefghijklmnopqrst");

	CYBOZU_TEST_EXCEPTION_MESSAGE(lineStream.next(), cybozu::StreamException, "line is too long");
}

CYBOZU_TEST_AUTO(test2)
{
	Socket socket;
	socket.push("01234567890\n0123456789");
	socket.push("01234");
	socket.push("567");
	socket.push("8");
	socket.push("9");
	socket.push("\n");

	socket.push("01234567890\n0123456789");
	socket.push("01234");
	socket.push("567890");

	LineStream lineStream(socket);
	const std::string *line;
	line = lineStream.next();
	CYBOZU_TEST_EQUAL(*line, "01234567890");

	line = lineStream.next();
	CYBOZU_TEST_EQUAL(*line, "01234567890123456789");

	line = lineStream.next();
	CYBOZU_TEST_EQUAL(*line, "01234567890");

	CYBOZU_TEST_EXCEPTION_MESSAGE(lineStream.next(), cybozu::StreamException, "no CRLF");
}

CYBOZU_TEST_AUTO(test3)
{
	const char tbl[][64] = {
		"abc\r\ndef\nghi",
		"jk\r\n123\n\r\nxyz",
		"A012\n3456\n789\n",
	};
	const char out[][64] = {
		"abc",
		"def",
		"ghijk",
		"123",
		"",
		"xyzA012",
		"3456",
		"789",
	};
	Socket socket;
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		socket.push(tbl[i]);
	}
	LineStream lineStream(socket);
	size_t outPos = 0;
	for (;;) {
		const std::string *line = lineStream.next();
		if (line == 0) break;
		CYBOZU_TEST_EQUAL(*line, out[outPos]);
		outPos++;
	}
	CYBOZU_TEST_EQUAL(outPos, CYBOZU_NUM_OF_ARRAY(out));
}

CYBOZU_TEST_AUTO(test4)
{
	const char tbl[][64] = {
		"\r\n\r\n",
		"\n\n",
		"\r\r\n",
	};
	const char out[][64] = {
		"",
		"",
		"",
		"",
		"\r",
	};
	Socket socket;
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		socket.push(tbl[i]);
	}
	LineStream lineStream(socket);
	size_t outPos = 0;
	for (;;) {
		const std::string *line = lineStream.next();
		if (line == 0) break;
		CYBOZU_TEST_EQUAL(*line, out[outPos]);
		outPos++;
	}
	CYBOZU_TEST_EQUAL(outPos, CYBOZU_NUM_OF_ARRAY(out));
}

CYBOZU_TEST_AUTO(lastDataWithoutCRLF)
{
	const char tbl[][64] = {
		"abc\r\ndef\nghi",
		"jk\r\n123\n\r\nxyz",
		"A012\n3456\n789",
	};
	const char out[][64] = {
		"abc",
		"def",
		"ghijk",
		"123",
		"",
		"xyzA012",
		"3456",
		"789",
	};
	Socket socket;
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		socket.push(tbl[i]);
	}
	LineStream lineStream(socket);
	size_t outPos = 0;
	for (;;) {
		const std::string *line = lineStream.next();
		if (line == 0) break;
		CYBOZU_TEST_EQUAL(*line, out[outPos]);
		outPos++;
	}
	CYBOZU_TEST_EQUAL(outPos, CYBOZU_NUM_OF_ARRAY(out));
}
