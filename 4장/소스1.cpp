//10.09 ���� ������ ����ȭ
// ������ ����ȭ�� shared_ptr �߰�

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <array>
#include <numeric>
#include <atomic>

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

// shared_ptr
class SP_NODE {
public:
	int	key;
	// ��� node�� shared_ptr���� ����
	std::shared_ptr<SP_NODE> next;
	std::mutex  n_lock;
	bool removed;

	SP_NODE(int x) : key(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};

// atomic_shared_ptr
class C20_SP_NODE {
public:
	int	key;
	// ��� node�� shared_ptr���� ����
	std::atomic<std::shared_ptr<C20_SP_NODE>> next;
	std::mutex  n_lock;
	bool removed;

	C20_SP_NODE(int x) : key(x), next(nullptr), removed(false) {}
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

class C_SET {
	NODE head{ (int)(0x80000000) }, tail{ (int)(0x7FFFFFFF) };
	DUMMYMUTEX set_lock;
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

		return true;

	}
	bool Remove(int x)
	{

		return true;

	}
	bool Contains(int x)
	{

		return false;

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

		return true;

	}
	bool Remove(int x)
	{

		return true;

	}
	bool Contains(int x)
	{
		return true;
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
	bool validate(const int x, NODE* prev, NODE* curr)
	{
		auto p = &head;
		auto c = p->next;
		while (c->key < x) {
			p = c;
			c = c->next;
		}
		return (prev == p) && (curr = c);
	}

	bool Add(int x)
	{
		auto p = new NODE{ x };
		while (true) {
			// �˻� Phase
			auto prev = &head;
			auto curr = prev->next;
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}
			// Locking
			prev->lock(); curr->lock();
			// Valid �˻�
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->key == x) {
				prev->unlock(); curr->unlock();
				delete p;
				return false;
			}
			else {
				// auto p = new NODE{ x };
				p->next = curr;
				prev->next = p;
				prev->unlock(); curr->unlock();
				return true;
			}
		}
	}
	bool Remove(int x)
	{
		return true;
	}
	bool Contains(int x)
	{
		return true;
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
			// �˻� Phase
			auto prev = &head;
			auto curr = prev->next;
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}
			// Locking
			prev->lock(); curr->lock();
			// Valid �˻�
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
			// �˻�
			auto prev = &head;
			auto curr = prev->next;
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}

			// lock
			prev->lock(); curr->lock();

			// �˻�
			if (false == validate(x, prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == x) {
				curr->removed = true; // ���� 
				// std::atomic_thread_fence(std::memory_order_seq_cst); // �� ����� �̰� ������
				// ������ �ڹٲ�� ��������...
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
		// ��� ������
		auto curr = head.next;
		while (curr->key < x) {
			curr = curr->next;
		}
		return false == curr->removed and curr->key == x;

		/*while (true) {
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
		}*/
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

// shared pointer > single thread�� ����
class SP_L_SET {
	std::shared_ptr<SP_NODE> head, tail;
public:
	SP_L_SET()
	{
		head = std::make_shared<SP_NODE>( std::numeric_limits<int>::min() );
		tail = std::make_shared<SP_NODE>( std::numeric_limits<int>::max() );
		head->next = tail;
	}
	void clear()
	{
		head->next = tail; // �ڵ����� ��������
	}
	bool validate(std::shared_ptr<SP_NODE> prev, std::shared_ptr<SP_NODE> curr)
	{
		return false == prev->removed and false == curr->removed and curr == prev->next;
	}

	bool Add(int x)
	{
		auto p = std::make_shared<SP_NODE>(x);
		while (true) {
			// �˻� Phase
			auto prev = head;
			auto curr = prev->next;
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}
			// Locking
			prev->lock(); curr->lock();
			// Valid �˻�
			if (false == validate(prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->key == x) {
				prev->unlock(); curr->unlock();
				//delete p;
				return false;
			}
			else {
				// auto p = new NODE{ x };
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
			// �˻�
			auto prev = head;
			auto curr = prev->next;
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}

			// lock
			prev->lock(); curr->lock();

			// �˻�
			if (false == validate(prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == x) {
				curr->removed = true; // ���� 
				// std::atomic_thread_fence(std::memory_order_seq_cst); // �� ����� �̰� ������
				// ������ �ڹٲ�� ��������...
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
		auto curr = head->next;
		while (curr->key < x) {
			curr = curr->next;
		}
		return false == curr->removed and curr->key == x;
		
	}
	void print20()
	{
		auto p = head->next;
		for (int i = 0; i < 20; ++i) {
			if (p == tail) break;
			std::cout << p->key << ", ";
			p = p->next;
		}
		std::cout << std::endl;
	}
};

// atomic shared pointer
//class ASP_L_SET {
//	std::shared_ptr<SP_NODE> head, tail;
//public:
//	// ����� �̱۽������ ���ư���.
//	ASP_L_SET()
//	{
//		head = std::make_shared<SP_NODE>(std::numeric_limits<int>::min());
//		tail = std::make_shared<SP_NODE>(std::numeric_limits<int>::max());
//		head->next = tail;
//	}
//	void clear()
//	{
//		head->next = tail; // �ڵ����� ��������
//	}
//
//	// --------------------------------------
//
//	// ���ڷ� �Ѱܹ޴� ģ������ ���������̴�.
//	bool validate(std::shared_ptr<SP_NODE> prev, std::shared_ptr<SP_NODE> curr)
//	{
//		return false == prev->removed and false == curr->removed and curr == prev->next;
//	}
//
//	bool Add(int x)
//	{
//		auto p = std::make_shared<SP_NODE>(x);
//		while (true) {
//			// �˻� Phase
//			auto prev = head; // read only
//			auto curr = std::atomic_load(&prev->next);
//			while (curr->key < x) {
//				prev = curr;
//				curr = std::atomic_load(&curr->next);
//			}
//			// Locking
//			prev->lock(); curr->lock();
//			// Valid �˻�
//			if (false == validate(prev, curr)) {
//				prev->unlock(); curr->unlock();
//				continue;
//			}
//			if (curr->key == x) {
//				prev->unlock(); curr->unlock();
//				//delete p;
//				return false;
//			}
//			else {
//				// auto p = new NODE{ x };
//				p->next = curr; // �ٸ� �����尡 p�� �Ȥ����ϱ� ���ص���
//				std::atomic_exchange(&prev->next, p);
//				prev->unlock(); curr->unlock();
//				return true;
//			}
//		}
//	}
//	bool Remove(int x)
//	{
//		while (true) {
//			// �˻�
//			auto prev = head;
//			auto curr = std::atomic_load(&prev->next);
//			while (curr->key < x) {
//				prev = curr;
//				curr = std::atomic_load(&curr->next);
//			}
//
//			// lock
//			prev->lock(); curr->lock();
//
//			// �˻�
//			if (false == validate(prev, curr)) {
//				prev->unlock(); curr->unlock();
//				continue;
//			}
//
//			if (curr->key == x) {
//				curr->removed = true; // ���� 
//				// std::atomic_thread_fence(std::memory_order_seq_cst); // �� ����� �̰� ������
//				// ������ �ڹٲ�� ��������...
//				std::atomic_exchange(&prev->next, std::atomic_load(&curr->next));
//				//prev->next = std::atomic_load(&curr->next); // ����.. �� ���ư�����..
//				prev->unlock(); curr->unlock();
//				return true;
//			}
//			else {
//				prev->unlock(); curr->unlock();
//				return false;
//			}
//		}
//	}
//	bool Contains(int x) // ���⿡ �־���ϴ°�?..
//	{
//		auto curr = std::atomic_load(&head->next);
//		while (curr->key < x) {
//			curr = std::atomic_load(&curr->next);
//		}
//		return false == curr->removed and curr->key == x;
//
//	}
//	void print20()
//	{
//		auto p = head->next;
//		for (int i = 0; i < 20; ++i) {
//			if (p == tail) break;
//			std::cout << p->key << ", ";
//			p = p->next;
//		}
//		std::cout << std::endl;
//	}
//};

class C20_ASP_L_SET {
	std::shared_ptr<C20_SP_NODE> head, tail; // ����� atomic���� ���� �ʾƵ� ��
	// read only�̱� ������
public:
	// ����� �̱۽������ ���ư���.
	C20_ASP_L_SET()
	{
		head = std::make_shared<C20_SP_NODE>(std::numeric_limits<int>::min());
		tail = std::make_shared<C20_SP_NODE>(std::numeric_limits<int>::max());
		head->next = tail;
	}
	void clear()
	{
		head->next = tail; // �ڵ����� ��������
	}

	// --------------------------------------

	// ���� ����.
	bool validate(const std::shared_ptr<C20_SP_NODE>& prev, 
		const std::shared_ptr<C20_SP_NODE>& curr)
	{
		return false == prev->removed and false == curr->removed and curr == prev->next.load();
	}

	bool Add(int x)
	{
		const auto p = std::make_shared<C20_SP_NODE>(x);
		while (true) {
			// �˻� Phase
			auto prev = head; // read only
			auto curr = prev->next.load();
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}
			// Locking
			prev->lock(); curr->lock();
			// Valid �˻�
			if (false == validate(prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->key == x) {
				prev->unlock(); curr->unlock();
				//delete p;
				return false;
			}
			else {
				// auto p = new NODE{ x };
				p->next = curr; // �ٸ� �����尡 p�� �Ȥ����ϱ� ���ص���
				prev->next = p;
				prev->unlock(); curr->unlock();
				return true;
			}
		}
	}
	bool Remove(int x)
	{
		while (true) {
			// �˻�
			auto prev = head;
			auto curr = prev->next.load();
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}

			// lock
			prev->lock(); curr->lock();

			// �˻�
			if (false == validate(prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == x) {
				curr->removed = true; // ���� 
				// std::atomic_thread_fence(std::memory_order_seq_cst); // �� ����� �̰� ������
				// ������ �ڹٲ�� ��������...
				prev->next = curr->next.load();
				//std::atomic_exchange(&prev->next, std::atomic_load(&curr->next));
				//prev->next = std::atomic_load(&curr->next); // ����.. �� ���ư�����..
				prev->unlock(); curr->unlock();
				return true;
			}
			else {
				prev->unlock(); curr->unlock();
				return false;
			}
		}
	}
	bool Contains(int x) // ���⿡ �־���ϴ°�?..
	{
		auto curr = head->next.load();
		while (curr->key < x) {
			curr = curr->next;
		}
		return false == curr->removed and curr->key == x;

	}
	void print20()
	{
		auto p = head->next.load();
		for (int i = 0; i < 20; ++i) {
			if (p == tail) break;
			std::cout << p->key << ", ";
			p = p->next;
		}
		std::cout << std::endl;
	}
};

L_SET my_set;

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

void worker_check(int num_threads, int thread_id)
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

void check_history(int num_threads)
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
		for (auto& v : history)
			v.clear();
		std::vector<std::thread> tv;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			tv.emplace_back(worker_check, n, i);
		}
		for (auto& th : tv)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		size_t ms = duration_cast<milliseconds>(exec_t).count();
		std::cout << n << " Threads,  " << ms << "ms.";
		my_set.print20();
		check_history(n);
	}

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