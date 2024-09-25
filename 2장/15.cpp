#include <iostream>
#include <thread>
#include <mutex>

constexpr int SIZE = 50000000;
std::atomic_int x, y; // 원래 volatile int였음!
int trace_x[SIZE], trace_y[SIZE]; // 얘는 DataRace가 아니다(스레드 따로)

std::atomic_bool g_ready = false;
std::atomic_int g_data = 0;

void worker_0()
{
	for (int i = 0; i < SIZE; ++i) {
		x++;
		//std::atomic_thread_fence(std::memory_order_seq_cst);
		trace_y[i] = y;
	}
}

void worker_1()
{
	for (int i = 0; i < SIZE; ++i) {
		y++;
		//std::atomic_thread_fence(std::memory_order_seq_cst);
		trace_x[i] = x;
	}
}

int main()
{
	std::thread r{ worker_0 };
	std::thread s{ worker_1 };

	r.join();
	s.join();

	int error = 0;
	for (int i = 0; i < (SIZE - 1); ++i) {
		if (trace_x[i] == trace_x[i + 1]) {
			int x_val = trace_x[i];
			if (trace_y[x_val] == trace_y[x_val + 1])
				if (trace_y[x_val] == i)
					error++;

		}
	}
	std::cout << "error: " << error << std::endl;
}