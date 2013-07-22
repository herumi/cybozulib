#include <cybozu/file.hpp>
#include <cybozu/test.hpp>
#include <memory.h>
#include <stdlib.h>
#include <vector>

static const std::string fileName = "test.tmp";
static const std::string fileName2 = "test2.tmp";
static const char testBuf[] = { 1, 2, 5, 4, 3, 2, 1 };

CYBOZU_TEST_AUTO(open_and_write)
{
	cybozu::File f;
	CYBOZU_TEST_ASSERT(!f.isOpen());
	CYBOZU_TEST_ASSERT(f.open(fileName, std::ios::out | std::ios::trunc, cybozu::DontThrow));
	CYBOZU_TEST_ASSERT(f.isOpen());
	CYBOZU_TEST_ASSERT(f.write(testBuf, sizeof(testBuf), cybozu::DontThrow));
}

CYBOZU_TEST_AUTO(read)
{
	cybozu::File f;
	CYBOZU_TEST_ASSERT(f.open(fileName, std::ios::in, cybozu::DontThrow));
	char buf[sizeof(testBuf)];
	CYBOZU_TEST_EQUAL(f.read(buf, sizeof(buf), cybozu::DontThrow), (int)sizeof(testBuf));
	CYBOZU_TEST_ASSERT(memcmp(buf, testBuf, sizeof(buf)) == 0);
	CYBOZU_TEST_EQUAL(f.getSize(), (int)sizeof(buf));
}

CYBOZU_TEST_AUTO(size)
{
	CYBOZU_TEST_EQUAL(cybozu::GetFileSize(fileName), sizeof(testBuf));
}

CYBOZU_TEST_AUTO(append)
{
	cybozu::File f;
	CYBOZU_TEST_ASSERT(!f.isOpen());
	CYBOZU_TEST_ASSERT(f.open(fileName, std::ios::out | std::ios::app, cybozu::DontThrow));
	CYBOZU_TEST_ASSERT(f.isOpen());
	CYBOZU_TEST_ASSERT(f.write(testBuf, sizeof(testBuf), cybozu::DontThrow));
}

CYBOZU_TEST_AUTO(read2)
{
	cybozu::File f;
	CYBOZU_TEST_ASSERT(f.open(fileName, std::ios::in, cybozu::DontThrow));
	const int size = sizeof(testBuf);
	char buf[size * 2];
	CYBOZU_TEST_EQUAL(f.read(buf, size * 2, cybozu::DontThrow), size * 2);
	CYBOZU_TEST_ASSERT(memcmp(buf, testBuf, size) == 0);
	CYBOZU_TEST_ASSERT(memcmp(buf + size, testBuf, size) == 0);
	CYBOZU_TEST_EQUAL(f.getSize(), size * 2);
}

CYBOZU_TEST_AUTO(badmode)
{
	cybozu::File f;
	CYBOZU_TEST_ASSERT(!f.open("test", std::ios::in | std::ios::out));
	CYBOZU_TEST_ASSERT(!f.open("test", std::ios::in | std::ios::trunc));
	CYBOZU_TEST_ASSERT(!f.open("test", std::ios::in | std::ios::app));
	CYBOZU_TEST_ASSERT(!f.open("test", std::ios::out | std::ios::trunc | std::ios::app));
}

CYBOZU_TEST_AUTO(move_and_remove)
{
	{
		cybozu::File f;
		CYBOZU_TEST_ASSERT(f.open(fileName, std::ios::out | std::ios::trunc));
		CYBOZU_TEST_ASSERT(f.isOpen());
		CYBOZU_TEST_ASSERT(f.write(testBuf, sizeof(testBuf), cybozu::DontThrow));
	}
	{
		/*
			remove fileName2 if exists and move fileName to fileName2
		*/
		if (cybozu::DoesFileExist(fileName2)) {
			cybozu::RemoveFile(fileName2);
		}
		CYBOZU_TEST_ASSERT(!cybozu::DoesFileExist(fileName2));
		cybozu::RenameFile(fileName, fileName2);
		CYBOZU_TEST_ASSERT(cybozu::DoesFileExist(fileName2));
		cybozu::RemoveFile(fileName2);
	}
	{
		CYBOZU_TEST_EXCEPTION_MESSAGE(cybozu::RemoveFile(fileName2), cybozu::Exception, fileName2);
		CYBOZU_TEST_EXCEPTION_MESSAGE(cybozu::RenameFile(fileName, fileName2), cybozu::Exception, fileName2);
	}
}

CYBOZU_TEST_AUTO(path)
{
	{
		std::string a = "abc\\def\\defg\\\\";
		std::string b = "abc/def/defg//";
		cybozu::ReplaceBackSlash(a);
		CYBOZU_TEST_EQUAL(a, b);
	}

	std::string path, baseName;
	path = cybozu::GetExePath(&baseName);
#ifdef NDEBUG
	const std::string cBaseName = "file_test";
#else
	const std::string cBaseName = "file_testd";
#endif
	CYBOZU_TEST_EQUAL(baseName, cBaseName);
	std::string cPath = "/cybozulib/bin/";
	if (path.size() >= cPath.size()) path = path.substr(path.size() - cPath.size(), std::string::npos);
	CYBOZU_TEST_EQUAL(path, cPath);
}

template<size_t N>
bool findIn(const std::string& name, const char (&tbl)[N][16])
{
	for (size_t i = 0; i < N; i++) {
		if (name == tbl[i]) return true;
	}
	return false;
}

CYBOZU_TEST_AUTO(GetFilesInDir)
{
	std::string path = cybozu::GetExePath() + "../include/cybozu/";
	cybozu::FileList list;
	CYBOZU_TEST_ASSERT(cybozu::GetFileList(list, path, "hpp"));

	const char fileTbl[][16] = {
		"inttype.hpp",
		"file.hpp",
		"exception.hpp",
		"atoi.hpp",
		"stacktrace.hpp",
		"mutex.hpp",
	};
	const char dirTbl[][16] = {
		".",
		"..",
		"nlp",
	};

	size_t foundFileNum = 0;
	size_t foundDirNum = 0;
	for (size_t i = 0; i < list.size(); i++) {
		const std::string& name = list[i].name;
		if (findIn(name, fileTbl)) {
			CYBOZU_TEST_ASSERT(list[i].isFile);
			foundFileNum++;
		}
		if (findIn(name, dirTbl)) {
			CYBOZU_TEST_ASSERT(!list[i].isFile);
			foundDirNum++;
		}
		printf("%s %d\n", list[i].name.c_str(), list[i].isFile);
	}
	CYBOZU_TEST_EQUAL(CYBOZU_NUM_OF_ARRAY(fileTbl), foundFileNum);
	CYBOZU_TEST_EQUAL(CYBOZU_NUM_OF_ARRAY(dirTbl), foundDirNum);
}

CYBOZU_TEST_AUTO(GetBaseName)
{
	const struct {
		const char *name;
		const char *base;
		const char *suf;
	} tbl[] = {
		{ "test", "test", "" },
		{ "test.abc", "test", "abc" },
		{ "test.abc.def", "test.abc", "def" },
		{ ".abc", "", "abc" },
		{ "abc.", "abc", "" },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const std::string name = tbl[i].name;
		CYBOZU_TEST_EQUAL(cybozu::GetBaseName(name), tbl[i].base);
		std::string suf;
		CYBOZU_TEST_EQUAL(cybozu::GetBaseName(name, &suf), tbl[i].base);
		CYBOZU_TEST_EQUAL(suf, tbl[i].suf);

	}
}

CYBOZU_TEST_AUTO(HasSuffix)
{
	const struct {
		const char *name;
		const char *suf;
		bool has;
	} tbl[] = {
		{ "test.txt", "txt", true },
		{ "test.txa", "txt", false },
		{ ".txt", "txt", true },
		{ "asbc", "", true },
		{ "", "a", false },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		CYBOZU_TEST_EQUAL(cybozu::HasSuffix(tbl[i].name, tbl[i].suf), tbl[i].has);
	}
}