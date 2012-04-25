/*
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
#include <cybozu/test.hpp>
#include <cybozu/nlp/svd.hpp>
#include <cybozu/file.hpp>

const double defaultEPS = 1e-5;

template<class Matrix>
void testColIsOrthgonal(const Matrix& M, double eps = defaultEPS)
{
	for (int i = 0; i < M.cols(); i++) {
		double norm = M.col(i).norm();
		CYBOZU_TEST_NEAR(norm, 1.0, eps);
		for (int j = i + 1; j < M.cols(); j++) {
			double x = M.col(i).dot(M.col(j));
			CYBOZU_TEST_NEAR(x, 0, eps);
		}
	}
}

template<class Matrix, class Vector>
void testSameUSV(const Matrix& U, const Vector& S, const Matrix& V, const Matrix& A, double eps = defaultEPS)
{
	double x = (A - U * S.asDiagonal() * V.transpose()).norm();
	CYBOZU_TEST_NEAR(x, 0, eps);
}

template<class Matrix>
void testSameMatrix(const Matrix& A, const Matrix& B)
{
	CYBOZU_TEST_EQUAL(A.cols(), B.cols());
	CYBOZU_TEST_EQUAL(A.rows(), B.rows());
	for (int i = 0; i < A.rows(); i++) {
		for (int j = 0; j < A.cols(); j++) {
			CYBOZU_TEST_NEAR(A(i, j), B(i, j), defaultEPS);
		}
	}
}

template<class Matrix>
void testSVD(const Matrix& A, int m, int n, int r, double eps = defaultEPS)
{
	Matrix U, V;
	Eigen::VectorXd S;

	cybozu::nlp::ComputeSVD(U, S, V, A, r);

	CYBOZU_TEST_EQUAL(U.rows(), m);
	CYBOZU_TEST_EQUAL(U.cols(), r);
	CYBOZU_TEST_EQUAL(S.rows(), r);
	CYBOZU_TEST_EQUAL(V.rows(), n);
	CYBOZU_TEST_EQUAL(V.cols(), r);

	testColIsOrthgonal(U, eps);
	testColIsOrthgonal(V, eps);

	testSameUSV(U, S, V, A, eps);
}

CYBOZU_TEST_AUTO(rank2)
{
	const int m = 4;
	const int n = 3;
	const int r = 2;
	Eigen::MatrixXd A(m, n);
	A << 1, 2, 3, 4,
	     5, 6, 7, 8,
	     9, 10, 11, 12;

	testSVD(A, m, n, r);
}

CYBOZU_TEST_AUTO(rank3)
{
	const int m = 4;
	const int n = 3;
	const int r = 3;
	Eigen::MatrixXd A(m, n);
	A << 1, 2, 3, 4,
	     5, 6, 7, 8,
	     9, 10, 11, 13;

	testSVD(A, m, n, r);
}

CYBOZU_TEST_AUTO(file)
{
	const int m = 3;
	const int n = 4;
	const std::string testName = cybozu::file::GetExePath() + "svd_test.txt";
	Eigen::MatrixXd A(m, n);
	cybozu::nlp::svd::InitRandomMatrix(A);
	CYBOZU_TEST_ASSERT(cybozu::nlp::svd::SaveMatrix(testName, A));
	Eigen::MatrixXd B;
	CYBOZU_TEST_ASSERT(cybozu::nlp::svd::LoadMatrix(B, testName));
	testSameMatrix(A, B);
}

CYBOZU_TEST_AUTO(file2)
{
	std::string path = cybozu::file::GetExePath() + "../sample/data/svd/";
	Eigen::MatrixXd A;
	Eigen::MatrixXd B;
	CYBOZU_TEST_ASSERT(cybozu::nlp::svd::LoadMatrix(A, path + "test1"));
	CYBOZU_TEST_ASSERT(cybozu::nlp::svd::LoadMatrix(B, path + "test2"));
	testSameMatrix(A, B);
}

CYBOZU_TEST_AUTO(random1)
{
	const int m = 10;
	const int n = 30;
	const int r = 10;
	Eigen::MatrixXd A(m, n);
	cybozu::nlp::svd::InitRandomMatrix(A);
	testSVD(A, m, n, r);
}

CYBOZU_TEST_AUTO(random2)
{
	const int m = 10;
	const int n = 30;
	const int r = 8;
	Eigen::MatrixXd A(m, n), U(m, r), V(n, r), U2, V2;
	Eigen::VectorXd S(r), S2;
	cybozu::nlp::svd::InitRandomMatrix(U);
	cybozu::nlp::svd::OrthonormalizeMatrix(U);
	cybozu::nlp::svd::InitRandomMatrix(V);
	cybozu::nlp::svd::OrthonormalizeMatrix(V);
	// descending order
	for (int i = 0; i < r; i++) {
		S(i) = r - i;
	}
	A = U * S.asDiagonal() * V.transpose();
	cybozu::nlp::ComputeSVD(U2, S2, V2, A, r);
	for (int i = 0; i < r; i++) {
		double x = U.col(i).dot(U2.col(i));
		CYBOZU_TEST_NEAR(x, 1.0, defaultEPS);
		x = V.col(i).dot(V2.col(i));
		CYBOZU_TEST_NEAR(x, 1.0, defaultEPS);
		CYBOZU_TEST_NEAR(S(i), S2(i), defaultEPS);
	}
}

