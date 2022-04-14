#pragma once
#pragma once
#include <cmath>
#include <cstdlib>
#include <random>
#include <cstdio>
#include <tbb/tbb.h>

class Matrix;

template<typename A>
class TBBMatrixBody {
	A action;
public:
	TBBMatrixBody(A func) : action(func) {}
	void operator()(const tbb::blocked_range<int>& r) {
		for (auto i = r.begin(); i != r.end(); i++) {
			action(i);
		}
	}
};


class Matrix {
	double** data = nullptr;
	int rows;
	int cols;

	void clearData() {
		if (data && rows > 0) {
			for (int i = 0; i < rows; i++) {
				delete[] data[i];
			}
			delete[] data;
		}
	}

	void copy(Matrix& other) {
		this->rows = other.rows;
		this->cols = other.cols;

		for (int i = 0; i < this->rows; i++) {
			for (int j = 0; j < this->cols; j++) {
				this->data[i][j] = other.data[i][j];
			}
		}		
	}

public:
	/// <summary>
	/// Produces a Matrix. Can initialize with random values, as empty or as an identity matrix
	/// </summary>
	/// <param name="rows"></param>
	/// <param name="cols"></param>
	/// <param name="rand"></param>
	Matrix(int rows, int cols, bool rand = false, bool identity = true) {
		this->rows = rows;
		this->cols = cols;
		data = new double* [rows];
		for (int i = 0; i < rows; i++) {
			data[i] = new double[cols];
			if (rand) {
				std::random_device rd;  //Will be used to obtain a seed for the random number engine
				std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
				std::uniform_real_distribution<> distr(-1, 1);
				for (int j = 0; j < cols; j++)
					data[i][j] = distr(gen);

			} else {
				for (int j = 0; j < cols; j++) {
					if (identity && i == j)
						data[i][j] = 1;
					else
						data[i][j] = 0;
				}
			}
		}
	}

	Matrix(Matrix& other) {
		this->rows = other.rows;
		this->cols = other.cols;
		data = new double* [rows];
		for (int i = 0; i < rows; i++) {
			data[i] = new double[cols];
			for (int j = 0; j < cols; j++)
				data[i][j] = other.data[i][j];
		}
	}

	Matrix(Matrix&& other) {
		this->rows = other.rows;
		this->cols = other.cols;
		this->data = other.data;
		other.rows = 0;
		other.cols = 0;
		other.data = nullptr;
	}

	~Matrix() {
		clearData();
	}

	Matrix& operator=(Matrix&& other) {
		if (&other == this)
			return *this;

		clearData();
		this->rows = other.rows;
		this->cols = other.cols;
		this->data = other.data;

		other.rows = 0;
		other.cols = 0;
		other.data = nullptr;
	}

	Matrix& operator=(Matrix& other) {
		if (&other == this)
			return *this;

		clearData();
		copy(other);

		return *this;
	}

	Matrix operator*(const Matrix& other) {
		if (this->cols != other.rows) {
			std::printf("Matrix sizes are not matched, multiplication not possible.");
			return Matrix(0, 0);
		} else {
			Matrix result(this->rows, other.cols, false, false);
			
			auto mul = [&](int m) {
				for (int n = 0; n < other.cols; n++) {
					for (int k = 0; k < other.rows; k++) {
						result.data[m][n] += this->data[m][k] * other.data[k][n];
					}
				}
			};

			TBBMatrixBody<decltype(mul)> mulBody(mul);

			auto apply = [&](tbb::blocked_range<int> br) {
				mulBody(br);
			};

			tbb::blocked_range<int> range(0, this->rows);
			tbb::parallel_for(range, apply);

			return result;
		}
	}

	void print() {
		for (int i = 0; i < rows; i++) {
			std::printf("| ");
			for (int j = 0; j < cols; j++) {
				std::printf(" %f ", data[i][j]);
			}
			std::printf(" |\n");
		}
		std::printf("\n");
	}
};