#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

constexpr int SIZE = 50000000;
volatile int* bounce;
int num_error = 0;
volatile bool done = false;

void worker_0()
{
	for (int i = 0; i < SIZE; ++i) {
		*bounce = -(1 + *bounce);
	}
	done = true;
}

void worker_1()
{
	while(false == done) {
		int v = *bounce;
		if (v != 0 and v != -1)
			num_error++;
	}
}

int main()
{
	//bounce = new int{ 0 };
	// 주소가 깔끔하게 들어가지 않았을 경우 문제가 생김
	int arr[64];
	int* p = &(arr[32]);

	long long addr = reinterpret_cast<long long>(p);
	addr = (addr / 64) * 64; 
	addr = addr - 2; // 얘를 추가했을떄 에러가 많이 생김.
	// 왜? 

	bounce = reinterpret_cast<int*>(addr);
	*bounce = 0;

	std::thread r{ worker_0 };
	std::thread s{ worker_1 };

	r.join();
	s.join();

	std::cout << "error: " << num_error << std::endl;
}