#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

class NODE {
public:
	int	key;
	NODE* volatile next;
	std::mutex n_lock;
	NODE(int x) : key(x), next(nullptr) {}
	void lock() { n_lock.lock(); }
	void unlock() { n_lock.unlock(); }
};

class C_SET {
	NODE head{ (int)(0x80000000) }, tail{ (int)(0x7FFFFFFF) };
	std::mutex set_lock;
public:
	C_SET()
	{
		head.next = &tail;
	}
	void clear()
	{
		while (head.next != &tail) {
			auto p = head.next;
			head.next = head.next->next;
			delete p;
		}
	}
	bool Add(int x)
	{
		auto n_node = new NODE(x);
		auto prev = &head;
		set_lock.lock();
		auto curr = prev->next;
		while (curr->key < x) {
			prev = curr;
			curr = curr->next;
		}
		if (curr->key == x) {
			set_lock.unlock();
			delete n_node;
			return false;
		}
		else {
			n_node->next = curr;
			prev->next = n_node;
			set_lock.unlock();
			return true;
		}
	}
	bool Remove(int x)
	{
		auto prev = &head;
		set_lock.lock();
		auto curr = prev->next;
		while (curr->key < x) {
			prev = curr;
			curr = curr->next;
		}
		if (curr->key == x) {
			prev->next = curr->next;
			set_lock.unlock();
			delete curr;
			return true;
		}
		else {
			set_lock.unlock();
			return false;
		}
	}
	bool Contains(int x)
	{
		auto prev = &head;
		set_lock.lock();
		auto curr = prev->next;
		while (curr->key < x) {
			prev = curr;
			curr = curr->next;
		}
		if (curr->key == x) {
			set_lock.unlock();
			return true;
		}
		else {
			set_lock.unlock();
			return false;
		}
	}
	void print20()
	{
		auto p = head.next;
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			std::cout << p->key << ", ";
			p = p->next;
		}
		std::cout << std::endl;
	}
};

C_SET my_set{};

const int NUM_TEST = 4000000;
const int KEY_RANGE = 1000;

void benchmark(const int num_thread)
{
	int key;

	for (int i = 0; i < NUM_TEST / num_thread; i++) {
		switch (rand() % 3) {
		case 0: key = rand() % KEY_RANGE;
			my_set.Add(key);
			break;
		case 1: key = rand() % KEY_RANGE;
			my_set.Remove(key);
			break;
		case 2: key = rand() % KEY_RANGE;
			my_set.Contains(key);
			break;
		default: std::cout << "Error\n";
			exit(-1);
		}
	}
}


int main()
{
	using namespace std::chrono;

	for (int n = 1; n <= 16; n = n * 2) {
		my_set.clear();
		std::vector<std::thread> tv;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			tv.emplace_back(benchmark, n);
		}
		for (auto& th : tv)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		size_t ms = duration_cast<milliseconds>(exec_t).count();
		std::cout << n << " Threads,  " << ms << "ms.";
		my_set.print20();
	}
}