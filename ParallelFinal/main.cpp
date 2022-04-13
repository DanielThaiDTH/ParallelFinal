#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/pipeline.hpp>

void example() {
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

	//tfExec.run(taskflow).wait();

	const int LEN = 10;
	int arr[LEN] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	auto doubleDisplay = [&](int i) { std::printf("%d times 2 is %d\n", i, i * 2); };
	taskflow.for_each_index(0, LEN, 2, doubleDisplay); //Displays all odd numbers by multiplied 2.
	taskflow.for_each_index(LEN - 1, 0, -2, doubleDisplay); //Displays all even numbers multiplied by 2.

	tfExec.run(taskflow).wait();

	std::cout << "Test\n" << std::endl;
	taskflow.dump(std::cout);
}


void example_1() {
	tf::Executor tfExec;
	tf::Taskflow taskflow;

	int val_1 = 2, val_2 = 1, val_3 = 5;

	std::cout << "Simple task, evaluate a*b + a*(a + c)\n";
	std::cout << "a: " << val_1 << " b: " << val_2 << " c: " << val_3 << std::endl;

	tf::Task C = taskflow.emplace([&]() { std::printf("Result is %d\n\n", val_2 + val_3);  });
	tf::Task A = taskflow.emplace([&]() { val_2 *= val_1; });
	tf::Task B = taskflow.emplace([&]() {
		int c = val_3; 
		val_3 += val_1;
		val_3 *= c;
	});

	A.name("A");
	B.name("B");
	C.name("C");

	A.precede(C);
	C.succeed(B);

	tfExec.run(taskflow).wait();
}

//Example from taskflow
void pipe_example() {
	tf::Taskflow taskflow;
	tf::Executor executor;

	const size_t num_lines = 4;
	const size_t num_pipes = 3;

	// create a custom data buffer
	std::array<std::array<int, num_pipes>, num_lines> buffer;

	// create a pipeline graph of four concurrent lines and three serial pipes
	tf::Pipeline pipeline(num_lines,
		// first pipe must define a serial direction
		tf::Pipe{ tf::PipeType::SERIAL, [&buffer](tf::Pipeflow& pf) {
			// generate only 5 scheduling tokens
			if (pf.token() == 5) {
			  pf.stop();
			}
			// save the token id into the buffer
			else {
			  buffer[pf.line()][pf.pipe()] = pf.token();
			}
		  } },
		tf::Pipe{ tf::PipeType::SERIAL, [&buffer](tf::Pipeflow& pf) {
			  // propagate the previous result to this pipe by adding one
			  buffer[pf.line()][pf.pipe()] = buffer[pf.line()][pf.pipe() - 1] + 1;
			} },
			  tf::Pipe{ tf::PipeType::SERIAL, [&buffer](tf::Pipeflow& pf) {
				// propagate the previous result to this pipe by adding one
				buffer[pf.line()][pf.pipe()] = buffer[pf.line()][pf.pipe() - 1] + 1;
			  } }
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

	for (std::array<int, num_pipes>& arr : buffer) {
		for (int& val : arr) {
			std::cout << val << " ";
		}
		std::cout << '\n';
	}

	std::cout << '\n';
}

int main(int argc, char** argv) {
	example_1();
	pipe_example();
	return 0;
}