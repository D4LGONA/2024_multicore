#include <iostream>

int main()
{
	int sum = 0;
	for (auto i = 0; i < 50000000; ++i) sum = sum + 2;
	std::cout << "Sum: " << sum << std::endl;
}