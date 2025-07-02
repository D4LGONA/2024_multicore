#include <omp.h>
#include <iostream>
#include <thread>

int main()
{
//#pragma omp parallel
//	{
//		int tid = omp_get_thread_num();
//		std::cout << "I am Thread, ThreadID: " << tid << std::endl;
//		if (tid == 1) {
//			int num_of_threads = omp_get_num_threads();
//			std::cout << "num of Threads: " << num_of_threads << std::endl;
//		}
//	}


//	volatile int sum = 0;
//#pragma omp parallel
//	{
//		for (int i = 0; i < 50000000/omp_get_num_threads(); ++i)
//#pragma omp critical // locking
//			sum = sum + 2;
//	}
//	std::cout << "SUM: " << sum << std::endl;

//	volatile int sum = 0;
//#pragma omp parallel
//	{
//#pragma omp for
//		for (int i = 0; i < 50000000; ++i)
//#pragma omp critical // locking
//			sum = sum + 2;
//	}
//	std::cout << "SUM: " << sum << std::endl;
//}

	volatile int sum = 0;
#pragma omp parallel
	{
#pragma omp for
		for (int i = 0; i < 50000000; ++i)
#pragma omp atomic // locking
			sum += 2;
	}
	std::cout << "SUM: " << sum << std::endl;
}
