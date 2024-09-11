#include <iostream>
#include <thread>
#include <mutex>
// debug������ �� ���ư�����(100) release���� 0�� ���´�.

std::mutex m;
bool g_ready = false;
int g_data = 0;

void Recv()
{
	while (true) {
		m.lock();
		if (true == g_ready) {
			m.unlock();
			break;
		}
		m.unlock();
	}
	m.lock();
	std::cout << "Data: " << g_data << std::endl;
	m.unlock();
}

void Send()
{
	m.lock();
	g_data = 100;
	g_ready = true;
	m.unlock();
}

int main()
{
	std::thread r{ Recv };
	std::thread s{ Send };

	r.join();
	s.join();
}