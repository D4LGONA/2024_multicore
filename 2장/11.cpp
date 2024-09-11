#include <iostream>
#include <thread>
// debug에서는 잘 돌아가지만(100) release에는 0이 나온다.


bool g_ready = false;
int g_data = 0;

void Recv()
{
	while (false == g_ready); // 여기 기다리지 않고 그냥 넘어가버림
	// while 루프를 컴파일하지도 않음 ㄷㄷ
	std::cout << "Data: " << g_data << std::endl;
}

void Send()
{
	g_data = 100;
	g_ready = true;
}

int main()
{
	std::thread r{ Recv };
	std::thread s{ Send };

	r.join();
	s.join();
}