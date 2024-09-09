#include <iostream>
#include <thread>
#include <mutex>
int sum;

std::mutex my_lock;
void worker()
{
	for (auto i = 0; i < 2500000; ++i) {
		my_lock.lock();
		sum = sum + 2;
		my_lock.unlock();
	}
}

int main()
{
	std::thread t1{ worker };
	std::thread t2{ worker };

	t1.join();
	t2.join();

	std::cout << "Sum: " << sum << std::endl;
}