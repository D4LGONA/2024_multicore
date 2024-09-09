#include <iostream>
#include <thread>
int sum;

void worker()
{
	for (auto i = 0; i < 2500000; ++i) {
		sum = sum + 2;
		sum = sum + 2; 
		sum = sum + 2;
		sum = sum + 2;
		sum = sum + 2;
		sum = sum + 2;
		sum = sum + 2;
		sum = sum + 2;
		sum = sum + 2;
		sum = sum + 2;
	}
}

int main()
{
	char ch;
	std::cout << "Enter number: ";
	std::cin >> ch;
	std::thread t1{ worker };
	std::thread t2{ worker };

	t1.join();
	t2.join();

	std::cout << "Sum: " << sum << std::endl;
}