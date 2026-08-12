#include <cybozu/test.hpp>
#include <cybozu/aes.hpp>
#include <cybozu/itoa.hpp>
#include <cybozu/atoi.hpp>

std::string toHexStr(const std::string& buf)
{
	std::string s;
	s.resize(buf.size() * 2);
	const uint8_t *p = (const uint8_t*)buf.c_str();
	for (size_t i = 0; i < buf.size(); i++) {
		cybozu::itohex(&s[i * 2], 2, p[i], false);
	}
	return s;
}

std::string fromHexStr(const std::string& hex)
{
	if ((hex.size() & 1) != 0) {
		throw cybozu::Exception("fromHexStr:bad size") << hex.size();
	}
	std::string out(hex.size() / 2, 0);
	for (size_t i = 0; i < out.size(); i++) {
		out[i] = static_cast<uint8_t>(cybozu::hextoi(&hex[i * 2], 2));
	}
	return out;
}

std::string evalCipher(cybozu::crypto::Cipher::Name name, cybozu::crypto::Cipher::Mode mode, const std::string& key, const std::string& iv, const std::string& input, bool padding = false)
{
	cybozu::crypto::Cipher cipher(name);
	cipher.setup(mode, key, iv, padding);
	std::string out(input.size() + 32, 0);
	int pos = cipher.update(&out[0], input.data(), static_cast<int>(input.size()));
	CYBOZU_TEST_ASSERT(pos >= 0);
	int last = cipher.finalize(&out[pos]);
	CYBOZU_TEST_ASSERT(last >= 0);
	out.resize(pos + last);
	return out;
}

struct AesCbcTestVec {
	cybozu::crypto::Cipher::Name name;
	const char *keyHex;
	const char *ivHex;
	const char *plain;
	const char *cipherHex;
	bool isAscii;
};

const AesCbcTestVec aesCbcTbl[] = {
	{
		cybozu::crypto::Cipher::N_AES128_CBC,
		"06a9214036b8a15b512e03d534120006",
		"3dafba429d9eb430b422da802c9fac41",
		"Single block msg",
		"e353779c1079aeb82708942dbe77181a",
		true,
	},
	{
		cybozu::crypto::Cipher::N_AES128_CBC,
		"c286696d887c9aa0611bbb3e2025a45a",
		"562e17996d093d28ddb3ba695a2e6f58",
		"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
		"d296cd94c2cccf8a3a863028b5e1dc0a7586602d253cfff91b8266bea6d61ab1",
		false,
	},
	{
		cybozu::crypto::Cipher::N_AES128_CBC,
		"6c3ea0477630ce21a2ce334aa746c2cd",
		"c782dc4c098c66cbd9cd27d825682c81",
		"This is a 48-byte message (exactly 3 AES blocks)",
		"d0a02b3836451753d493665d33f0e8862dea54cdb293abc7506939276772f8d5021c19216bad525c8579695d83ba2684",
		true,
	},
	{
		cybozu::crypto::Cipher::N_AES128_CBC,
		"56e47a38c5598974bc46903dba290349",
		"8ce82eefbea0da3c44699ed7db51b7d9",
		"a0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedf",
		"c30e32ffedc0774e6aff6af0869f71aa0f3af07a9a31a9c684db207eb0ef8e4e35907aa632c3ffdf868bb7b29d3d46ad83ce9f9a102ee99d49a53e87f4c3da55",
		false,
	},
	// NIST SP 800-38A F.2.3 CBC-AES192
	{
		cybozu::crypto::Cipher::N_AES192_CBC,
		"8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b",
		"000102030405060708090a0b0c0d0e0f",
		"6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
		"4f021db243bc633d7178183a9fa071e8b4d9ada9ad7dedf4e5e738763f69145a571b242012fb7ae07fa9baac3df102e008b0e27988598881d920a9e64f5615cd",
		false,
	},
	// NIST SP 800-38A F.2.5 CBC-AES256
	{
		cybozu::crypto::Cipher::N_AES256_CBC,
		"603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
		"000102030405060708090a0b0c0d0e0f",
		"6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
		"f58c4c04d6e5f1ba779eabfb5f7bfbd69cfc4e967edb808d679f777bc6702c7d39f23369a9d9bacfa530e26304231461b2eb05e2c39be9fcda6c19078c6a9d1b",
		false,
	},
};

struct AesEcbTestVec {
	cybozu::crypto::Cipher::Name name;
	const char *keyHex;
	const char *plainHex;
	const char *cipherHex;
};

// NIST SP 800-38A F.1.1/F.1.3/F.1.5 ECB
const AesEcbTestVec aesEcbTbl[] = {
	{
		cybozu::crypto::Cipher::N_AES128_ECB,
		"2b7e151628aed2a6abf7158809cf4f3c",
		"6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
		"3ad77bb40d7a3660a89ecaf32466ef97f5d3d58503b9699de785895a96fdbaaf43b1cd7f598ece23881b00e3ed0306887b0c785e27e8ad3f8223207104725dd4",
	},
	{
		cybozu::crypto::Cipher::N_AES192_ECB,
		"8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b",
		"6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
		"bd334f1d6e45f25ff712a214571fa5cc974104846d0ad3ad7734ecb3ecee4eefef7afd2270e2e60adce0ba2face6444e9a4b41ba738d6c72fb16691603c18e0e",
	},
	{
		cybozu::crypto::Cipher::N_AES256_ECB,
		"603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
		"6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
		"f3eed1bdb5d2a03c064b5a7e3db181f8591ccb10d410ed26dc5ba74a31362870b6ed21b99ca6f4f9f153e7b1beafed1d23304b7a39f9f3ff067d8d8f9e24ecc7",
	},
};

struct AesCtrTestVec {
	cybozu::crypto::Cipher::Name name;
	const char *keyHex;
	const char *counterHex;
	const char *plain;
	const char *cipherHex;
	bool isAscii;
};

const AesCtrTestVec aesCtrTbl[] = {
	{
		cybozu::crypto::Cipher::N_AES128_CTR,
		"ae6852f8121067cc4bf7a5765577f39e",
		"00000030000000000000000000000001",
		"Single block msg",
		"e4095d4fb7a7b3792d6175a3261311b8",
		true,
	},
	{
		cybozu::crypto::Cipher::N_AES128_CTR,
		"7e24067817fae0d743d6ce1f32539163",
		"006cb6dbc0543b59da48d90b00000001",
		"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
		"5104a106168a72d9790d41ee8edad388eb2e1efc46da57c8fce630df9141be28",
		false,
	},
	{
		cybozu::crypto::Cipher::N_AES128_CTR,
		"7691be035e5020a8ac6e618529f9a0dc",
		"00e0017b27777f3f4a1786f000000001",
		"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20212223",
		"c1cf48a89f2ffdd9cf4652e9efdb72d74540a42bde6d7836d59a5ceaaef3105325b2072f",
		false,
	},
	{
		cybozu::crypto::Cipher::N_AES192_CTR,
		"16af5b145fc9f579c175f93e3bfb0eed863d06ccfdb78515",
		"0000004836733c147d6d93cb00000001",
		"Single block msg",
		"4b55384fe259c9c84e7935a003cbe928",
		true,
	},
	{
		cybozu::crypto::Cipher::N_AES192_CTR,
		"7c5cb2401b3dc33c19e7340819e0f69c678c3db8e6f6a91a",
		"0096b03b020c6eadc2cb500d00000001",
		"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
		"453243fc609b23327edfaafa7131cd9f8490701c5ad4a79cfc1fe0ff42f4fb00",
		false,
	},
	{
		cybozu::crypto::Cipher::N_AES192_CTR,
		"02bf391ee8ecb159b959617b0965279bf59b60a786d3e0fe",
		"0007bdfd5cbd60278dcc091200000001",
		"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20212223",
		"96893fc55e5c722f540b7dd1ddf7e758d288bc95c69165884536c811662f2188abee0935",
		false,
	},
	{
		cybozu::crypto::Cipher::N_AES256_CTR,
		"776beff2851db06f4c8a0542c8696f6c6a81af1eec96b4d37fc1d689e6c1c104",
		"00000060db5672c97aa8f0b200000001",
		"Single block msg",
		"145ad01dbf824ec7560863dc71e3e0c0",
		true,
	},
	{
		cybozu::crypto::Cipher::N_AES256_CTR,
		"f6d66d6bd52d59bb0796365879eff886c66dd51a5b6a99744b50590c87a23884",
		"00faac24c1585ef15a43d87500000001",
		"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
		"f05e231b3894612c49ee000b804eb2a9b8306b508f839d6a5530831d9344af1c",
		false,
	},
	{
		cybozu::crypto::Cipher::N_AES256_CTR,
		"ff7a617ce69148e4f1726e2f43581de2aa62d9f805532edff1eed687fb54153d",
		"001cc5b751a51d70a1c1114800000001",
		"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20212223",
		"eb6c52821d0bbbf7ce7594462aca4faab407df866569fd07f48cc0b583d6071f1ec0e6b8",
		false,
	},
};

CYBOZU_TEST_AUTO(aesCbcRfc3602)
{
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(aesCbcTbl); i++) {
		const std::string key = fromHexStr(aesCbcTbl[i].keyHex);
		const std::string iv = fromHexStr(aesCbcTbl[i].ivHex);
		const std::string plain = aesCbcTbl[i].isAscii ? std::string(aesCbcTbl[i].plain) : fromHexStr(aesCbcTbl[i].plain);
		const std::string cipher = fromHexStr(aesCbcTbl[i].cipherHex);
		CYBOZU_TEST_EQUAL(evalCipher(aesCbcTbl[i].name, cybozu::crypto::Cipher::Encoding, key, iv, plain), cipher);
		CYBOZU_TEST_EQUAL(evalCipher(aesCbcTbl[i].name, cybozu::crypto::Cipher::Decoding, key, iv, cipher), plain);
	}
}

CYBOZU_TEST_AUTO(aesCtrRfc3686)
{
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(aesCtrTbl); i++) {
		const std::string key = fromHexStr(aesCtrTbl[i].keyHex);
		const std::string iv = fromHexStr(aesCtrTbl[i].counterHex);
		const std::string plain = aesCtrTbl[i].isAscii ? std::string(aesCtrTbl[i].plain) : fromHexStr(aesCtrTbl[i].plain);
		const std::string cipher = fromHexStr(aesCtrTbl[i].cipherHex);
		CYBOZU_TEST_EQUAL(evalCipher(aesCtrTbl[i].name, cybozu::crypto::Cipher::Encoding, key, iv, plain), cipher);
		CYBOZU_TEST_EQUAL(evalCipher(aesCtrTbl[i].name, cybozu::crypto::Cipher::Decoding, key, iv, cipher), plain);
	}
}

CYBOZU_TEST_AUTO(aesEcbNist)
{
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(aesEcbTbl); i++) {
		const std::string key = fromHexStr(aesEcbTbl[i].keyHex);
		const std::string plain = fromHexStr(aesEcbTbl[i].plainHex);
		const std::string cipher = fromHexStr(aesEcbTbl[i].cipherHex);
		CYBOZU_TEST_EQUAL(evalCipher(aesEcbTbl[i].name, cybozu::crypto::Cipher::Encoding, key, "", plain), cipher);
		CYBOZU_TEST_EQUAL(evalCipher(aesEcbTbl[i].name, cybozu::crypto::Cipher::Decoding, key, "", cipher), plain);
	}
}

// call update with small chunks and verify the result is the same as the one-shot version
std::string evalCipherChunked(cybozu::crypto::Cipher::Name name, cybozu::crypto::Cipher::Mode mode, const std::string& key, const std::string& iv, const std::string& input, size_t chunkSize)
{
	cybozu::crypto::Cipher cipher(name);
	cipher.setup(mode, key, iv);
	std::string out(input.size() + 32, 0);
	int pos = 0;
	size_t offset = 0;
	while (offset < input.size()) {
		size_t n = input.size() - offset;
		if (n > chunkSize) n = chunkSize;
		int ret = cipher.update(&out[pos], input.data() + offset, static_cast<int>(n));
		CYBOZU_TEST_ASSERT(ret >= 0);
		pos += ret;
		offset += n;
	}
	int last = cipher.finalize(&out[pos]);
	CYBOZU_TEST_ASSERT(last >= 0);
	out.resize(pos + last);
	return out;
}

CYBOZU_TEST_AUTO(aesPartialUpdate)
{
	const size_t chunkTbl[] = { 1, 3, 7, 16, 21 };
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(aesCbcTbl); i++) {
		const std::string key = fromHexStr(aesCbcTbl[i].keyHex);
		const std::string iv = fromHexStr(aesCbcTbl[i].ivHex);
		const std::string plain = aesCbcTbl[i].isAscii ? std::string(aesCbcTbl[i].plain) : fromHexStr(aesCbcTbl[i].plain);
		const std::string cipher = fromHexStr(aesCbcTbl[i].cipherHex);
		for (size_t j = 0; j < CYBOZU_NUM_OF_ARRAY(chunkTbl); j++) {
			CYBOZU_TEST_EQUAL(evalCipherChunked(aesCbcTbl[i].name, cybozu::crypto::Cipher::Encoding, key, iv, plain, chunkTbl[j]), cipher);
			CYBOZU_TEST_EQUAL(evalCipherChunked(aesCbcTbl[i].name, cybozu::crypto::Cipher::Decoding, key, iv, cipher, chunkTbl[j]), plain);
		}
	}
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(aesCtrTbl); i++) {
		const std::string key = fromHexStr(aesCtrTbl[i].keyHex);
		const std::string iv = fromHexStr(aesCtrTbl[i].counterHex);
		const std::string plain = aesCtrTbl[i].isAscii ? std::string(aesCtrTbl[i].plain) : fromHexStr(aesCtrTbl[i].plain);
		const std::string cipher = fromHexStr(aesCtrTbl[i].cipherHex);
		for (size_t j = 0; j < CYBOZU_NUM_OF_ARRAY(chunkTbl); j++) {
			CYBOZU_TEST_EQUAL(evalCipherChunked(aesCtrTbl[i].name, cybozu::crypto::Cipher::Encoding, key, iv, plain, chunkTbl[j]), cipher);
			CYBOZU_TEST_EQUAL(evalCipherChunked(aesCtrTbl[i].name, cybozu::crypto::Cipher::Decoding, key, iv, cipher, chunkTbl[j]), plain);
		}
	}
}
