#pragma once
#include <tbb/tbb.h>

template<typename T, typename A>
class ForEachBody {
	T* const data;
	A action;
	bool even;
public:
	ForEachBody(T* newData, A func, bool even = true) : data(newData), action(func), even(even) {}
	void operator()(const tbb::blocked_range<int>& r) {
		T* mut = data;
		for (auto i = r.begin(); i != r.end(); i++) {
			if ((even && i%2 == 0) || (!even && i%2 != 0))
				action(mut[i]);
		}
	}
};