/*
	A = U S t(V)
	t(U) U = I
	t(V) V = I
*/
#include <cybozu/test.hpp>
#include <cybozu/nlp/svd.hpp>
#include <cybozu/file.hpp>

const double defaultEPS = 1e-5;

template<class Matrix>
void testSame(const Matrix& A, const Matrix& B)
{
	double x = (A - B).norm();
	CYBOZU_TEST_NEAR(x, 0, defaultEPS);
}

template<class Matrix, class Vector>
void testUSV(const Matrix& U, const Vector& S, const Matrix& V, const Matrix& A)
{
	const Matrix B = U * S.asDiagonal() * V.transpose();
	testSame(A, B);
}
/*
	verity t(A) A = I
*/
template<class Matrix>
void testUnitary(const Matrix& A)
{
	Matrix B = A.transpose() * A;
	Matrix I;
	I.setIdentity(B.cols(), B.cols());
	testSame(B, I);
}

template<class Matrix>
void testSVD(const Matrix& A, int m, int n, int r)
{
	Matrix U, V;
	Eigen::VectorXd S;

	cybozu::nlp::ComputeSVD(U, S, V, A, r);

	CYBOZU_TEST_EQUAL(U.rows(), m);
	CYBOZU_TEST_EQUAL(U.cols(), r);
	CYBOZU_TEST_EQUAL(S.rows(), r);
	CYBOZU_TEST_EQUAL(V.rows(), n);
	CYBOZU_TEST_EQUAL(V.cols(), r);

	testUnitary(U);
	testUnitary(V);

	testUSV(U, S, V, A);
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
	testSame(A, B);
}

CYBOZU_TEST_AUTO(file2)
{
	std::string path = cybozu::file::GetExePath() + "../sample/data/svd/";
	Eigen::MatrixXd A;
	Eigen::MatrixXd B;
	CYBOZU_TEST_ASSERT(cybozu::nlp::svd::LoadMatrix(A, path + "test1"));
	CYBOZU_TEST_ASSERT(cybozu::nlp::svd::LoadMatrix(B, path + "test2"));
	testSame(A, B);
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
	testUnitary(U2);
	testUnitary(V2);
	testSame(S, S2);
	testUSV(U2, S2, V2, A);
}

CYBOZU_TEST_AUTO(multiUnitMatrix)
{
	int row = 13;
	int col = 15;
	int r = 4;
	Eigen::MatrixXd R(col, r);
	cybozu::nlp::svd::InitUnitMatrix(R);
	Eigen::MatrixXd A(row, col), B;
	cybozu::nlp::svd::InitRandomMatrix(A);
	cybozu::nlp::svd::CompressCol(B, A, r);
	double diff = ((A * R) - B).norm();
	CYBOZU_TEST_NEAR(diff, 0, defaultEPS);
}
