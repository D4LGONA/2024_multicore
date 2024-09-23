// peterson //

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>

volatile int sum;
std::mutex my_lock;

void worker(const int num_loop)
{
	for (auto i = 0; i < num_loop; ++i) {
		my_lock.lock();
		sum = sum + 2;
		my_lock.unlock();
	}
}

void worker_nolock(const int num_loop)
{
	for (auto i = 0; i < num_loop; ++i) {
		sum += 2;
	}
}

volatile bool flags[2] = { false, false };
volatile int victim;// 데드락 방지용

void p_lock(int th_id)
{
	int other = 1 - th_id;
	flags[th_id] = true;
	victim = th_id; // 양보하기.
	while (flags[other] == true and victim == th_id);
}

void p_unlock(int th_id)
{
	flags[th_id] = false;
}

void worker_p(const int th_id, const int num_loop)
{
	for (auto i = 0; i < num_loop; ++i) {
		p_lock(th_id);
		sum = sum + 2;
		p_unlock(th_id);
	}
	
}

int main()
{
	using namespace std::chrono;
	// lock 사용 버전
	for (int n = 1; n <= 2; ++n) {
		sum = 0;
		std::vector<std::thread> tv;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			int num_loop = 50000000 / n;
			if (i == (n - 1))
				num_loop += 50000000 % num_loop;
			tv.emplace_back(worker, num_loop);
		}
		for (auto& th : tv)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		size_t ms = duration_cast<milliseconds>(exec_t).count();
		std::cout << n << "Threads, Mutex Sum: " << sum << ", " << ms << "ms" << std::endl;
	}

	// 피터슨 버전
	for (int n = 1; n <= 2; ++n) {
		sum = 0;
		std::vector<std::thread> tv;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			int num_loop = 50000000 / n;
			if (i == (n - 1))
				num_loop += 50000000 % num_loop;
			tv.emplace_back(worker_p, i, num_loop); // 스레드 아이디 추가
		}
		for (auto& th : tv)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		size_t ms = duration_cast<milliseconds>(exec_t).count();
		std::cout << n << "Threads, Peterson Sum: " << sum << ", " << ms << "ms" << std::endl;
	}
}