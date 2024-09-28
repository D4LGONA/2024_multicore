#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <chrono>
using namespace std;

volatile bool* flag;
volatile int* label;
volatile int sum;
vector<thread> t;
mutex l;

int thread_num = 1;

void lock(int th_id)
{
	flag[th_id] = true;
	label[th_id] = *max_element(label, label + thread_num) + 1;
	//??
	for (int k = 0; k < thread_num; ++k)
		while (k != th_id && flag[k] &&
			(label[k] < label[th_id] ||
				(label[k] == label[th_id] && k < th_id)));
}

void unlock(int th_id)
{
	flag[th_id] = false;
}

void worker_bakery(int th_id)
{
	for (int i = 0; i < 10000000 / thread_num; ++i) {
		lock(th_id);
		sum++;
		unlock(th_id);
	}
}

void worker_nolock()
{
	for (int i = 0; i < 10000000 / thread_num; ++i) {
		sum++;
	}
}

void worker_mutex()
{
	for (int i = 0; i < 10000000 / thread_num; ++i) {
		l.lock();
		sum++;
		l.unlock();
	}
}

int main()
{
	using namespace chrono;

	
	cout << "----bakery----" << endl;
	for (int i = 0; i < 4; ++i) {
		sum = 0;
		flag = new bool[thread_num];
		label = new int[thread_num];
		fill(flag, flag + thread_num, false);
		fill(label, label + thread_num, 0);

		auto start = high_resolution_clock::now();
		for (int j = 0; j < thread_num; ++j) {
			t.emplace_back(worker_bakery, j );
		}

		for (auto& th : t)
			th.join();

		auto end = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(end - start).count();

		cout << thread_num << "threads" << ": " << sum << ", " << duration << "ms" << endl;

		thread_num *= 2;
		t.clear();
		delete[] flag;
		delete[] label;
	}

	thread_num = 1;
	cout << endl << "----mutex----" << endl;
	for (int i = 0; i < 4; ++i) {
		sum = 0;
		auto start = high_resolution_clock::now();
		for (int j = 0; j < thread_num; ++j) {
			t.emplace_back(worker_mutex);
		}

		for (auto& th : t)
			th.join();

		auto end = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(end - start).count();

		cout << thread_num << "threads" << ": " << sum << ", " << duration << "ms" << endl;

		thread_num *= 2;
		t.clear();
	}

	thread_num = 1;
	cout << endl << "----no lock----" << endl;
	for (int i = 0; i < 4; ++i) {
		sum = 0;
		auto start = high_resolution_clock::now();
		for (int j = 0; j < thread_num; ++j) {
			t.emplace_back(worker_nolock);
		}

		for (auto& th : t)
			th.join();

		auto end = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(end - start).count();

		cout << thread_num << "threads" << ": " << sum << ", " << duration << "ms" << endl;

		thread_num *= 2;
		t.clear();
	}
}