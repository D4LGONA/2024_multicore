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

std::atomic_bool stop = false;

class LF_QUEUE {
	NODE* volatile head;
	NODE* volatile tail;
	std::mutex head_lock;
public:
	LF_QUEUE()
	{
		head = tail = new NODE{ -1 };
	}
	void clear()
	{
		while (-1 != Dequeue());
	}
	bool CAS(NODE* volatile* ptr, NODE* old_ptr, NODE* new_ptr)
	{
		return std::atomic_compare_exchange_strong(reinterpret_cast<volatile std::atomic_llong*>(ptr),
			reinterpret_cast<long long*> (&old_ptr), reinterpret_cast<long long>(new_ptr));
	}
	void Enqueue(int x)
	{
		NODE* e = new NODE(x);
		while (true) {
			NODE* last = tail;
			NODE* next = last->next;
			if (last != tail) continue;
			if (nullptr == next) {
				if (true == CAS(&last->next, nullptr, e)) {
					CAS(&tail, last, e);
					break;
				}
			}
			else CAS(&tail, last, next);
		}
	}
	int Dequeue()
	{
		while (true) {
			NODE* first = head;
			NODE* next = first->next;
			NODE* last = tail;
			if (first != head) continue;
			if (nullptr == next) return -1;
			if (first == last) {
				CAS(&tail, last, next);
				continue;
			}
			int value = next->key;
			if (false == CAS(&head, first, next)) continue;
			//delete first;
			return value;
		}
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
std::atomic_int loop_count = NUM_TEST;

LF_QUEUE my_queue;

template <class T>
void benchmark(int num_thread, const int th_id, T& my_set)
{
	thread_id = th_id;
	int key = 0;
	/*while (loop_count-- > 0) {
		if ((rand() % 2 == 0))
			my_queue.Enqueue(key++);
		else
			my_queue.Dequeue();
	}*/
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
			tv.emplace_back(benchmark<LF_QUEUE>, n, i, std::ref(my_queue));
		}
		for (auto& th : tv) th.join();
		auto end_t = high_resolution_clock::now();
		std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
		my_queue.print20();
	}
	my_queue.clear();
}

