#include <iostream>
#include <thread>
#include <mutex>

volatile bool g_ready = false;
volatile int g_data = 0;

void Recv()
{
	while (false == g_ready);
	std::cout << "Data: " << g_data << std::endl;
}

void Send()
{
	g_data = 100; // volatile�� ���������� g_data�� g_ready��
	g_ready = true; // ���� ���� ������ �ٲ����...
}

int main()
{
	std::thread r{ Recv };
	std::thread s{ Send };

	r.join();
	s.join();
}