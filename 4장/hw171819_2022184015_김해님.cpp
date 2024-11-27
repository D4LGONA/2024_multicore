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

class SK_LF_NODE;
class SK_SPTR
{
	std::atomic<long long> sptr;
public:
	SK_SPTR() : sptr(0) {}
	void set_ptr(SK_LF_NODE* ptr)
	{
		sptr = reinterpret_cast<long long>(ptr);
	}
	SK_LF_NODE* get_ptr()
	{
		return reinterpret_cast<SK_LF_NODE*>(sptr & 0xFFFFFFFFFFFFFFFE);
	}
	SK_LF_NODE* get_ptr(bool* removed)
	{
		long long p = sptr;
		*removed = (1 == (p & 1));
		return reinterpret_cast<SK_LF_NODE*>(p & 0xFFFFFFFFFFFFFFFE);
	}
	bool get_removed()
	{
		return (1 == (sptr & 1));
	}
	bool CAS(SK_LF_NODE* old_p, SK_LF_NODE* new_p, bool old_m, bool new_m)
	{
		long long old_v = reinterpret_cast<long long>(old_p);
		if (true == old_m) old_v = old_v | 1;
		else
			old_v = old_v & 0xFFFFFFFFFFFFFFFE;
		long long new_v = reinterpret_cast<long long>(new_p);
		if (true == new_m) new_v = new_v | 1;
		else
			new_v = new_v & 0xFFFFFFFFFFFFFFFE;
		return std::atomic_compare_exchange_strong(&sptr, &old_v, new_v);
	}
};

class SK_LF_NODE {
public:
	int	key;
	int top_level;
	SK_SPTR next[MAX_TOP + 1];
	SK_LF_NODE(int x, const int top) : key(x), top_level(top) {
		//for (auto& p : next) p = nullptr;
	}
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

class LF_SK_SET {
	SK_LF_NODE head{ std::numeric_limits<int>::min(), MAX_TOP };
	SK_LF_NODE tail{ std::numeric_limits<int>::max(), MAX_TOP };
public:
	LF_SK_SET()
	{
		for (auto& p : head.next) p.set_ptr(&tail);
	}
	void clear()
	{
		while (head.next[0].get_ptr() != &tail) {
			auto p = head.next[0].get_ptr();
			head.next[0].set_ptr(head.next[0].get_ptr()->next[0].get_ptr());
			delete p;
		}
		for (auto& p : head.next)
			p.set_ptr(&tail);
	}
	bool Find(int x, SK_LF_NODE* prevs[], SK_LF_NODE* currs[])
	{
		bool marked = false;
		bool snip;
		SK_LF_NODE* pred = nullptr;
		SK_LF_NODE* curr = nullptr;
		SK_LF_NODE* succ = nullptr;
	retry:
		while (true) {
			pred = &head;
			for (int i = MAX_TOP; i >= 0; --i) {
				curr = pred->next[i].get_ptr();
				while (true) {
					succ = curr->next[i].get_ptr(&marked);
					while (marked) {
						snip = pred->next[i].CAS(curr, succ, false, false);
						if (false == snip) 
							goto retry;
						curr = pred->next[i].get_ptr();
						succ = curr->next[i].get_ptr(&marked);
					}
					if (curr->key < x) {
						pred = curr;
						curr = succ;
					}
					else break;
				}
				prevs[i] = pred;
				currs[i] = curr;
			}
			return (curr->key == x);
		}
	}
	bool Add(int x)
	{
		SK_LF_NODE* prevs[MAX_TOP + 1];
		SK_LF_NODE* currs[MAX_TOP + 1];
		int lv = 0;
		for (int i = 0; i < MAX_TOP; ++i) {
			if (rand() % 2 == 0) break;
			lv++;
		}
		while (true) {
			bool found = Find(x, prevs, currs);
			if (true == found)
				return false;
			SK_LF_NODE* n = new SK_LF_NODE{ x, lv };
			for (int i = 0; i <= lv; ++i) { // 내 뒤에 애
				n->next[i].set_ptr(currs[i]);
			}
			SK_LF_NODE* pred = prevs[0];
			SK_LF_NODE* curr = currs[0];
			n->next[0].set_ptr(curr);
			if (false == prevs[0]->next[0].CAS(currs[0], n, false, false)) // 내 앞에 애
				continue;
			for (int i = 1; i <= lv; ++i) {
				while (true) {
					pred = prevs[i];
					curr = currs[i];
					if (true == pred->next[i].CAS(currs[i], n, false, false))
						break;
					Find(x, prevs, currs);
				}
			}
			return true;
		}
	}
	bool Remove(int x) {
		SK_LF_NODE* prevs[MAX_TOP + 1];
		SK_LF_NODE* currs[MAX_TOP + 1];

		while (true) {
			bool found = Find(x, prevs, currs);
			if (!found) return false;

			SK_LF_NODE* nodeToRemove = currs[0];
			for (int i = nodeToRemove->top_level; i >= 1; --i) {
				bool marked = false;
				SK_LF_NODE* succ = nodeToRemove->next[i].get_ptr(&marked);
				while (!marked) {
					nodeToRemove->next[i].CAS(succ, succ, false, true);
					succ = nodeToRemove->next[i].get_ptr(&marked);
				}

				if (false == marked)
					int k = 0;
			}

			bool marked = false;
			SK_LF_NODE* succ = nodeToRemove->next[0].get_ptr(&marked);
			while (true) {
				bool b = nodeToRemove->next[0].CAS(succ, succ, false, true);
				succ = nodeToRemove->next[0].get_ptr(&marked);
				if (b == true) {
					Find(x, prevs, currs);
					return true;
				}
				else if (marked) {
					return false;
				}
				
			}
		}
	}
	bool Contains(int x)
	{
		SK_LF_NODE* prevs = &head;
		SK_LF_NODE* currs;
		SK_LF_NODE* succ;
		bool marked;
		for (int i = MAX_TOP; i >= 0; --i) {
			currs = prevs->next[i].get_ptr();
			while (true) {
				succ = currs->next[i].get_ptr(&marked);
				while (marked) {
					currs = currs->next[i].get_ptr();
					succ = currs->next[i].get_ptr(&marked);
				}
				if (currs->key < x) {
					prevs = currs;
					currs = succ;
				}
				else break;

			}
		}
		return (currs->key == x);
	}
	void print20() // 0번만 따라가면 됨.
	{
		auto p = head.next[0].get_ptr();
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			std::cout << p->key << ", ";
			p = p->next[0].get_ptr();
		}
		std::cout << std::endl;
	}
};

LF_SK_SET my_set;

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
		my_set.print20();
		check_history(n);
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