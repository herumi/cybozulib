#pragma once
/**
	@file
	@brief probabilistic SVD

	Copyright (C) 2012 Cybozu Labs, Inc., all rights reserved.
*/
#include <assert.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cybozu/nlp/random.hpp>
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4714) // force inline
#endif
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Eigenvalues>
#ifdef _MSC_VER
//	#pragma warning(pop)
#endif

/***
	text format

	Matrix(dense)
	---
	# M D <row> <col>
	data1_1 data1_2 data1_3 ...
	data2_1 data2_2 ...
	....
	---

	Matrix(sparse)
	---
	# M S <row> <col>
	c1:data1_c1 c2:data1_c2 c3:data1_c3 ...
	c1:data2_c1 c2:data2_c2 c3:data2_c3 ...
	....
	---

	ex.
	M = (1.0 2.0 3.0)
	    (1.2 2.4 3.5)
	---
	# M D 2 3
	1.0 2.0 3.0
	1.2 2.4 3.5
	---

	M = (1.0 0   3.0)
	    (0   4.2 0  )
	---
	# M S 2 3
	0:1.0 2:3.0
	1:4.2
	---
*/
namespace cybozu { namespace nlp {

namespace svd {

template<class Matrix>
void InitRandomMatrix(Matrix& M)
{
	cybozu::nlp::NormalRandomGenerator r;
	for (int i = 0; i < M.rows(); i++) {
		for (int j = 0; j < M.cols(); j++) {
			M(i, j) = typename Matrix::Scalar(r.get());
		}
	}
}

template<class Matrix>
void OrthonormalizeMatrix(Matrix& M)
{
	const double eps = 1e-5;
	typedef typename Matrix::Scalar Double;
	for (int i = 0; i < M.cols(); i++) {
		double norm = M.col(i).norm();
		if (norm < eps) {
			M.col(i).setZero();
		} else {
			Double rev = Double(1.0 / norm);
			M.col(i) *= rev;
			for (int j = i + 1; j < M.cols(); j++) {
				Double x = M.col(i).dot(M.col(j));
				M.col(j) -= M.col(i) * x;
			}
		}
	}
}

inline bool LoadHeader(bool *isMatrix, bool *isSparse, int *row, int *col, std::istream& ifs)
{
	std::string line;
	if (std::getline(ifs, line)) {
		std::istringstream is(line);
		char c, vec, type;
		is >> c >> vec >> type >> *row >> *col;
		if (c != '#') {
			fprintf(stderr, "top char is #(%c)\n", c);
			goto ERR;
		}
		if (vec != 'M' && vec != 'V') {
			fprintf(stderr, "vec is M(matrix) or V(vector) (%c)\n", vec);
			goto ERR;
		}
		*isMatrix = vec == 'M';
		if (type != 'S' && type != 'D') {
			fprintf(stderr, "type is D(dense) or S(sparse) (%c)\n", type);
			goto ERR;
		}
		*isSparse = type == 'S';
		if (*row <= 0 || *col <= 0) {
			fprintf(stderr, "row(%d) and col(%d) should be positive\n", *row, *col);
			goto ERR;
		}
		fprintf(stderr, "input (%c, %c, %d, %d)\n", vec, type, *row, *col);
		return true;
	}
ERR:
	fprintf(stderr, "bad format top line must be '# (M|V) (D|S) <row> <col>'\n");
	return false;
}

template<class Matrix>
bool LoadMatrix(Matrix& A, const std::string& input)
{
	std::ifstream ifs(input.c_str(), std::ios::binary);
	bool isMatrix = false;
	bool isSparse = false;
	int row = 0, col = 0;
	if (!LoadHeader(&isMatrix, &isSparse, &row, &col, ifs) || !isMatrix) {
		return false;
	}
	A.resize(row, col);
	if (isSparse) {
		for (int i = 0; i < row; i++) {
			A.row(i).setZero();
			std::string line;
			if (!std::getline(ifs, line)) {
				fprintf(stderr, "can't read %d line\n", i);
				return false;
			}
			std::istringstream is(line);
			for (;;) {
				int idx;
				char sep;
				double v;
				is >> idx >> sep >> v;
				if (!is) break;
				if (sep != ':' || idx < 0 || idx >= col) {
					fprintf(stderr, "can't read %s\n", line.c_str());
					return false;
				}
				A(i, idx) = typename Matrix::Scalar(v);
			}
		}
	} else {
		for (int i = 0; i < row; i++) {
			for (int j = 0; j < col; j++) {
				double v;
				ifs >> v;
				if (!ifs) {
					fprintf(stderr, "can't read (%d,%d)\n", i, j);
					return false;
				}
				A(i, j) = typename Matrix::Scalar(v);
			}
		}
	}
	return true;
}

template<class Matrix>
bool SaveMatrix(const std::string& outName, const Matrix& M)
{
	std::ofstream ofs(outName.c_str(), std::ios::binary);
	ofs << std::setprecision(8);

	ofs << "# M D " << M.rows() << " " << M.cols() << std::endl;
	for (int i = 0; i < M.rows(); i++) {
		for (int j = 0; j < M.cols(); j++) {
			if (j > 0) ofs << ' ';
			ofs << M(i, j);
		}
		ofs << std::endl;
	}
	return ofs.good();
}

template<class Vector>
bool SaveVector(const std::string& outName, const Vector& V)
{
	std::ofstream ofs(outName.c_str(), std::ios::binary);
	ofs << std::setprecision(8);
	ofs << "# V D " << V.rows() << std::endl;
	for (int i = 0; i < V.rows(); i++) {
		ofs << V(i) << std::endl;
	}
	return ofs.good();
}

} // svd

/*
	approximate singular value decomposition
	A = U S t(V) with rank r

	t(M) : transpose of M

	R1 : random
	Y = t(A) R1
	Y = orthonormalize(Y) ; t(Y) Y = I
	B = A Y
	R2 : random
	Z = B R2
	Z = orthonormalize(Z) ; t(Z) Z = I
	C = t(Z) B
	C = U' S t(V')
	A \simeq A Y t(Y)
	 = B t(Y)
     \simeq Z t(Z) B t(Y)
	 = Z C t(Y)
	 = Z U' S t(V') t(Y)
	 = (Z U') S t(YV')
	 = U S V

	see http://code.google.com/p/redsvd/

	Copyright (c) 2010 Daisuke Okanohara

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	1. Redistributions of source code must retain the above Copyright
	   notice, this list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above Copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.

	3. Neither the name of the authors nor the names of its contributors
	   may be used to endorse or promote products derived from this
	   software without specific prior written permission.
*/
template<class Matrix, class Vector>
bool ComputeSVD(Matrix& U, Vector& S, Matrix& V, const Matrix& A, int rank)
{
	const int r = std::min<int>(std::min(A.cols(), A.rows()), rank);
	if (r <= 0) return false;

	Matrix R1(A.rows(), r);
	svd::InitRandomMatrix(R1);
fprintf(stderr, "generate R1\n");
	Matrix Y = A.transpose() * R1;
	svd::OrthonormalizeMatrix(Y);
fprintf(stderr, "normalize Y\n");
	const Matrix B = A * Y;
fprintf(stderr, "calc B\n");
	Matrix R2(B.cols(), r);
	svd::InitRandomMatrix(R2);
fprintf(stderr, "generate R2\n");
	Matrix Z = B * R2;
	svd::OrthonormalizeMatrix(Z);
fprintf(stderr, "normalize Z\n");
	Matrix C = Z.transpose() * B;
fprintf(stderr, "computing SVD\n");
	Eigen::JacobiSVD<Matrix> svd(C, Eigen::ComputeThinU | Eigen::ComputeThinV);
	U = Z * svd.matrixU();
fprintf(stderr, "end of SVD\n");
	S = svd.singularValues();
	V = Y * svd.matrixV();
	return true;
}

} } // cybozu::nlp

