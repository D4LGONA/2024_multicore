#include <iostream>
#include <thread>
#include <mutex>

std::atomic_bool g_ready = false;
std::atomic_int g_data = 0;

void Recv()
{
	while (false == g_ready);
	std::cout << "Data: " << g_data << std::endl;
}

void Send()
{
	g_data = 100; // volatile을 쓰지않으면 g_data와 g_ready에
	g_ready = true; // 값이 들어가는 순서가 바뀔수도...
}

int main()
{
	std::thread r{ Recv };
	std::thread s{ Send };

	r.join();
	s.join();
}