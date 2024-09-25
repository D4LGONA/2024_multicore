#include <iostream>
#include <thread>
#include <mutex>

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
	// ÁÖ¼Ò°¡ ±ò²ûÇÏ°Ô µé¾î°¡Áö ¤·³º¾ÒÀ» °Ü¿ì ¹®Á¦°¡ »ý±è
	int array[128]
	std::thread r{ worker_0 };
	std::thread s{ worker_1 };

	r.join();
	s.join();

	std::cout << "error: " << num_error << std::endl;
}