#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <random>
#include <cmath>
#include <ctime>
#include <random>
#include <tuple>
#include <tbb/tbb.h>
#include <tbb/parallel_reduce.h>
#include <tbb/parallel_pipeline.h>
#include <tbb/flow_graph.h>
#include "bodies.h"

using namespace tbb::flow;

void example_1() {
	int val_1 = 2, val_2 = 1, val_3 = 5, res = 0;

	std::cout << "simple task, evaluate a*b + a*(a + c)\n";
	std::cout << "a: " << val_1 << " b: " << val_2 << " c: " << val_3 << std::endl;

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
		std::printf("result is %d\n\n", std::get<0>(v) + std::get<1>(v));
		});

	make_edge(a, std::get<0>(c_join.input_ports())); //(pre, suc)
	make_edge(b, std::get<1>(c_join.input_ports()));
	make_edge(c_join, c);
	a.try_put(a_input);
	b.try_put(b_input);
	g.wait_for_all();
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

void pipe_example() {
	float arr[1000];
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<> distr(1, RAND_MAX);
	int max = distr(gen);
	std::cout << "Random max of " << max << std::endl;

	for (int i = 0; i < 1000; i++) {
		arr[i] = (float)(i + distr(gen) % max);
	}

	float* first = arr;
	float* last = arr + 1000;

	//Below from TBB docs
	float sum = 0;
	tbb::parallel_pipeline( /*max_number_of_live_token=*/16,
		tbb::make_filter<void, float*>(
			tbb::filter_mode::serial_in_order,
			[&](tbb::flow_control& fc)-> float* {
				if (first < last) {
					return first++;
				} else {
					fc.stop();
					return nullptr;
				}
			}
			) &
		tbb::make_filter<float*, float>(
			tbb::filter_mode::parallel,
			[](float* p) {return (*p) * (*p); }
			) &
				tbb::make_filter<float, void>(
					tbb::filter_mode::serial_out_of_order,
					[&](float x) {sum += x; }
					)
				);

	std::cout << "RMS of random sequence is " << sqrt(sum/1000) << "\n\n";
}

int main(int argc, char** argv) {
	/*example_display();*/
	example_1();
	example_for_each();
	pipe_example();
	return 0;
}