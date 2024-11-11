#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

constexpr int MAX_THREADS = 16;
class STNODE;

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

class C_STACK {
	NODE* volatile m_top;
	std::mutex st_lock;
public:
	C_STACK()
	{
		m_top = nullptr;
	}
	void clear()
	{
		while (-2 != Pop());
	}
	void Push(int x)
	{
		NODE* e = new NODE(x);
		st_lock.lock();
		e->next = m_top;
		m_top = e;
		st_lock.unlock();
	}
	int Pop()
	{
		st_lock.lock();
		if (nullptr == m_top) {
			st_lock.unlock();
			return -2;
		}
		int res = m_top->key;
		auto e = m_top;
		m_top = m_top->next;
		st_lock.unlock();
		delete e;
		return res;
	}
	void print20()
	{
		for (int i = 0; i < 20; ++i) {
			int v = Pop();
			if (v == -2) break; // 비었다는 뜻
			std::cout << v << ", ";
		}
		std::cout << std::endl;
	}
};

class LF_STACK {
	NODE* volatile m_top;
public:
	LF_STACK()
	{
		m_top = nullptr;
	}
	bool CAS(NODE* old_ptr, NODE* new_ptr)
	{
		return std::atomic_compare_exchange_strong(reinterpret_cast<volatile std::atomic_llong*>(&m_top),
			reinterpret_cast<long long*> (&old_ptr), reinterpret_cast<long long>(new_ptr));
	}
	void clear()
	{
		while (-2 != Pop());
	}
	void Push(int x)
	{
		NODE* e = new NODE(x);
		e->next = m_top;
		CAS(e->next, e);
		m_top = e;
	}
	int Pop()
	{
		if (nullptr == m_top) {
			return -2;
		}
		int res = m_top->key;
		auto e = m_top;
		CAS(e, e->next);
		//delete e;
		return res;
	}
	void print20()
	{
		for (int i = 0; i < 20; ++i) {
			int v = Pop();
			if (v == -2) break; // 비었다는 뜻
			std::cout << v << ", ";
		}
		std::cout << std::endl;
	}
};

const int NUM_TEST = 10000000;
thread_local int thread_id;

LF_STACK my_stack;

template <class T>
void benchmark(int num_thread, const int th_id, T& my_set)
{
	thread_id = th_id;
	int key = 0;
	for (int i = 0; i < NUM_TEST / num_thread; i++) {
		if ((i < 1000) || (rand() % 2 == 0)) // 초반엔 push만 나오도록 하는 것
			my_stack.Push(key++);
		else
			my_stack.Pop();
	}
}

int main()
{
	using namespace std::chrono;

	// 성능확인
	std::cout << "------ 성능 ------" << std::endl;
	for (int n = 1; n <= MAX_THREADS; n *= 2) {
		my_stack.clear();
		std::vector<std::thread> tv;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			tv.emplace_back(benchmark<LF_STACK>, n, i, std::ref(my_stack));
		}
		for (auto& th : tv) th.join();
		auto end_t = high_resolution_clock::now();
		std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
		my_stack.print20();
	}
	my_stack.clear();
}

