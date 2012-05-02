#include <iostream>
#include <stdio.h>
#include <math.h>
#include <cybozu/inttype.hpp>
#include <cybozu/nlp/svd.hpp>
#include <cybozu/time.hpp>

template<class Matrix>
void SetUnitary(Matrix& m)
{
	cybozu::nlp::svd::InitRandomMatrix(m);
	cybozu::nlp::svd::OrthonormalizeMatrix(m);
}

double getTime()
{
	cybozu::Time t;
	t.setCurrentTime();
	return t.getTime() + t.getMsec() * 1e-3;
}

template<class Matrix, class Vector>
void test(int row, int col, int rank, int tryRank)
{
	printf("row=%d, col=%d, rank=%d, tryRank=%d ", row, col, rank, tryRank);
	typedef typename Matrix::Scalar Double;
	Matrix U(row, rank);
	Vector S(rank);
	Matrix V(col, rank);
	SetUnitary(U);
	SetUnitary(V);
	for (int i = 0; i < rank; i++) {
		S(i) = (Double)(3 * pow(0.9, i));
	}
	Matrix A = U * S.asDiagonal() * V.transpose();
	Matrix UO, VO;
	Vector SO;
	{
		double b = getTime();
		cybozu::nlp::ComputeSVD(UO, SO, VO, A, tryRank);
		double e = getTime();
		printf("%.2fmsec ", (e - b) * 1e3);
	}
	if (row * col <= 400000) {
		double b = getTime();
		Eigen::JacobiSVD<Matrix> svd(A, Eigen::ComputeThinU | Eigen::ComputeThinV);
		double e = getTime();
		printf("%.2fmsec ", (e - b) * 1e3);
	} else {
		printf("---msec ");
	}

	double sum_s = 0;
	double sum_v1 = 0;
	double sum_v2 = 0;
	for (int i = 0; i < tryRank; ++i) {
		double v1 = fabs(U.col(i).dot(UO.col(i)));
		double v2 = fabs(V.col(i).dot(VO.col(i)));
		sum_s += fabs(S(i) - SO(i));
		sum_v1 += v1;
		sum_v2 += v2;
	}
	sum_s /= tryRank;
	sum_v1 /= tryRank;
	sum_v2 /= tryRank;
	printf("%f %f %f\n", sum_s, sum_v1, sum_v2);
}

int main()
{
	const struct {
		int row;
		int col;
		int rank;
		int tryRank;
	} tbl[] = {
		{ 100, 100, 10, 10 },
		{ 100, 100, 10, 9 },
		{ 100, 100, 10, 8 },
		{ 100, 100, 50, 50 },
		{ 100, 100, 50, 49 },
		{ 100, 100, 50, 48 },
		{ 300, 300, 50, 48 },
		{ 600, 600, 100, 95 },
		{ 1000, 1000, 10, 10 },
		{ 1000, 1000, 10, 9 },
		{ 1000, 1000, 10, 8 },
		{ 1000, 1000, 20, 20 },
		{ 1000, 1000, 20, 19 },
		{ 1000, 1000, 20, 18 },
		{ 1000, 1000, 100, 100 },
		{ 1000, 1000, 100, 99 },
		{ 1000, 1000, 100, 90 },
		{ 10000, 10000, 100, 100 },
		{ 10000, 10000, 100, 90 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		test<Eigen::MatrixXd, Eigen::VectorXd>(tbl[i].row, tbl[i].col, tbl[i].rank, tbl[i].tryRank);
	}
#if 0
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		test<Eigen::MatrixXf, Eigen::VectorXf>(tbl[i].row, tbl[i].col, tbl[i].rank, tbl[i].tryRank);
	}
#endif
}
