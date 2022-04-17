#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <random>
#include <cmath>
#include <ctime>
#include <random>
#include <tuple>
#include <chrono>
#include <tbb/tbb.h>
#include <tbb/parallel_reduce.h>
#include <tbb/parallel_pipeline.h>
#include <tbb/flow_graph.h>
#include "bodies.h"
#include "Matrix.h"

using namespace tbb::flow;

void example_1() {
	int val_1 = 2, val_2 = 1, val_3 = 5;

	std::cout << "Simple task, evaluate x = i*j + k*(k + i) and y = k*(k + i) * (i*j)\n";
	std::cout << "i: " << val_1 << " j: " << val_2 << " k: " << val_3 << std::endl;

	tbb::flow::graph g;
	std::pair<int, int> a_input{ val_1, val_2 };
	std::pair<int, int> b_input{ val_1, val_3 };
	std::pair<int, int> c_input{ val_2, val_3 };
	function_node<std::pair<int, int>, int> a(g, 1, [](std::pair<int, int> v) { 
		return v.second * v.first; });
	function_node<std::pair<int, int>, int> b(g, 1, [](std::pair<int, int> v) { 
		return v.second * (v.second + v.first); });
	join_node<std::tuple<int, int>, queueing> c_join(g);
	function_node<std::tuple<int, int>> c(g, 1, [](std::tuple<int, int> v) {
		std::printf("X result is %d\n", std::get<0>(v) + std::get<1>(v));
		});
	function_node<std::tuple<int, int>> d(g, 1, [](std::tuple<int, int> v) {
		std::printf("Y result is %d\n", std::get<0>(v) * std::get<1>(v));
		});

	make_edge(a, std::get<0>(c_join.input_ports())); //(pre, suc)
	make_edge(b, std::get<1>(c_join.input_ports()));
	make_edge(c_join, c);
	make_edge(c_join, d);
	a.try_put(a_input);
	b.try_put(b_input);
	g.wait_for_all();
	std::printf("\n");
}

void example_for_each() {
	const int LEN = 10;
	int arr[LEN] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	auto doubleDisplay = [&](int i) { std::printf("%d times 2 is %d\n", i, i * 2); };

	ForEachBody<int, decltype(doubleDisplay)> bodyEven(arr, doubleDisplay);
	ForEachBody<int, decltype(doubleDisplay)> bodyOdd(arr, doubleDisplay, false);

	auto applyEven = [&](tbb::blocked_range<int> br) {
		bodyEven(br);
	};
	auto applyOdd = [&](tbb::blocked_range<int> br) {
		bodyOdd(br);
	};

	tbb::blocked_range<int> range(0, LEN);
	tbb::parallel_for(range, applyEven);
	tbb::parallel_for(range, applyOdd);
}

void pipe_example(int count = 1000) {
	double* arr = new double[count];
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<> distr(1, RAND_MAX);
	int max = distr(gen);
	std::cout << "Random max of " << max << std::endl;

	for (int i = 0; i < count; i++) {
		arr[i] = (double)(distr(gen) % max);
	}

	double* first = arr;
	double *last = arr + count;

	//Below from TBB docs
	double sum = 0;

	std::chrono::steady_clock::time_point ts, te;
	ts = std::chrono::steady_clock::now();
	tbb::parallel_pipeline( /*max_number_of_live_token=*/16,
		tbb::make_filter<void, double*>(
			tbb::filter_mode::serial_in_order,
			[&](tbb::flow_control& fc)-> double* {
				if (first < last) {
					return first++;
				} else {
					fc.stop();
					return nullptr;
				}
			}
			) &
		tbb::make_filter<double*, double>(
			tbb::filter_mode::parallel,
			[&count](double* p) {return ((*p) * (*p))/count; }
			) &
				tbb::make_filter<double, void>(
					tbb::filter_mode::serial_out_of_order,
					[&](double x) {sum += x; }
					)
				);
	te = std::chrono::steady_clock::now();

	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(te - ts);

	std::cout << "RMS of random sequence is " << sqrt(sum) << "\n";
	std::printf("RMS calculation took %dms\n", (int)ms.count());

	sum = 0;
	ts = std::chrono::steady_clock::now();
	for (int i = 0; i < count; i++) {
		sum += arr[i] * arr[i];
	}
	te = std::chrono::steady_clock::now();
	ms = std::chrono::duration_cast<std::chrono::milliseconds>(te - ts);
	std::printf("Serial RMS calculation took %dms\n", (int)ms.count());
	std::cout << "Validation " << sqrt(sum / count) << "\n\n";
	delete[] arr;
}


void example_matrix(int size, int iterations) {
	std::vector<Matrix> matrices;

	std::printf("Generating random (%dx%d) matrices\n", size, size);
	for (int n = 0; n < iterations; n++) {
		matrices.push_back(Matrix(size, size, true));
	}

	Matrix result = matrices[0];
	std::chrono::steady_clock::time_point ts, te;

	std::printf("Starting %d matrix multiplications\n", iterations);
	ts = std::chrono::steady_clock::now();
	for (int n = 1; n < iterations; n++) {
		result = result * matrices[n];
	}
	te = std::chrono::steady_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(te - ts);
	std::printf("%d multiplications of (%dx%d) matrices took %dms\n", iterations, size, size, (int)ms.count());
}


int inputRange(std::string prompt, int min, int max)
{
	if (min > max) {
		std::cout << "The minimum value pass to the inputRange function was \
						larger than the maximum value.\n";
		return INT_MIN;
	}

	int ans = INT_MIN;

	while (true) {
		std::cout << prompt;
		if (!(std::cin >> ans)) {
			std::cin.clear();
		}
		std::cin.ignore(1000, '\n');

		if (ans >= min && ans <= max)
			break;

	}

	return ans;
}


int main(int argc, char** argv) {
	int choice = -1;
	while (choice != 0) {
		std::cout << "Please select example to run.\n";
		std::cout << "*****************************\n";
		std::cout << "Graph example: 1\n"
			<< "For each example: 2\n"
			<< "Pipe example: 3\n"
			<< "Matrix example: 4\n"
			<< "Exit: 0\n\n";
		choice = inputRange("Enter: ", 0, 4);
		switch (choice) {
		case 1: 
			example_1();
			break;
		case 2:
			example_for_each();
			break;
		case 3:
			pipe_example(100000);
			break;
		case 4:
			example_matrix(600, 4);
			break;
		default:
			break;
		}
	}
	return 0;
}