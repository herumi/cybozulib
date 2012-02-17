#include <cybozu/test.hpp>
#include <cybozu/zlib.hpp>
#include <cybozu/file.hpp>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

struct OutputStream {
	std::ostringstream os_;
	size_t write(const char *str, size_t size)
	{
		os_.write(str, size);
		return os_ ? size : 0;
	}
};

struct InputStream {
	std::stringstream is_;
	size_t read(char *str, size_t size)
	{
		is_.read(str, size);
		return is_.gcount();
	}
};

CYBOZU_TEST_AUTO(test1_deflate)
{
	typedef cybozu::ZlibCompressorT<OutputStream> Compressor;
	typedef cybozu::ZlibDecompressorT<InputStream> Decompressor;
	const std::string in = "this is a pen";
	OutputStream os;

	{
		Compressor comp(os);
		comp.exec(in.c_str(), in.size());
		comp.flush();
	}
	const std::string enc = os.os_.str();

	const char encTbl[] = "\x78\x9c\x2b\xc9\xc8\x2c\x56\x00\xa2\x44\x85\x82\xd4\x3c\x00\x21\x0c\x04\x99";

	const std::string encOk(encTbl, encTbl + sizeof(encTbl) - 1);
	CYBOZU_TEST_EQUAL(enc, encOk);

	InputStream is;
	is.is_ << encOk;

	{
		Decompressor dec(is);
		char buf[2048];
		std::string out;
		for (;;) {
			size_t size = dec.read(buf, sizeof(buf));
			if (size == 0) break;
			out.append(buf, buf + size);
		}
		CYBOZU_TEST_EQUAL(in, out);
	}
}

CYBOZU_TEST_AUTO(test2_deflate)
{
	typedef cybozu::ZlibCompressorT<OutputStream, 1> Compressor;
	typedef cybozu::ZlibDecompressorT<InputStream, 1> Decompressor;
	const std::string in = "this is a pen";
	OutputStream os;

	{
		Compressor comp(os);
		for (size_t i = 0; i < in.size(); i++) {
			comp.exec(&in[i], 1);
		}
		comp.flush();
	}
	const std::string enc = os.os_.str();

	const char encTbl[] = "\x78\x9c\x2b\xc9\xc8\x2c\x56\x00\xa2\x44\x85\x82\xd4\x3c\x00\x21\x0c\x04\x99";

	const std::string encOk(encTbl, encTbl + sizeof(encTbl) - 1);
	CYBOZU_TEST_EQUAL(enc, encOk);

	InputStream is;
	is.is_ << encOk;

	{
		Decompressor dec(is);
		char buf[2048];
		std::string out;
		for (;;) {
			size_t size = dec.read(buf, sizeof(buf));
			if (size == 0) break;
			out.append(buf, buf + size);
		}
		CYBOZU_TEST_EQUAL(in, out);
	}
}

namespace test3 {

struct OutputStream {
	std::ostringstream os_;
	size_t write(const char *buf, size_t size)
	{
		os_.write(buf, size);
		return size;
	}
};

struct InputMemoryStream {
	const char *p_;
	size_t remainSize_;
	InputMemoryStream(const char *buf, size_t size)
		: p_(buf)
		, remainSize_(size)
	{
	}
	size_t read(char *buf, size_t size)
	{
		size_t readSize = std::min(size, remainSize_);
		memcpy(buf, p_, readSize);
		p_ += readSize;
		remainSize_ -= readSize;
		return readSize;
	}
};

}

CYBOZU_TEST_AUTO(enc_and_dec)
{
	std::string body = "From: cybozu\r\n"
				 "To: cybozu\r\n"
				 "\r\n"
				 "hello with zlib compressed\r\n"
				 ".\r\n";
	test3::OutputStream os;
	cybozu::ZlibCompressorT<test3::OutputStream> comp(os);
	comp.exec(&body[0], body.size());
	comp.flush();


	std::string enc = os.os_.str();

	test3::InputMemoryStream ims(&enc[0], enc.size());

	cybozu::ZlibDecompressorT<test3::InputMemoryStream> dec(ims);
	char buf[4096];
	size_t size = dec.read(buf, sizeof(buf));
	std::string decStr(buf, buf + size);
	CYBOZU_TEST_EQUAL(body, decStr);
}

CYBOZU_TEST_AUTO(output_gzip1)
{
	std::string str = "Only a test, test, test, test, test, test, test, test!\n";
	test3::OutputStream os;
	cybozu::ZlibCompressorT<test3::OutputStream> comp(os, true);
	comp.exec(&str[0], str.size());
	comp.flush();
	std::string enc = os.os_.str();
//	std::ofstream ofs("c:/tmp/aaa.gz", std::ios::binary);
//	ofs << enc;

	{
		InputStream is;
		is.is_ << enc;
		cybozu::ZlibDecompressorT<InputStream> dec(is, true);
		char buf[4096];
		size_t size = dec.read(buf, sizeof(buf));
		std::string decStr(buf, buf + size);
		CYBOZU_TEST_EQUAL(decStr, str);
	}

	{
		const char s[] = "\x1F\x8B\x08\x00\x00\x00\x00\x00\x00\x0B\xF2\xCF\xCB\xA9\x54\x48\x54\x28\x49\x2D\x2E\xD1\x21\x9A\x54\xE4\x02\x00\x00\x00\xFF\xFF\x03\x00\x6A\xBD\xF4\xC9\x37\x00\x00";
		InputStream is;
		is.is_ << std::string(s, s + sizeof(s));
		cybozu::ZlibDecompressorT<InputStream> dec(is, true);
		char buf[4096];
		size_t size = dec.read(buf, sizeof(buf));
		std::string decStr(buf, buf + size);
		CYBOZU_TEST_EQUAL(decStr, str);
	}
}

#ifdef _MSC_VER
	#pragma warning(disable : 4309)
#endif

CYBOZU_TEST_AUTO(output_gzip2)
{
	const char textBuf[] = {
		0x23, 0x69, 0x6E, 0x63, 0x6C, 0x75, 0x64, 0x65, 0x20, 0x3C, 0x73, 0x74, 0x64, 0x69, 0x6F, 0x2E,
		0x68, 0x3E, 0x0A, 0x0A, 0x69, 0x6E, 0x74, 0x20, 0x6D, 0x61, 0x69, 0x6E, 0x28, 0x29, 0x0A, 0x7B,
		0x0A, 0x09, 0x70, 0x75, 0x74, 0x73, 0x28, 0x22, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x22, 0x29, 0x3B,
		0x0A, 0x09, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6E, 0x20, 0x30, 0x3B, 0x0A, 0x7D, 0x0A, 0x0A,
	};

	const char encBuf[] = {
		0x1F, 0x8B, 0x08, 0x08, 0xA4, 0x7E, 0x20, 0x4B, 0x00, 0x03, 0x74, 0x2E, 0x63, 0x70, 0x70, 0x00,
		0x53, 0xCE, 0xCC, 0x4B, 0xCE, 0x29, 0x4D, 0x49, 0x55, 0xB0, 0x29, 0x2E, 0x49, 0xC9, 0xCC, 0xD7,
		0xCB, 0xB0, 0xE3, 0xE2, 0xCA, 0xCC, 0x2B, 0x51, 0xC8, 0x4D, 0xCC, 0xCC, 0xD3, 0xD0, 0xE4, 0xAA,
		0xE6, 0xE2, 0x2C, 0x28, 0x2D, 0x29, 0xD6, 0x50, 0xCA, 0x48, 0xCD, 0xC9, 0xC9, 0x57, 0xD2, 0xB4,
		0xE6, 0xE2, 0x2C, 0x4A, 0x2D, 0x29, 0x2D, 0xCA, 0x53, 0x30, 0xB0, 0xE6, 0xAA, 0xE5, 0xE2, 0x02,
		0x00, 0x48, 0xAB, 0x48, 0x61, 0x3F, 0x00, 0x00, 0x00,
	};
	const std::string text(textBuf, textBuf + sizeof(textBuf));

	{
		InputStream is;
		is.is_.write(encBuf, sizeof(encBuf));
		cybozu::ZlibDecompressorT<InputStream> dec(is, true);
		char buf[4096];
		size_t size = dec.read(buf, sizeof(buf));
		std::string decStr(buf, buf + size);
		CYBOZU_TEST_EQUAL(decStr, text);
	}
}
