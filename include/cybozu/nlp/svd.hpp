#pragma once
/**
	@file
	@brief fast non-probabilistic SVD

	Copyright (C) 2012 Cybozu Labs, Inc., all rights reserved.
	@author MITSUNARI Shigeo
*/
#include <assert.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
//#define CYBOZU_NLP_SVD_USE_RANDOM
#ifdef CYBOZU_NLP_SVD_USE_RANDOM
#include <cybozu/nlp/random.hpp>
#endif
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

#ifdef CYBOZU_NLP_SVD_USE_RANDOM
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
#endif

template<class Matrix>
void InitUnitMatrix(Matrix& m)
{
	typedef typename Matrix::Scalar Double;
	m.setZero();
	const int row = m.rows();
	const int col = m.cols();
	assert(col <= row);
#if 1
	const int adj = 0;//(col & 1) ? row/2 : 0;
	for (int i = 0; i < row; i++) {
		m(i, (i * col + adj) / row) = 1;
	}
#else
	const int q0 = row / col;
	const int r0 = row % col;
	const double rcol = 1.0 / col;
	int b = 0;
	int q = q0;
	int e = r0;
	int rowIdx = 0;
	int colIdx = 0;
	for (;;) {
		if (b > 0) {
			m(rowIdx, colIdx) = Double(b * rcol);
			rowIdx++;
		}
		for (int j = 0; j < q; j++) {
			m(rowIdx, colIdx) = 1;
			rowIdx++;
		}
		if (e > 0) {
			m(rowIdx, colIdx) = Double(e * rcol);
		}
		if (colIdx == col - 1) break;
		b = e == 0 ? 0 : col - e;
		e = r0 - b;
		if (e < 0) {
			q = q0 - 1;
			e += col;
		} else {
			q = q0;
		}
		colIdx++;
	}
	assert(rowIdx == row);
#endif
}
/*
	m(row, col) => M(row, r)
	r <= col
*/
template<class Matrix1, class Matrix2>
void CompressCol(Matrix1& out, const Matrix2& m, int r)
{
	typedef typename Matrix1::Scalar Double;
	const int row = m.rows();
	const int col = m.cols();
	assert(r <= col);
	out.resize(row, r);
#if 1
	int begin = 0;
	for (int j = 0; j < r; j++) {
		int end = std::min(((j + 1) * col + r - 1) / r, col);
//		printf("%d [%d, %d)\n", j, begin, end);
		for (int i = 0; i < row; i++) {
			double x = 0;
			for (int k = begin; k < end; k++) {
				x += m(i, k);
			}
			out(i, j) = Double(x);
		}
		begin = end;
	}
#else
	const int q0 = col / r;
	const int r0 = col % r;
	const double rr = 1.0 / r;
	int b = 0;
	int q = q0;
	int e = r0;
	int colIdx = 0;
	int rIdx = 0;
	for (;;) {
		for (int i = 0; i < row; i++) {
			double x = 0;
			int k = colIdx;
			if (b > 0) {
				x += m(i, k) * b * rr;
				k++;
			}
			for (int j = 0; j < q; j++) {
				x += m(i, k);
				k++;
			}
			if (e > 0) {
				x += m(i, k) * e * rr;
			}
			out(i, rIdx) = Double(x);
		}
		if (b > 0) colIdx++;
		colIdx += q;
		if (rIdx == r - 1) break;
		b = e == 0 ? 0 : r - e;
		e = r0 - b;
		if (e < 0) {
			q = q0 - 1;
			e += r;
		} else {
			q = q0;
		}
		rIdx++;
	}
	assert(colIdx == col);
#endif
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
	t(U) U = I
	t(V) V = I

	R1 : compressed unit matrix
	Y = t(A) R1
	Y = orthonormalize(Y) ; t(Y) Y = I
	B = A Y
	Z = orthonormalize(B) ; t(Z) Z = I
	C = t(Z) B
	C = U' S t(V')
	A \simeq A Y t(Y)
	 = B t(Y)
     \simeq Z t(Z) B t(Y)
	 = Z C t(Y)
	 = Z U' S t(V') t(Y)
	 = (Z U') S t(YV')
	 = U S V
*/
template<class Matrix, class Vector>
bool ComputeSVD(Matrix& U, Vector& S, Matrix& V, const Matrix& A, int rank)
{
	const int row = A.rows();
	const int col = A.cols();
	const int r = std::min(std::min(col, row), rank);
	if (r <= 0) return false;

#if 0
	Matrix R1(row, r);
	svd::InitRandomMatrix(R1);
//	svd::InitUnitMatrix(R1);
	Matrix Y = A.transpose() * R1;
#else
	Matrix Y;
	svd::CompressCol(Y, A.transpose(), r);
#endif
	svd::OrthonormalizeMatrix(Y);
	const Matrix B = A * Y;
	Matrix Z = B;
	svd::OrthonormalizeMatrix(Z);
	const Matrix C = Z.transpose() * B;
	const Eigen::JacobiSVD<Matrix> svd(C, Eigen::ComputeThinU | Eigen::ComputeThinV);
	U = Z * svd.matrixU();
	S = svd.singularValues();
	V = Y * svd.matrixV();
	return true;
}

} } // cybozu::nlp

