#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <random>
#include <cmath>
#include <ctime>
#include <random>
#include <cstdlib>
#include <chrono>
#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/pipeline.hpp>
#include "Matrix.h"

void example_display() {
	tf::Executor tfExec;
	tf::Taskflow taskflow;

	auto [A, B, C, D] = taskflow.emplace(
		[]() { std::cout << "TaskA\n"; },
		[]() { std::cout << "TaskB\n"; },
		[]() { std::cout << "TaskC\n"; },
		[]() { std::cout << "TaskD\n"; }
	);

	A.name("A");
	B.name("B");
	C.name("C");
	D.name("D");

	A.precede(B, C);
	D.precede(A);
	taskflow.dump(std::cout);

	tfExec.run(taskflow).wait();
	std::cout << '\n';
}


void example_for_each() {
	tf::Executor tfExec;
	tf::Taskflow taskflow;

	const int LEN = 10;
	int arr[LEN] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	auto doubleDisplay = [&](int i) { std::printf("%d times 2 is %d\n", arr[i], arr[i] * 2); };
	taskflow.for_each_index(0, LEN, 2, doubleDisplay); //Displays all odd numbers by multiplied 2.
	taskflow.for_each_index(LEN - 1, 0, -2, doubleDisplay); //Displays all even numbers multiplied by 2.

	tfExec.run(taskflow).wait();
	std::cout << '\n';
}


void example_1() {
	tf::Executor tfExec;
	tf::Taskflow taskflow;

	int val_1 = 2, val_2 = 1, val_3 = 5, x, y, a_res, b_res;

	std::cout << "Simple task, evaluate x = i*j + k*(k + i) and y = k*(k + i) * (i*j)\n";
	std::cout << "i: " << val_1 << " j: " << val_2 << " k: " << val_3 << std::endl;

	tf::Task C = taskflow.emplace([&]() { std::printf("X result is %d\n", a_res + b_res);  });
	tf::Task A = taskflow.emplace([&]() { a_res = val_2 * val_1; });
	tf::Task B = taskflow.emplace([&]() {
		b_res = val_3*(val_3 + val_1);
	});
	tf::Task D = taskflow.emplace([&]() { std::printf("Y result is %d\n", a_res * b_res); });

	A.precede(C);
	C.succeed(B);
	D.succeed(A);
	D.succeed(B);

	tfExec.run(taskflow).wait();
	std::cout << '\n';
}

//Modified example from taskflow
void pipe_example_old() {
	tf::Taskflow taskflow;
	tf::Executor executor;

	const size_t num_lines = 16;
	const size_t num_pipes = 3;

	// create a custom data buffer
	std::array<std::array<int, num_pipes>, num_lines> buffer;
	int output;

	std::cout << "Pipeline task\n";
	std::cout << "x1 -> x2 = x1 *2 -> x3 = x2 + 1 \n";
	std::cout << "y1 = x1 + 1 -> y2 = y1 *2 -> y3 = y2 + 1 + x3\n";
	std::cout << "Final result is in the third pipe of the last line.\n";

	// create a pipeline graph of 16 concurrent lines and three pipes
	tf::Pipeline pipeline(num_lines,
		// first pipe must define a serial direction
		tf::Pipe(tf::PipeType::SERIAL, [&buffer](tf::Pipeflow& pf) {

			if (pf.token() == num_lines) {
				pf.stop();
			} else {
				//Initialize the values in the first pipe, each line adds one to the previous lines pipe value
				if (pf.token() == 0) {
					buffer[pf.line()][pf.pipe()] = 1;
				} else {
					if (pf.line() > 0) {
						buffer[pf.line()][pf.pipe()] = buffer[pf.line() - 1][pf.pipe()] + 1;
					}
				}
			}

		}),
			tf::Pipe(tf::PipeType::PARALLEL, [&buffer](tf::Pipeflow& pf) {
					// propagate the previous result by multiplying by 2
					buffer[pf.line()][pf.pipe()] = 2 * buffer[pf.line()][pf.pipe() - 1];
			}),
			  tf::Pipe(tf::PipeType::SERIAL, [&buffer, &output](tf::Pipeflow& pf) {
					// propagate the previous result to this pipe by adding one plus, value from previous line
					buffer[pf.line()][pf.pipe()] = buffer[pf.line()][pf.pipe() - 1] + 1;
					if (pf.line() > 0) {
						buffer[pf.line()][pf.pipe()] += buffer[pf.line() - 1][pf.pipe()];
					}
					if (pf.line() + 1 == num_lines) {
						output = buffer[pf.line()][pf.pipe()];
					}
				})
		);

	// build the pipeline graph using composition
	tf::Task init = taskflow.emplace([]() { std::cout << "ready\n"; })
		.name("starting pipeline");
	tf::Task task = taskflow.composed_of(pipeline)
		.name("pipeline");
	tf::Task stop = taskflow.emplace([]() { std::cout << "stopped\n"; })
		.name("pipeline stopped");

	// create task dependency
	init.precede(task);
	task.precede(stop);

	// run the pipeline
	executor.run(taskflow).wait();

	for (int i = 0; i < num_lines; i++) {
		for (int& val : buffer[i]) {
			std::cout << val << " ";
		}
		std::cout << '\n';
	}
	
	std::cout << "Final Result: " << output << '\n';

	std::cout << '\n';
}

//Using TBB example
void pipe_example(int count = 1000) {
	tf::Taskflow taskflow;
	tf::Executor executor;

	const size_t num_lines = 16;
	const size_t num_pipes = 3;

	double* arr = new double[count];
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<> distr(1, RAND_MAX);
	int max = distr(gen);
	std::cout << "Random max of " << max << std::endl;

	for (int i = 0; i < count; i++) {
		arr[i] = (double)(distr(gen) % max);
	}

	int token_count = 0;
	double sum = 0;
	std::array<std::array<double, num_pipes - 1>, num_lines> buffer;

	std::chrono::steady_clock::time_point ts, te;
	ts = std::chrono::steady_clock::now();
	tf::Pipeline pipeline(num_lines,
		// first pipe must define a serial direction
		tf::Pipe(tf::PipeType::SERIAL, [&](tf::Pipeflow& pf) {

			if (token_count == count) {
				pf.stop();
			} else {
				token_count++;
			}

			}),
		tf::Pipe(tf::PipeType::PARALLEL, [&](tf::Pipeflow& pf) {
				buffer[pf.line()][pf.pipe()] = arr[pf.token()] * arr[pf.token()];
			}),
				tf::Pipe(tf::PipeType::SERIAL, [&](tf::Pipeflow& pf) {
				sum += buffer[pf.line()][pf.pipe() - 1];
					})
				);

	//Set the pipeline
	tf::Task task = taskflow.composed_of(pipeline)
		.name("pipeline");

	// run the pipeline
	executor.run(taskflow).wait();
	te = std::chrono::steady_clock::now();

	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(te - ts);

	std::cout << "RMS of random sequence is " << sqrt(sum / count) << "\n";
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
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(te-ts);
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
			<< "Graph visualize example: 5\n"
			<< "Exit: 0\n\n";
		choice = inputRange("Enter: ", 0, 5);
		switch (choice) {
		case 1:
			example_1();
			break;
		case 2:
			example_for_each();
			break;
		case 3:
			pipe_example(3200000);
			break;
		case 4:
			example_matrix(600, 4);
			break;
		case 5:
			example_display();
			break;
		default:
			break;
		}
	}
	return 0;
}