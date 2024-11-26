#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <array>
#include <atomic>
#include <queue>
#include <set>

constexpr int MAX_THREADS = 16;
constexpr int MAX_TOP = 9;
thread_local int thread_id;

class SK_NODE {
public:
	int	key;
	SK_NODE* volatile next[MAX_TOP + 1];
	int top_level;

	volatile bool fully_linked;
	volatile bool removed;
	std::recursive_mutex n_lock;
	SK_NODE(int x, const int top) : key(x), top_level(top), fully_linked(false), removed(false) {
		for (auto& p : next) p = nullptr;
	}

	void Lock() { n_lock.lock(); }
	void Unlock() { n_lock.unlock(); }
};

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

class C_SK_SET {
	SK_NODE head{ std::numeric_limits<int>::min(), MAX_TOP };
	SK_NODE tail{ std::numeric_limits<int>::max(), MAX_TOP };
	std::mutex set_lock;
public:
	C_SK_SET()
	{
		for (auto& p : head.next) p = &tail;
	}
	void clear()
	{
		while (head.next[0] != &tail) {
			auto p = head.next[0];
			head.next[0] = head.next[0]->next[0];
			delete p;
		}
		for (auto& p : head.next)
			p = &tail;
	}
	void Find(int x, SK_NODE* prevs[], SK_NODE* currs[])
	{
		for (int i = MAX_TOP; i >= 0; --i) {
			if (i == MAX_TOP)
				prevs[i] = &head;
			else
				prevs[i] = prevs[i + 1];
			currs[i] = prevs[i]->next[i];

			while (currs[i] != nullptr && currs[i]->key < x) { // currs[i]가 nullptr인지 확인
				prevs[i] = currs[i];
				currs[i] = currs[i]->next[i];
			}
		}
	}
	bool Add(int x)
	{
		// 1. 검색하기.
		SK_NODE* prevs[MAX_TOP + 1];
		SK_NODE* currs[MAX_TOP + 1];
		set_lock.lock();
		Find(x, prevs, currs); // 삽입할 곳을 찾음.

		if (currs[0]->key == x and currs[0] != nullptr) { // 이미 있는 값
			set_lock.unlock();
			return false;
		}
		else {
			int lv = 0;
			for (int i = 0; i < MAX_TOP; ++i) {
				if (rand() % 2 == 0) break;
				lv++;
			}
			SK_NODE* n = new SK_NODE{ x, lv };
			for (int i = 0; i <= lv; ++i) {
				n->next[i] = currs[i];
				prevs[i]->next[i] = n;
			}
			set_lock.unlock();
			return true;
		}
	}
	bool Remove(int x)
	{
		//return true;
		// 1. 검색하기.
		SK_NODE* prevs[MAX_TOP + 1];
		SK_NODE* currs[MAX_TOP + 1];
		set_lock.lock();
		Find(x, prevs, currs); // 삭제할 것 찾기: curr이 삭제할 것?

		if (currs[0]->key != x) {
			set_lock.unlock();
			return false; // 존재하지 않음
		}
		else {
			int lv = currs[0]->top_level;
			for (int i = 0; i <= lv; ++i) {
				prevs[i]->next[i] = currs[i]->next[i];
			}

			set_lock.unlock();
			delete currs[0];
			return true;
		}

	}
	bool Contains(int x)
	{
		SK_NODE* prevs[MAX_TOP + 1];
		SK_NODE* currs[MAX_TOP + 1];
		set_lock.lock();
		Find(x, prevs, currs);

		if (currs[0]->key == x) {
			set_lock.unlock();
			return true;
		}
		else {
			set_lock.unlock();
			return false;
		}
	}
	void print20() // 0번만 따라가면 됨.
	{
		auto p = head.next[0];
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			std::cout << p->key << ", ";
			p = p->next[0];
		}
		std::cout << std::endl;
	}
};

class L_SK_SET {
	SK_NODE head{ std::numeric_limits<int>::min(), MAX_TOP };
	SK_NODE tail{ std::numeric_limits<int>::max(), MAX_TOP };
public:
	L_SK_SET()
	{
		for (auto& p : head.next) p = &tail;
		head.fully_linked = true; // ..?
		tail.fully_linked = true;
	}
	void clear()
	{
		while (head.next[0] != &tail) {
			auto p = head.next[0];
			head.next[0] = head.next[0]->next[0];
			delete p;
		}
		for (auto& p : head.next)
			p = &tail;
	}
	int Find(int x, SK_NODE* prevs[], SK_NODE* currs[])
	{
		int found_level = -1;
		for (int i = MAX_TOP; i >= 0; --i) {
			if (i == MAX_TOP)
				prevs[i] = &head;
			else
				prevs[i] = prevs[i + 1];
			currs[i] = prevs[i]->next[i];

			while (currs[i] != nullptr && currs[i]->key < x) { // currs[i]가 nullptr인지 확인
				prevs[i] = currs[i];
				currs[i] = currs[i]->next[i];
			}
			if (-1 == found_level and currs[i]->key == x)
				found_level = i;
		}
		return found_level;
	}
	bool Add(int x)
	{
		// 1. 검색하기.
		SK_NODE* prevs[MAX_TOP + 1];
		SK_NODE* currs[MAX_TOP + 1];
		while (true) {
			if (x == 0)
				int k = 0;
			int f_level = Find(x, prevs, currs); // 삽입할 곳을 찾음.
			if (f_level != -1) { // 애가 있음
				SK_NODE* f_node = currs[f_level]; // 찾은 노드
				if (true == f_node->removed) { // valid하지 않은 애가 있음
					while (false == f_node->fully_linked);
					return false;
				}
				return false;
			}

			// topLevel 만드는 것
			int lv = 0;
			for (int i = 0; i < MAX_TOP; ++i) {
				if (rand() % 2 == 0) break;
				lv++;
			}

			// locking
			bool valid = true;
			int max_locked = -1;
			for (int i = 0; i <= lv; ++i) {
				prevs[i]->Lock();
				max_locked = i;
				valid = (false == prevs[i]->removed) and (currs[i] == prevs[i]->next[i]) and
					(false == currs[i]->removed); // valid
				if (false == valid) break;
			}
			if (false == valid) { // valid하지 않으면 처음부터 다시
				for (int i = 0; i <= max_locked; ++i)
					prevs[i]->Unlock();
				continue;
			}
			else { // linking
				SK_NODE* n = new SK_NODE{ x, lv };
				for (int i = 0; i <= lv; ++i) {
					n->next[i] = currs[i];
				}
				for (int i = 0; i <= lv; ++i) {
					prevs[i]->next[i] = n;
				}
				n->fully_linked = true;
				for (int i = 0; i <= lv; ++i)
					prevs[i]->Unlock();
				if (x == 0)
					int k = 0;
				return true;
			}
		}
	}

	bool Remove(int x)
	{

		SK_NODE* prevs[MAX_TOP + 1];
		SK_NODE* currs[MAX_TOP + 1];
		while (true) {
			int f_level = Find(x, prevs, currs);
			if (-1 == f_level) return false;
			SK_NODE* victim = currs[f_level];
			if (true == victim->removed)
				return false;
			if (true == victim->fully_linked)
				return false;
			if (f_level != victim->top_level)
				continue;

			victim->Lock();
			if (false == victim->removed)
				victim->removed = true;
			else {
				victim->Unlock();
				return false;
			}

			while (true) {
				bool valid = true;
				int max_locked = -1;
				for (int i = 0; i <= victim->top_level; ++i) {
					prevs[i]->Lock();
					valid = (prevs[i]->removed == false)
						&& (prevs[i]->next[i] == victim);
					if (false == valid) break;
				}
				if (false == valid) {
					for (int i = 0; i <= max_locked; ++i)
						prevs[i]->Unlock();
					Find(x, prevs, currs);
					continue;
				}
				for (int i = victim->top_level; i >= 0; --i) {
					prevs[i]->next[i] = currs[i]->next[i];
				}
				for (int i = 0; i <= victim->top_level; ++i)
					prevs[i]->Unlock();
				return true;
			}
		}
	}
	bool Contains(int x)
	{
		SK_NODE* prevs[MAX_TOP + 1];
		SK_NODE* currs[MAX_TOP + 1];
		int f_level = Find(x, prevs, currs);
		if (f_level == -1) return false;
		return currs[f_level]->removed == false and currs[f_level]->fully_linked == true;
	}
	void print20() // 0번만 따라가면 됨.
	{
		auto p = head.next[0];
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			std::cout << p->key << ", ";
			p = p->next[0];
		}
		std::cout << std::endl;
	}
};

C_SK_SET my_set;

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

void worker_check(int num_threads, int th_id)
{
	thread_id = th_id;
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

void benchmark(const int th_id, const int num_thread)
{
	int key;

	thread_id = th_id;

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

	for (int n = 1; n <= MAX_THREADS; n = n * 2) {
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
		check_history(n);
		my_set.print20();
	}

	for (int n = 1; n <= MAX_THREADS; n = n * 2) {
		my_set.clear();
		std::vector<std::thread> tv;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			tv.emplace_back(benchmark, i, n);
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