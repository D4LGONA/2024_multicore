#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>

const int MAX_THREADS = 16;
volatile int sum[MAX_THREADS * 16]; // 64/4ÇÑ°Í
std::mutex my_lock;

void worker_nolock(const int th_id, const int num_loop)
{
	for (auto i = 0; i < num_loop; ++i) {
		sum[th_id * 16] = sum[th_id * 16] + 2;
	}
}

int main()
{
	using namespace std::chrono;
	for (int n = 1; n < 16; ++n) {
		for (auto& n : sum) n = 0;
		std::vector<std::thread> tv;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			int num_loop = 50000000 / n;
			if (i == (n - 1))
				num_loop += 50000000 % num_loop;
			tv.emplace_back(worker_nolock, i, num_loop);
		}
		for (auto& th : tv)
			th.join();
		int result = 0;
		for (auto n : sum) result = result + n;
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		size_t ms = duration_cast<milliseconds>(exec_t).count();
		std::cout << "Duration: " << ms << "msecs" << std::endl;
		std::cout << n << "Threads, Sum: " << result << std::endl;
	}
}