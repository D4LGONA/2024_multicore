#include <iostream>
#include <chrono>
#include <thread>
#include <tbb/task_group.h>

long long fibo(int n)
{
	if (n == 0) return 0;
	else if (n == 1) return 1;
	else return (fibo(n - 1) + fibo(n - 2));
}

long long tbb_fibo(int n)
{
	if (n < 20) return fibo(n);
	tbb::task_group tbb_task;
	long long res1;
	long long res2;
	tbb_task.run_and_wait([&res1, n]() {res1 = tbb_fibo(n - 1); });
	tbb_task.run_and_wait([&res2, n]() {res2 = tbb_fibo(n - 2); });

	return res2 + res1;
}

int main()
{
	using namespace std::chrono;
	{
		auto start_t = high_resolution_clock::now();
		long long res = fibo(40);
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		size_t ms = duration_cast<milliseconds>(exec_t).count();

		std::cout << "Result: " << res << ", " << ms << "ms." << std::endl;
	}

	{
		auto start_t = high_resolution_clock::now();
		long long res = tbb_fibo(40);
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		size_t ms = duration_cast<milliseconds>(exec_t).count();

		std::cout << "Result: " << res << ", " << ms << "ms.";
	}
	
}