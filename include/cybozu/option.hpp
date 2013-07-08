#pragma once
/**
	@file
	@brief command line parser

	Copyright (C) Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdlib.h>
#include <assert.h>
#include <cybozu/exception.hpp>
#include <cybozu/atoi.hpp>

/*
	progName (-opt val ...) param1 param2 ...
*/

namespace cybozu {

struct OptionError : public cybozu::Exception {
	enum Type {
		NoError = 0,
		BAD_OPT = 1,
		BAD_VALUE,
		NO_VALUE,
		OPT_IS_NECESSARY,
		PARAM_IS_NECESSARY,
		REDUNDANT_VAL
	};
	Type type;
	int argPos;
	OptionError()
		: cybozu::Exception("OptionError")
		, type(NoError)
		, argPos(0)
	{
	}
	cybozu::Exception& set(Type type, int argPos = 0)
	{
		this->type = type;
		this->argPos = argPos;
		switch (type) {
		case BAD_OPT:
			(*this) << "bad opt";
			break;
		case BAD_VALUE:
			(*this) << "bad value";
			break;
		case NO_VALUE:
			(*this) << "no value";
			break;
		case OPT_IS_NECESSARY:
			(*this) << "opt is necessary";
			break;
		case PARAM_IS_NECESSARY:
			(*this) << "param is necessary";
			break;
		case REDUNDANT_VAL:
			(*this) << "redundant argVal";
			break;
		default:
			break;
		}
		return *this;
	}
};

namespace option_local {

template<class T>
bool convert(T* x, const char *str)
{
	std::istringstream is(str);
	is >> *x;
	return is != 0;
}

template<class T>
bool convertInt(T* x, const char *str)
{
	size_t len = strlen(str);
	int factor = 1;
	if (len > 1) {
		switch (str[len - 1]) {
		case 'k': factor = 1000; len--; break;
		case 'm': factor = 1000 * 1000; len--; break;
		case 'K': factor = 1024; len--; break;
		case 'M': factor = 1024 * 1024; len--; break;
		default: return false;
		}
	}
	bool b;
	T y = cybozu::atoi(&b, str, len);
	if (factor > 1) {
		T yfactor = T(y * factor);
		if (T(yfactor / factor) != y) return false;
		*x = yfactor;
	} else {
		*x = y;
	}
	return true;
}

#define CYBOZU_OPTION_DEFINE_CONVERT_INT(type) \
template<>bool convert(type* x, const char *str) { return convertInt(x, str); }

CYBOZU_OPTION_DEFINE_CONVERT_INT(short)
CYBOZU_OPTION_DEFINE_CONVERT_INT(int)
//CYBOZU_OPTION_DEFINE_CONVERT_INT(long)
//CYBOZU_OPTION_DEFINE_CONVERT_INT(long long)
CYBOZU_OPTION_DEFINE_CONVERT_INT(unsigned short)
CYBOZU_OPTION_DEFINE_CONVERT_INT(unsigned int)
//CYBOZU_OPTION_DEFINE_CONVERT_INT(unsigned long)
//CYBOZU_OPTION_DEFINE_CONVERT_INT(unsigned long long)

#undef CYBOZU_OPTION_DEFINE_CONVERT_INT

struct HolderBase {
	virtual ~HolderBase(){}
	virtual bool set(const char*) = 0;
	virtual HolderBase *clone() const = 0;
	virtual std::string toStr() const = 0;
};

template<class T>
struct Holder : public HolderBase {
	T *p_;
	Holder(T *p) : p_(p) {}
	HolderBase *clone() const { return new Holder(p_); }
	bool set(const char *str) { return option_local::convert(p_, str); }
	std::string toStr() const
	{
		std::ostringstream os;
		os << *p_;
		return os.str();
	}
};

template<class T, class Alloc, template<class T_, class Alloc_>class Container>
struct Holder<Container<T, Alloc> > : public HolderBase {
	typedef Container<T, Alloc> Vec;
	Vec *p_;
	Holder(Vec *p) : p_(p) {}
	HolderBase *clone() const { return new Holder<Vec>(p_); }
	bool set(const char *str)
	{
		T t;
		bool b = option_local::convert(&t, str);
		if (b) p_->push_back(t);
		return b;
	}
	std::string toStr() const
	{
		std::ostringstream os;
		bool isFirst = true;
		for (typename Vec::const_iterator i = p_->begin(), ie = p_->end(); i != ie; ++i) {
			if (isFirst) {
				isFirst = false;
			} else {
				os << ' ';
			}
			os << *i;
		}
		return os.str();
	}
};

class Var {
	HolderBase *p_;
	bool isSet_;
public:
	Var() : p_(0), isSet_(false) { }
	Var(const Var& rhs) : p_(rhs.p_->clone()), isSet_(false) { }
	template<class T>
	explicit Var(T *x) : p_(new Holder<T>(x)), isSet_(false) { }

	~Var() { delete p_; }

	void swap(Var& rhs) throw()
	{
		std::swap(p_, rhs.p_);
	}

	void operator=(const Var& rhs)
	{
		Var v(rhs);
		swap(v);
	}
	bool set(const char *str)
	{
		isSet_ = true;
		return p_->set(str);
	}
	std::string toStr() const { return p_->toStr(); }
	bool isSet() const { return isSet_; }
};

} // option_local

/*
	Option parser
	see sample/option_smpl.cpp

	progName (opt1-name|opt2-name|...) param1-name param2-name ...
	  param1-name:param1-help
	  param2-name:param2-help
	  -opt1-name:opt1-help
	  ...
*/
class Option {
	enum Mode {
		N_is0 = 0,
		N_is1 = 1,
		N_any = 2
	};
	struct Info {
		option_local::Var var;
		Mode mode; // 0 or 1 or any
		bool isMust; // this option is must
		std::string opt; // option param without '-'
		std::string help; // description of option

		Info() : mode(N_is0) {}
		template<class T>
		Info(T* pvar, Mode mode, bool isMust, const char *opt, const char *help)
			: var(pvar)
			, mode(mode)
			, isMust(isMust)
			, opt(opt)
			, help(help)
		{
		}
		void put() const
		{
			printf("%s=%s (%s)\n", opt.c_str(), var.toStr().c_str(), var.isSet() ? "set" : "default");
		}
		void usage() const
		{
			printf("  -%s %s [%s]\n", opt.c_str(), help.c_str(), isMust ? "must" : "opt");
		}
	};
	typedef std::vector<Info> InfoVec;
	typedef std::vector<std::string> StrVec;
	struct Param {
		std::string *pvar;
		std::string name;
		std::string help;
		Param() {}
		Param(std::string *pvar, const std::string& name, const std::string& help)
			: pvar(pvar), name(name), help(help) { }
	};
	typedef std::vector<Param> ParamVec;
	typedef std::map<std::string, size_t> OptMap;
	const char *progName_;
	InfoVec infoVec_;
	ParamVec paramVec_;
	Info remains_;
	bool permitVariableParam_;
	OptMap optMap_;
	template<class T>
	void appendSub(T *pvar, Mode mode, bool isMust, const char *opt, const char *help)
	{
		if (optMap_.find(opt) != optMap_.end()) {
			throw cybozu::Exception("Option::append:duplicate option") << opt;
		}
		optMap_[opt] = infoVec_.size();
		infoVec_.push_back(Info(pvar, mode, isMust, opt, help));
	}

	template<class T, class U>
	void append(T *pvar, const U& defaultVal, bool isMust, const char *opt, const char *help = "")
	{
		*pvar = defaultVal;
		appendSub(pvar, N_is1, isMust, opt, help);
	}
	void append(bool *pvar, const bool& defaultVal, bool isMust, const char *opt, const char *help = "")
	{
		*pvar = defaultVal;
		appendSub(pvar, N_is0, isMust, opt, help);
	}
public:
	Option()
		: progName_(0)
		, permitVariableParam_(false)
	{
	}
	virtual ~Option() {}
	/*
		append optional option with default value
		@param pvar [in] pointer to option variable
		@param defaultVal [in] default value
		@param opt [in] option name
		@param help [in] option help
		@note you can use 123k, 56M if T is integer type.
		k : *1000
		m : *1000000
		K : *1024
		M : *1024*1024
	*/
	template<class T, class U>
	void appendOpt(T *pvar, const U& defaultVal, const char *opt, const char *help = "")
	{
		append(pvar, defaultVal, false, opt, help);
	}
	/*
		append necessary option
		@param pvar [in] pointer to option variable
		@param opt [in] option name
		@param help [in] option help
	*/
	template<class T>
	void appendMust(T *pvar, const char *opt, const char *help = "")
	{
		append(pvar, T(), true, opt, help);
	}
	/*
		append vector option
		@param pvar [in] pointer to option variable
		@param opt [in] option name
		@param help [in] option help
	*/
	template<class T, class Alloc, template<class T_, class Alloc_>class Container>
	void appendVec(Container<T, Alloc> *pvar, const char *opt, const char *help = "")
	{
		appendSub(pvar, N_any, false, opt, help);
	}
	/*
		append parameter
		@param pvar [in] pointer to parameter
		@param opt [in] option name
		@param help [in] option help
	*/
	void appendParam(std::string *pvar, const char *name, const char *help = "")
	{
		pvar->clear();
		paramVec_.push_back(Param(pvar, name, help));
	}
	/*
		append remain parameter
		@param pvar [in] pointer to vector of parameter
		@param opt [in] option name
		@param help [in] option help
	*/
	template<class T, class Alloc, template<class T_, class Alloc_>class Container>
	void appendParamVec(Container<T, Alloc> *pvar, const char *name, const char *help = "")
	{
		if (permitVariableParam_) throw cybozu::Exception("Option:appendParamVec:already appendParamVec is called");
		permitVariableParam_ = true;
		remains_.var = option_local::Var(pvar);
		remains_.mode = N_any;
		remains_.isMust = true;
		remains_.opt = name;
		remains_.help = help;
	}
	/*
		parse (argc, argv)
		@param argc [in] argc of main
		@param argv [in] argv of main
		@param doThrow [in] whether throw exception or return false
	*/
	bool parse(int argc, char *argv[], bool doThrow = false)
	{
		progName_ = argv[0];
		OptionError err;
		for (int pos = 1; pos < argc; pos++) {
			if (argv[pos][0] == '-') {
				const std::string str = argv[pos] + 1;
				OptMap::const_iterator i = optMap_.find(str);
				if (i == optMap_.end()) {
					err.set(OptionError::BAD_OPT, pos);
					goto ERR;
				}

				Info& info = infoVec_[i->second];
				switch (info.mode) {
				case N_is0:
					if (!info.var.set("1")) {
						err.set(OptionError::BAD_VALUE, pos);
						goto ERR;
					}
					break;
				case N_is1:
					if (argc <= 1) goto ERR;
					pos++;
					if (!info.var.set(argv[pos])) {
						err.set(OptionError::BAD_VALUE, pos) << (std::string(argv[pos]) + " for -" + info.opt);
						goto ERR;
					}
					break;
				case N_any:
				default:
					{
						pos++;
						int i = 0;
						while (pos < argc && argv[pos][0] != '-') {
							if (!info.var.set(argv[pos])) {
								err.set(OptionError::BAD_VALUE, pos) << (std::string(argv[pos]) + " for -" + info.opt) << i;
								goto ERR;
							}
							pos++;
							i++;
						}
						if (i > 0) {
							pos--;
						} else {
							err.set(OptionError::NO_VALUE, pos) << (std::string("for -") + info.opt);
							goto ERR;
						}
					}
					break;
				}
			} else {
				bool used = false;
				for (size_t i = 0; i < paramVec_.size(); i++) {
					Param& param = paramVec_[i];
					if (param.pvar->empty()) {
						*param.pvar = argv[pos];
						used = true;
						break;
					}
				}
				if (!used) {
					if (permitVariableParam_) {
						remains_.var.set(argv[pos]);
					} else {
						err.set(OptionError::REDUNDANT_VAL, pos) << argv[pos];
						goto ERR;
					}
				}
			}
		}
		// check whether must-opt is set
		for (size_t i = 0; i < infoVec_.size(); i++) {
			const Info& info = infoVec_[i];
			if (info.isMust && !info.var.isSet()) {
				err.set(OptionError::OPT_IS_NECESSARY) << info.opt;
				goto ERR;
			}
		}
		// check whether param is set
		for (size_t i = 0; i < paramVec_.size(); i++) {
			const Param& param = paramVec_[i];
			if (param.pvar->empty()) {
				err.set(OptionError::PARAM_IS_NECESSARY) << param.name;
				goto ERR;
			}
		}
		// check whether remains is set
		if (permitVariableParam_ && !remains_.var.isSet()) {
			err.set(OptionError::PARAM_IS_NECESSARY) << remains_.opt;
			goto ERR;
		}
		return true;
	ERR:
		assert(err.type);
		if (doThrow) throw err;
		printf("err %s\n", err.what());
		return false;
	}
	void usage() const
	{
		printf("usage:%s (option)", progName_);
		for (size_t i = 0; i < paramVec_.size(); i++) {
			printf(" %s", paramVec_[i].name.c_str());
		}
		if (permitVariableParam_) {
			printf(" %s", remains_.opt.c_str());
		}
		printf("\n");
		for (size_t i = 0; i < paramVec_.size(); i++) {
			const Param& param = paramVec_[i];
			if (!param.help.empty()) printf("  %s:%s\n", paramVec_[i].name.c_str(), paramVec_[i].help.c_str());
		}
		if (!remains_.help.empty()) printf("  %s:%s\n", remains_.opt.c_str(), remains_.help.c_str());
		for (size_t i = 0; i < infoVec_.size(); i++) {
			infoVec_[i].usage();
		}
	}
	void put() const
	{
		for (size_t i = 0; i < paramVec_.size(); i++) {
			const Param& param = paramVec_[i];
			printf("%s=%s\n", param.name.c_str(), param.pvar->c_str());
		}
		if (permitVariableParam_) {
			printf("remains=%s\n", remains_.var.toStr().c_str());
		}
		for (size_t i = 0; i < infoVec_.size(); i++) {
			infoVec_[i].put();
		}
	}
};

} // cybozu
