#pragma once
/**
	@file
	@brief wrapper of MeCab

	Copyright (C) 2007-2012 Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include "mecab.h"
#include <cybozu/exception.hpp>
#ifdef _WIN32
#pragma comment(lib, "libmecab.lib")
#endif

namespace cybozu { namespace nlp {

struct MecabException : cybozu::Exception {
	MecabException() : cybozu::Exception("nlp::mecab") { }
};

struct Mecab {
	Mecab(const char *option = "-O wakati")
		: tagger_(MeCab::createTagger(option))
		, node_(0)
	{
		if (tagger_ == 0) {
			MecabException e;
			e << "createTagger";
			throw e;
		}
	}
	/**
		T must have push_back(std::string)
	*/
	template<class T>
	bool parse(T& out, const char *str, size_t strLen = 0)
	{
		if (strLen == 0) {
			strLen = strlen(str);
		}
		const char *p = tagger_->parse(str, strLen);
		if (p == 0) return false;
		while (*p) {
			if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') {
				p++;
				continue;
			}
			const char *q = strchr(p, ' ');
			if (q == 0) {
				out.push_back(p);
				break;
			}
			out.push_back(std::string(p, q));
			p = q + 1;
		}
		return true;
	}
	void set(const char *str, size_t strLen = 0)
	{
		if (strLen == 0) {
			strLen = strlen(str);
		}
		node_ = tagger_->parseToNode(str, strLen);
	}
	bool isEnd() const { return node_->stat == MECAB_EOS_NODE; }
	const char *getPos() const { return node_->surface; }
	size_t getSize() const { return node_->length; }
	/* adhoc */
	bool isNoun() const
	{
		const char *p = node_->feature;
		return p[0] == '\xE5' && p[1] == '\x90' && p[2] == '\x8D';
	}
	void next()
	{
		node_ = node_->next;
	}
	~Mecab()
	{
		delete tagger_;
	}
private:
	MeCab::Tagger *tagger_;
	const MeCab::Node *node_;
};

} } // cybozu::nlp
