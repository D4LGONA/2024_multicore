#include <iostream>
#include <thread>
// debug������ �� ���ư�����(100) release���� 0�� ���´�.


bool g_ready = false;
int g_data = 0;

void Recv()
{
	while (false == g_ready); // ���� ��ٸ��� �ʰ� �׳� �Ѿ����
	// while ������ ������������ ���� ����
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