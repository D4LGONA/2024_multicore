#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <array>

class NODE {
public:
	int	key;
	NODE* volatile next;
	std::mutex  n_lock;
	bool removed;
	NODE(int x) : key(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};

class DUMMYMUTEX
{
public:
	void lock() {}
	void unlock() {}
};

// 성긴 동기화
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

// 세밀한 동기화
class F_SET {
	NODE head{ (int)(0x80000000) }, tail{ (int)(0x7FFFFFFF) };
public:
	F_SET()
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
		prev->lock();
		auto curr = prev->next;
		curr->lock();
		while (curr->key < x) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (curr->key == x) {
			prev->unlock(); curr->unlock();
			delete n_node;
			return false;
		}
		else {
			n_node->next = curr;
			prev->next = n_node;
			prev->unlock(); curr->unlock();
			return true;
		}
	}
	bool Remove(int x)
	{
		auto prev = &head;
		prev->lock();
		auto curr = prev->next;
		curr->lock();
		while (curr->key < x) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (curr->key == x) {
			prev->next = curr->next;
			prev->unlock(); curr->unlock();
			delete curr;
			return true;
		}
		else {
			prev->unlock(); curr->unlock();
			return false;
		}
	}
	bool Contains(int x)
	{
		auto prev = &head;
		prev->lock();
		auto curr = prev->next;
		curr->lock();
		while (curr->key < x) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (curr->key == x) {
			prev->unlock(); curr->unlock();
			return true;
		}
		else {
			prev->unlock(); curr->unlock();
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

// 낙천적 동기화
class O_SET {
	NODE head{ (int)(0x80000000) }, tail{ (int)(0x7FFFFFFF) };
public:
	O_SET()
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

	bool validate(const int x, NODE* prev, NODE* curr) // 교재에 있는거랑 같음
	{
		auto p = &head;
		auto c = p->next;
		while (c->key < x) {
			p = c;
			c = c->next;
		}
		return (prev == p) and (curr == c);
	}

	bool Add(int x)
	{
		auto n_node = new NODE(x);
		while (true) {

			// 검색
			auto prev = &head;
			auto curr = prev->next;
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}

			// locking
			prev->lock(); curr->lock();

			// validate 검사
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == x) {
				prev->unlock(); curr->unlock();
				delete n_node;
				return false;
			}

			else {
				n_node->next = curr;
				prev->next = n_node;
				prev->unlock(); curr->unlock();
				return true;
			}
		}
	}

	bool Remove(int x)
	{
		while (true) {
			// 검색
			auto prev = &head;
			auto curr = prev->next;
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}

			// lock
			prev->lock(); curr->lock();

			// 검사
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == x) {
				prev->next = curr->next;
				prev->unlock(); curr->unlock();
				return true;
			}
			else {
				prev->unlock(); curr->unlock();
				return false;
			}
		}
	}
	bool Contains(int x)
	{
		while (true) {
			auto prev = &head;
			auto curr = prev->next;
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}

			prev->lock(); curr->lock();
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == x) {
				prev->unlock(); curr->unlock();
				return true;
			}
			else {
				prev->unlock(); curr->unlock();
				return false;
			}
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

// 게으른 동기화
class L_SET {
	NODE head{ (int)(0x80000000) }, tail{ (int)(0x7FFFFFFF) };
public:
	L_SET()
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
	bool validate(const int x, NODE* prev, NODE* curr)
	{
		return false == prev->removed and false == curr->removed and curr == prev->next;
	}

	bool Add(int x)
	{
		//auto p = new NODE{ x };
		while (true) {
			// 검색 Phase
			auto prev = &head;
			auto curr = prev->next;
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}
			// Locking
			prev->lock(); curr->lock();
			// Valid 검사
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->key == x) {
				prev->unlock(); curr->unlock();
				//delete p;
				return false;
			}
			else {
				auto p = new NODE{ x };
				p->next = curr;
				prev->next = p;
				prev->unlock(); curr->unlock();
				return true;
			}
		}
	}
	bool Remove(int x)
	{
		while (true) {
			// 검색
			auto prev = &head;
			auto curr = prev->next;
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}

			// lock
			prev->lock(); curr->lock();

			// 검사
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == x) {
				curr->removed = true; // 삭제 
				// std::atomic_thread_fence(std::memory_order_seq_cst); // 맥 등에서는 이게 없으면
				// 순서가 뒤바뀌어 버릴지도...
				prev->next = curr->next;
				prev->unlock(); curr->unlock();
				return true;
			}
			else {
				prev->unlock(); curr->unlock();
				return false;
			}
		}
	}
	bool Contains(int x)
	{
		// 요기 수정됨
		auto curr = head.next;
		while (curr->key < x) {
			curr = curr->next;
		}
		return false == curr->removed and curr->key == x;
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

C_SET my_set1;
F_SET my_set2;
O_SET my_set3;
L_SET my_set4;

const int NUM_TEST = 4000000;
const int KEY_RANGE = 1000;

class HISTORY {
public:
	int op;
	int i_value;
	bool o_value;
	HISTORY(int o, int i, bool re) : op(o), i_value(i), o_value(re) {}
};

std::array<std::vector<HISTORY>, 16> history;

template<class T>
void worker_check(int num_threads, int thread_id, T& my_set)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		int op = rand() % 3;
		switch (op) {
		case 0: {
			int v = rand() % KEY_RANGE;
			history[thread_id].emplace_back(0, v, my_set.Add(v));
			break;
		}
		case 1: {
			int v = rand() % KEY_RANGE;
			history[thread_id].emplace_back(1, v, my_set.Remove(v));
			break;
		}
		case 2: {
			int v = rand() % KEY_RANGE;
			history[thread_id].emplace_back(2, v, my_set.Contains(v));
			break;
		}
		}
	}
}

template<class T>
void check_history(int num_threads, T& my_set)
{
	std::array <int, KEY_RANGE> survive = {};
	std::cout << "Checking Consistency : ";
	if (history[0].size() == 0) {
		std::cout << "No history.\n";
		return;
	}
	for (int i = 0; i < num_threads; ++i) {
		for (auto& op : history[i]) {
			if (false == op.o_value) continue;
			if (op.op == 3) continue;
			if (op.op == 0) survive[op.i_value]++;
			if (op.op == 1) survive[op.i_value]--;
		}
	}
	for (int i = 0; i < KEY_RANGE; ++i) {
		int val = survive[i];
		if (val < 0) {
			std::cout << "ERROR. The value " << i << " removed while it is not in the set.\n";
			exit(-1);
		}
		else if (val > 1) {
			std::cout << "ERROR. The value " << i << " is added while the set already have it.\n";
			exit(-1);
		}
		else if (val == 0) {
			if (my_set.Contains(i)) {
				std::cout << "ERROR. The value " << i << " should not exists.\n";
				exit(-1);
			}
		}
		else if (val == 1) {
			if (false == my_set.Contains(i)) {
				std::cout << "ERROR. The value " << i << " shoud exists.\n";
				exit(-1);
			}
		}
	}
	std::cout << " OK\n";
}

template<class T>
void benchmark(const int num_thread, T& my_set)
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

	// case 1: 성긴 동기화
	//std::cout << "------ 성긴 동기화 검사 ------" << std::endl;
	//for (int n = 1; n <= 16; n = n * 2) {
	//	my_set1.clear();
	//	for (auto& v : history)
	//		v.clear();
	//	std::vector<std::thread> tv;
	//	auto start_t = high_resolution_clock::now();
	//	for (int i = 0; i < n; ++i) {
	//		tv.emplace_back(worker_check<C_SET>, n, i, std::ref(my_set1));
	//	}
	//	for (auto& th : tv)
	//		th.join();
	//	auto end_t = high_resolution_clock::now();
	//	auto exec_t = end_t - start_t;
	//	size_t ms = duration_cast<milliseconds>(exec_t).count();
	//	std::cout << n << " Threads,  " << ms << "ms.";
	//	my_set1.print20();
	//	check_history(n, my_set1);
	//}

	//// case 2. 세밀한 동기화
	//std::cout << "------ 세밀한 동기화 검사 ------" << std::endl;
	//for (int n = 1; n <= 16; n = n * 2) {
	//	my_set2.clear();
	//	for (auto& v : history)
	//		v.clear();
	//	std::vector<std::thread> tv;
	//	auto start_t = high_resolution_clock::now();
	//	for (int i = 0; i < n; ++i) {
	//		tv.emplace_back(worker_check<F_SET>, n, i, std::ref(my_set2));
	//	}
	//	for (auto& th : tv)
	//		th.join();
	//	auto end_t = high_resolution_clock::now();
	//	auto exec_t = end_t - start_t;
	//	size_t ms = duration_cast<milliseconds>(exec_t).count();
	//	std::cout << n << " Threads,  " << ms << "ms.";
	//	my_set2.print20();
	//	check_history(n, my_set2);
	//}

	//// case 3. 낙천적 동기화
	//std::cout << "------ 낙천적 동기화 검사 ------" << std::endl;
	//for (int n = 1; n <= 16; n = n * 2) {
	//	my_set3.clear();
	//	for (auto& v : history)
	//		v.clear();
	//	std::vector<std::thread> tv;
	//	auto start_t = high_resolution_clock::now();
	//	for (int i = 0; i < n; ++i) {
	//		tv.emplace_back(worker_check<O_SET>, n, i, std::ref(my_set3));
	//	}
	//	for (auto& th : tv)
	//		th.join();
	//	auto end_t = high_resolution_clock::now();
	//	auto exec_t = end_t - start_t;
	//	size_t ms = duration_cast<milliseconds>(exec_t).count();
	//	std::cout << n << " Threads,  " << ms << "ms.";
	//	my_set3.print20();
	//	check_history(n, my_set3);
	//}

	//// case 4. 게으른 동기화
	//std::cout << "------ 게으른 동기화 검사 ------" << std::endl;
	//for (int n = 1; n <= 16; n = n * 2) {
	//	my_set4.clear();
	//	for (auto& v : history)
	//		v.clear();
	//	std::vector<std::thread> tv;
	//	auto start_t = high_resolution_clock::now();
	//	for (int i = 0; i < n; ++i) {
	//		tv.emplace_back(worker_check<L_SET>, n, i, std::ref(my_set4));
	//	}
	//	for (auto& th : tv)
	//		th.join();
	//	auto end_t = high_resolution_clock::now();
	//	auto exec_t = end_t - start_t;
	//	size_t ms = duration_cast<milliseconds>(exec_t).count();
	//	std::cout << n << " Threads,  " << ms << "ms.";
	//	my_set4.print20();
	//	check_history(n, my_set4);
	//}

	//// 성능 체크
	//std::cout << "------ 성긴 동기화 성능체크 ------" << std::endl;
	//for (int n = 1; n <= 16; n = n * 2) {
	//	my_set1.clear();
	//	std::vector<std::thread> tv;
	//	auto start_t = high_resolution_clock::now();
	//	for (int i = 0; i < n; ++i) {
	//		tv.emplace_back(benchmark<C_SET>, n, std::ref(my_set1));
	//	}
	//	for (auto& th : tv)
	//		th.join();
	//	auto end_t = high_resolution_clock::now();
	//	auto exec_t = end_t - start_t;
	//	size_t ms = duration_cast<milliseconds>(exec_t).count();
	//	std::cout << n << " Threads,  " << ms << "ms.";
	//	my_set1.print20();
	//}

	//std::cout << "------ 세밀한 동기화 성능체크 ------" << std::endl;
	//for (int n = 1; n <= 16; n = n * 2) {
	//	my_set2.clear();
	//	std::vector<std::thread> tv;
	//	auto start_t = high_resolution_clock::now();
	//	for (int i = 0; i < n; ++i) {
	//		tv.emplace_back(benchmark<F_SET>, n, std::ref(my_set2));
	//	}
	//	for (auto& th : tv)
	//		th.join();
	//	auto end_t = high_resolution_clock::now();
	//	auto exec_t = end_t - start_t;
	//	size_t ms = duration_cast<milliseconds>(exec_t).count();
	//	std::cout << n << " Threads,  " << ms << "ms.";
	//	my_set2.print20();
	//}

	//std::cout << "------ 낙천적 동기화 성능체크 ------" << std::endl;
	//for (int n = 1; n <= 16; n = n * 2) {
	//	my_set3.clear();
	//	std::vector<std::thread> tv;
	//	auto start_t = high_resolution_clock::now();
	//	for (int i = 0; i < n; ++i) {
	//		tv.emplace_back(benchmark<O_SET>, n, std::ref(my_set3));
	//	}
	//	for (auto& th : tv)
	//		th.join();
	//	auto end_t = high_resolution_clock::now();
	//	auto exec_t = end_t - start_t;
	//	size_t ms = duration_cast<milliseconds>(exec_t).count();
	//	std::cout << n << " Threads,  " << ms << "ms.";
	//	my_set3.print20();
	//}

	std::cout << "------ 게으른 동기화 성능체크 ------" << std::endl;
	for (int n = 1; n <= 16; n = n * 2) {
		my_set4.clear();
		std::vector<std::thread> tv;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			tv.emplace_back(benchmark<L_SET>, n, std::ref(my_set4));
		}
		for (auto& th : tv)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		size_t ms = duration_cast<milliseconds>(exec_t).count();
		std::cout << n << " Threads,  " << ms << "ms.";
		my_set4.print20();
	}
}