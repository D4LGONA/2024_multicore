#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>

volatile int sum; 
std::mutex my_lock;

void worker_nolock(const int num_loop)
{
	for (auto i = 0; i < num_loop; ++i) {
		sum += 2;
	}
}

void worker_opt(const int num_loop)
{
	volatile int local_sum = 0;
	for (auto i = 0; i < num_loop; ++i) {
		local_sum = local_sum + 2;
	}
	my_lock.lock();
	sum = sum + local_sum;
	my_lock.unlock();
}

void worker(const int num_loop)
{
	for (auto i = 0; i < num_loop; ++i) {
		my_lock.lock();
		sum = sum + 2;
		my_lock.unlock();
	}
}

int main()
{
	using namespace std::chrono;
	// �ʹ� �����ɷ��� 0 �Ѱ� ����
	for (int n = 1; n < 16; ++n) {
		sum = 0;
		std::vector<std::thread> tv;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			int num_loop = 50000000 / n;
			if (i == (n - 1))
				num_loop += 50000000 % num_loop;
			tv.emplace_back(worker_opt, num_loop);
		}
		for (auto& th : tv)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		size_t ms = duration_cast<milliseconds>(exec_t).count();
		std::cout << "Duration: " << ms << "msecs" << std::endl;
		std::cout << n << "Threads, Sum: " << sum << std::endl;
	}
}