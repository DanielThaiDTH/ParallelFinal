# Notes

# Basic parallel algorithm support

Taskflow uses a a Taskflow class to provide and run parallel algorithms. The parallel algorithms parallel for, reduce, transform, and sort are provided in methods for_each/for_each_index(iterator and index based), reduce, transform and sort. The design of Taskflow is based on the STL of C++. All methods of the the Taskflow library will return a Task that be used for future scheduling.

In the examples below `taskflow` can be considered to be defined in the code as `tf::Taskflow taskflow;`, where `tf` is the namespace used by Taskflow. Taskflows also need to be run by an executor class named `Executor`. An `Executor` instance is created and named `tfExec` in the examples below. The `Executor` has several methods but the main one is the `run` method. It takes a `Taskflow` object, distributes tasks to threads depending on the dependency graph and runs them in parallel. It returns a  `Future` object that inherits from the standard `std::future` class and also provides a `cancel` method to cancel tasks.

## Parallel for

 The `for_each` and `for_each_index` method runs a task in parallel iterations. The `for_each` method uses an array index to partition out iterations to threads. The format of this function is as follows: 
 
	template <typename B, typename E, typename S, typename C>
	tf::Task for_each_index(B first, E last, S step, C callable)
 
 The `first` parameter is the starting index type, The `last` parameter is the ending index parameter, `S` is the type of the step size and lastly `C` should represent a callable type like a Functor or function that can take one parameter, the iteration index. Parameter types `B`, `E`, and `S` should be integral types. The `first` will be the starting index, `last` will be the ending index, `step` will be the step size and `callable` is the function to run each iteration. The method will distribute and run the iterations from `[first, last)`.
 
 The behaviour of this method changes depending on if the step value is positive or negative.
 
	// positive step
	for(auto i=first; i<last; i+=step) {
		callable(i);
	}

	// negative step
	for(auto i=first; i>last; i+=step) {
		callable(i);
	}
 
 **Example**
 
	const int LEN = 10;
	int arr[LEN] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	auto doubleDisplay = [&](int i) { std::printf("%d times 2 is %d", i, i * 2 ); }; 
	taskflow.for_each_index(0, LEN, 2, doubleDisplay ); //Displays all odd numbers by multiplied 2.
	taskflow.for_each_index(LEN - 1, 0, -2, doubleDisplay ); //Displays all even numbers multiplied by 2.
	
The for_each method is similar except it uses STL iterators rather than integral types as the first and last element types. This method will iterate over every iteration from `first` to `last`. The method will distribute and run the iterations from `[first, last)`.

	template <typename B, typename E, typename C>
	tf::Task for_each(B first, E last, C callable)
	
The `first` parameter is the starting index iterator type, The `last` parameter is the ending index iterator type, and lastly `C` should represent a callable type like a Functor or function that can take one parameter, a collection iterator. The parameter`first` will be the starting iterator, `last` will be the ending iterator,  and `callable` is the function to run each iteration. Iterators must have a ++ operator defined.

The serial equivalent would be:

	for(auto itr=first; itr!=last; itr++) {
		callable(*itr);
	}
	
**Example**

	std::vector<int> arr{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	
	taskflow.for_each(arr.begin(), arr.end(), [&](int i) { std::printf("Value of %d\n", i); });
	
## Parallel transform

A transform operation changes a collection of items and stores it in another collection. 

The parallel transform operation is provided by the `transform` method. The syntax is very similar to the STL algorithm library transform function, with the start and end iterators from the input collection and an output iterator from the output collection. The function signature is:4

	template<typename B, typename E, typename O, typename C>
	Task transform(B first1, E last1, O d_first, C c)
	
`first1` is the starting input iterator, `last1` is the ending input iterator, `d_first` is the output iterator of a collection to start inserting in. The parameter `c` is a callable type that takes a dereferenced iterator of type `B/E` and returns a value that can be assigned to a deferenced iterator of type `O`.

The serial equivalent of this algorithm would be: 
	
	while (first1 != last1) {
		*d_first++ = c(*first1++);
	}
	
**Example**

	std::vector<int> input{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
	std::vector<double> output;
	
	auto inv_sqrt = [](int val){ return 1/std::sqrt(val); };
	
	taskflow.transform(input.begin(), input.end(), std::back_inserter(output.begin()), inv_sqrt);
	
The Taskflow transform function also has an overload for a binary callable that operates on two collections. It takes two collections, combines them according to the callable and outputs to a single collection. The signature of this overload is:

	template<typename B, typename E, , typename B2, typename O, typename C>
	Task transform(B first1, E last1, B2 first2, O d_first, C c)
	
The serial equivalent of this would be: 

	while (first1 != last1) {
		*d_first++ = c(*first1++, *first2++);
	}
	
**Example**

	std::vector<int> src1 = {1, 2, 3, 4, 5};
	std::vector<int> src2 = {5, 4, 3, 2, 1};
	std::vector<int> tgt(src.size());
	taskflow.transform(
		src1.begin(), src1.end(), src2.begin(), tgt.begin(), 
		[](int i, int j){ 
			return i + j;
		}
	);
	tfExec.run(taskflow).wait();
	
## Parallel reduce

The reduce operation combines all elements of a collection into a final result.

Taskflow provides a parallel reduce algorithm in the `reduce` method. This method uses iterators to traverse a collection and takes a user defined combine operation. The result will be stored in a parameter that is passed by reference. The signature of the method is:

	template<typename B, typename E, typename T, typename O> 
	Task reduce(B first, E last, T& init, O bop)
	
`B` and `E` are iterators to a collection to reduce on. `T` is the type of the initial value to work with and `O` is a callable that takes two parameters, the first being of type `T` and the second being `B/E`. `first` is the starting iterator of the collection, `last` is end iterator of the collection. `init` is the initial value to reduce on and it is also stores the result because it is a reference. `bop` is the callable is the binary operation you want to reduce with. 

The serial equivalent would be: 

	for(auto itr=first; itr!=last; itr++) {
		init = bop(init, *itr);
	}
	
**Example**

	std::vector<int> vec{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	int init = 1;
	auto bop = [](int acc, int val){ return acc*val; };
	taskflow.reduce(vec.begin(), vec.end(), init, bop);
	executor.run(taskflow).wait();
	
## Parallel sort
	