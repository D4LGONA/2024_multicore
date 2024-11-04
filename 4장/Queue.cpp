#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

constexpr int MAX_THREADS = 16;

class DUMMYMUTEX 
{
public:
	void lock() {}
	void unlock() {}
};

class NODE 
{
public:
	int	key;
	NODE* volatile next;
	NODE(int x) : key(x), next(nullptr) {}
};  // 노드를 락킹할 필요는 x


class C_QUEUE {
	NODE* volatile head;
	NODE* volatile tail;
	std::mutex head_lock, tail_lock;
public:
	C_QUEUE()
	{
		head = tail = new NODE{ -1 };
	}
	void clear()
	{
		while (-1 != Dequeue());
	}
	void Enqueue(int x)
	{
		tail_lock.lock();
		NODE* e = new NODE(x);
		tail->next = e;
		tail = e;
		tail_lock.unlock();
	}
	int Dequeue()
	{
		head_lock.lock();
		if (head->next == nullptr) {
			head_lock.unlock();
			return -1;
		}
		int res = head->next->key;
		auto e = head;
		head = head->next;
		head_lock.unlock();
		delete e;
		return res;
	}
	void print20()
	{
		for (int i = 0; i < 20; ++i) {
			int v = Dequeue();
			if (v == -1) break; // 비었다는 뜻
			std::cout << v << ", ";
		}
		std::cout << std::endl;
	}
};

const int NUM_TEST = 10000000;
thread_local int thread_id;

C_QUEUE my_queue;

template <class T>
void benchmark(int num_thread, const int th_id, T& my_set)
{
	thread_id = th_id;
	int key = 0;
	for (int i = 0; i < NUM_TEST / num_thread; i++) {
		if ((i < 32) || (rand() % 2 == 0))
			my_queue.Enqueue(key++);
		else
			my_queue.Dequeue();
	}
}

int main()
{
	using namespace std::chrono;

	// 성능확인
	std::cout << "------ 성능 ------" << std::endl;
	for (int n = 1; n <= MAX_THREADS; n *= 2) {
		my_queue.clear();
		std::vector<std::thread> tv;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			tv.emplace_back(benchmark<C_QUEUE>, n, i, std::ref(my_queue));
		}
		for (auto& th : tv) th.join();
		auto end_t = high_resolution_clock::now();
		std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
		my_queue.print20();
	}
	my_queue.clear();
}

