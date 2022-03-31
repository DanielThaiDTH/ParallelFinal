#include <iostream>
#include <taskflow\taskflow.hpp>

int main(int argc, char** argv) {
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
	return 0;
}