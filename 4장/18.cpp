#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
using namespace std;

volatile int sum;
volatile int LOCK = 0;
vector<thread> th;
int num_threads = 1;

bool CAS(volatile int* addr, int old_v, int new_v)
{
	return atomic_compare_exchange_strong(
		reinterpret_cast<volatile atomic_int*>(addr),
		&old_v, new_v);
}

void CAS_LOCK()
{
	while (true) {
		if (true == CAS(&LOCK, 0, 1)) break;
		this_thread::sleep_for(1ms);
	}
}

void CAS_UNLOCK()
{
	CAS(&LOCK, 1, 0); // ���⼭ cas�� �̿����� ������ 
					  // sum+2���� Lock=0�� ���� ����� �� �ִ�.
}

void worker(int num_threads)
{
	const int loop_count = 50000000 / num_threads;
	for (auto i = 0; i < loop_count; ++i) {
		CAS_LOCK();
		sum = sum + 2;
		CAS_UNLOCK();
	}
}

int main()
{
	using namespace chrono;

	for (int i = 0; i < 4; ++i) {

		auto start = high_resolution_clock::now();
		for (int j = 0; j < num_threads; ++j)
			th.emplace_back(worker, num_threads);
		
		for (auto& t : th)
			t.join();
		auto du = duration_cast<milliseconds>(high_resolution_clock::now() - start).count();

		cout << num_threads << "threads: " << sum << ", " << du << "ms" << endl;

		th.clear();
		sum = 0;
		num_threads *= 2;
	}
}