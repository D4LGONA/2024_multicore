#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <array>
#include <atomic>
#include <queue>
#include <set>
//#include <concurrent_unordered_set.h> // 락킹을 걸지 않고 멀티스레드에서 실행가능.
// add는 되는데 remove가 안된다..?

constexpr int MAX_THREADS = 16;
class LFNODE; // 전방선언

class SPTR
{
	std::atomic<long long> sptr;
public:
	SPTR() : sptr(0) {}
	void set_ptr(LFNODE* ptr)
	{
		sptr = reinterpret_cast<long long>(ptr);
	}
	LFNODE* get_ptr()
	{
		return reinterpret_cast<LFNODE*>(sptr & 0xFFFFFFFFFFFFFFFE);
	}
	LFNODE* get_ptr(bool* removed)
	{
		long long p = sptr;
		*removed = (1 == (p & 1));
		return reinterpret_cast<LFNODE*>(p & 0xFFFFFFFFFFFFFFFE);
	}
	bool get_removed()
	{
		return (1 == (sptr & 1));
	}
	bool CAS(LFNODE* old_p, LFNODE* new_p, bool old_m, bool new_m)
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

class NODE {
public:
	int	key;
	NODE* volatile next;
	std::mutex  n_lock;
	bool removed;
	int ebr_number;
	NODE(int x) : removed(false), key(x), next(nullptr), ebr_number(0) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};
class LFNODE
{
public:
	int key;
	SPTR next;
	int ebr_number;
	LFNODE(int v) : key(v), ebr_number(0) {}
};

thread_local std::queue <LFNODE*> m_free_queue;
thread_local std::queue <NODE*> m_l_free_queue;
thread_local int thread_id;

class EBR
{
	std::atomic_int epoch_counter;
	std::atomic_int epoch_array[MAX_THREADS * 16];
public:
	EBR() :epoch_counter(1) {
	}
	~EBR() {
		clear();
	}
	void clear()
	{
		while (false == m_free_queue.empty()) m_free_queue.pop();
		epoch_counter = 1;
	}
	void reuse(LFNODE* node)
	{
		node->ebr_number = epoch_counter;
		m_free_queue.push(node);
		// PLAN_A : if (m_free_queue.size() > MAX_FREE_SIZE) safe_delete();
		//    낭비되는 메모리의 최대값을 제한한다.
	}
	void start_epoch()
	{
		int epoch = epoch_counter++;
		epoch_array[thread_id * 16] = epoch;
	}
	void end_epoch()
	{
		epoch_array[thread_id] = 0;
	}

	// PLAN_B : 가능한 한 new/delete없이 재사용.
	LFNODE* get_node(int x)
	{
		if (true == m_free_queue.empty())
			return new LFNODE{ x };
		LFNODE* p = m_free_queue.front();
		for (int i = 0; i < MAX_THREADS; ++i) {
			if ((epoch_array[i * MAX_THREADS] != 0) && (epoch_array[i * MAX_THREADS] < p->ebr_number)) {
				return new LFNODE{ x };
			}
		}
		m_free_queue.pop();
		p->key = x;
		p->next.set_ptr(nullptr);
		return p;
	}
};
class L_EBR
{
	std::atomic_int epoch_counter;
	std::atomic_int epoch_array[MAX_THREADS * 16];
public:
	L_EBR() :epoch_counter(1) {
	}
	~L_EBR() {
		clear();
	}
	void clear()
	{
		while (false == m_l_free_queue.empty()) m_l_free_queue.pop();
		epoch_counter = 1;
	}
	void reuse(NODE* node)
	{
		node->ebr_number = epoch_counter;
		m_l_free_queue.push(node);
		// PLAN_A : if (m_free_queue.size() > MAX_FREE_SIZE) safe_delete();
		//    낭비되는 메모리의 최대값을 제한한다.
	}
	void start_epoch()
	{
		int epoch = epoch_counter++;
		epoch_array[thread_id * 16] = epoch;
	}
	void end_epoch()
	{
		epoch_array[thread_id] = 0;
	}

	// PLAN_B : 가능한 한 new/delete없이 재사용.
	NODE* get_node(int x)
	{
		if (true == m_l_free_queue.empty())
			return new NODE{ x };
		NODE* p = m_l_free_queue.front();
		for (int i = 0; i < MAX_THREADS; ++i) {
			if ((epoch_array[i * MAX_THREADS] != 0) && (epoch_array[i * MAX_THREADS] < p->ebr_number)) {
				return new NODE{ x };
			}
		}
		m_l_free_queue.pop();
		p->key = x;
		return p;
	}
};

EBR ebr;
L_EBR l_ebr;


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
		return  (false == prev->removed) && (false == curr->removed) && (curr == prev->next);
	}

	bool Add(int x)
	{
		auto p = new NODE{ x };
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
			if (curr->key != x) {
				prev->unlock(); curr->unlock();
				return false;
			}
			else {
				curr->removed = true;
				std::atomic_thread_fence(std::memory_order_seq_cst);
				prev->next = curr->next;
				prev->unlock(); curr->unlock();
				return true;
			}
		}
	}
	bool Contains(int x)
	{
		auto curr = head.next;
		while (curr->key < x) {
			curr = curr->next;
		}
		return (false == curr->removed) && (curr->key == x);
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
class LF_SET {
	LFNODE head{ (int)(0x80000000) }, tail{ (int)(0x7FFFFFFF) };
public:
	LF_SET()
	{
		head.next.set_ptr(& tail);
	}
	void clear()
	{
		while (head.next.get_ptr() != &tail) {
			auto p = head.next.get_ptr();
			head.next.set_ptr(p->next.get_ptr());
			delete p;
		}
	}

	void Find(int x, LFNODE*& prev, LFNODE*& curr)
	{
		while (true) {
		retry:
			prev = &head;
			curr = prev->next.get_ptr();

			while (true) {
				// 청소
				bool removed = false;
				do {
					LFNODE* succ = curr->next.get_ptr(&removed);
					if (removed == true) {
						if (false == prev->next.CAS(curr, succ, false, false)) goto retry;
						curr = succ;
					}
				} while (removed == true);

				while (curr->key >= x) return;
				prev = curr;
				curr = curr->next.get_ptr();
			}
		}
	}

	bool Add(int x)
	{
		auto p = new LFNODE{ x };
		while (true) {
			// 검색 Phase
			LFNODE* prev, * curr;
			Find(x, prev, curr);

			if (curr->key == x) {
				delete p;
				return false;
			}
			else {
				p->next.set_ptr(curr);
				if (true == prev->next.CAS(curr, p, false, false))
					return true;
			}
		}
	}
	bool Remove(int x)
	{
		while (true) {
			LFNODE* prev, * curr;
			Find(x, prev, curr);
			if (curr->key != x) {
				return false;
			}
			else {
				LFNODE* succ = curr->next.get_ptr();
				if (false == curr->next.CAS(succ, succ, false, true))
					continue;
				prev->next.CAS(curr, succ, false, false);
				return true;
			}
		}
	}
	bool Contains(int x)
	{
		LFNODE *curr = head.next.get_ptr();
		while (curr->key < x) {
			curr = curr->next.get_ptr();
		}
		return (false == curr->next.get_removed()) && (curr->key == x);
	}
	void print20()
	{
		LFNODE *p = head.next.get_ptr();
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			std::cout << p->key << ", ";
			p = p->next.get_ptr();
		}
		std::cout << std::endl;
	}
};
class EBR_LF_SET {
	LFNODE head{ (int)(0x80000000) }, tail{ (int)(0x7FFFFFFF) };
public:
	EBR_LF_SET()
	{
		head.next.set_ptr(&tail);
	}
	void clear()
	{
		while (head.next.get_ptr() != &tail) {
			auto p = head.next.get_ptr();
			head.next.set_ptr(p->next.get_ptr());
			delete p;
		}
	}

	void Find(int x, LFNODE*& prev, LFNODE*& curr)
	{
		while (true) {
		retry:
			prev = &head;
			curr = prev->next.get_ptr();

			while (true) {
				// 청소
				bool removed = false;
				do {
					LFNODE* succ = curr->next.get_ptr(&removed);
					if (removed == true) {
						if (false == prev->next.CAS(curr, succ, false, false)) goto retry;
						ebr.reuse(curr);
						curr = succ;
					}
				} while (removed == true);

				while (curr->key >= x) return;
				prev = curr;
				curr = curr->next.get_ptr();
			}
		}
	}

	bool Add(int x)
	{
		auto p = ebr.get_node(x);
		ebr.start_epoch();
		while (true) {
			// 검색 Phase
			LFNODE* prev, * curr;
			Find(x, prev, curr);

			if (curr->key == x) {
				ebr.end_epoch();
				delete p;
				return false;
			}
			else {
				p->next.set_ptr(curr);
				if (true == prev->next.CAS(curr, p, false, false)) {
					ebr.end_epoch();
					return true;
				}
			}
		}
	}
	bool Remove(int x)
	{
		ebr.start_epoch();
		while (true) {
			LFNODE* prev, * curr;
			Find(x, prev, curr);
			if (curr->key != x) {
				ebr.end_epoch();
				return false;
			}
			else {
				LFNODE* succ = curr->next.get_ptr();
				if (false == curr->next.CAS(succ, succ, false, true))
					continue;
				if (true == prev->next.CAS(curr, succ, false, false))
					ebr.reuse(curr);
				ebr.end_epoch();
				return true;
			}
		}
	}
	bool Contains(int x)
	{
		ebr.start_epoch();
		LFNODE* curr = head.next.get_ptr();
		while (curr->key < x) {
			curr = curr->next.get_ptr();
		}
		bool result = (false == curr->next.get_removed()) && (curr->key == x);
		ebr.end_epoch();
		return result;

		//ebr.start_epoch();
		//LFNODE* prev;
		//LFNODE* curr;
		////Find(x, prev, curr);

		//bool res = (curr != &tail && curr->key == x && !curr->next.get_removed());
		//ebr.end_epoch();
		//return res;
	}
	void print20()
	{
		LFNODE* p = head.next.get_ptr();
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			std::cout << p->key << ", ";
			p = p->next.get_ptr();
		}
		std::cout << std::endl;
	}
};
class EBR_L_SET
{
	NODE head{ (int)(0x80000000) }, tail{ (int)(0x7FFFFFFF) };
public:
	EBR_L_SET()
	{
		head.next = &tail;
	}
	void clear()
	{
		while (head.next != &tail) {
			auto p = head.next;
			head.next = head.next->next;
			delete p;  // 메모리 해제
		}
	}
	bool validate(const int x, NODE* prev, NODE* curr)
	{
		return  (false == prev->removed) && (false == curr->removed) && (curr == prev->next);
	}

	bool Add(int x)
	{
		ebr.start_epoch();  // EBR epoch 시작
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
				ebr.end_epoch();  // EBR epoch 종료
				return false;
			}
			else {
				NODE* p = l_ebr.get_node(x);
				p->next = curr;
				prev->next = p;
				prev->unlock(); curr->unlock();
				ebr.end_epoch();  // EBR epoch 종료
				return true;
			}
		}
	}

	bool Remove(int x)
	{
		ebr.start_epoch();  // EBR epoch 시작
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
			if (curr->key != x) {
				prev->unlock(); curr->unlock();
				ebr.end_epoch();  // EBR epoch 종료
				return false;
			}
			else {
				curr->removed = true;
				std::atomic_thread_fence(std::memory_order_seq_cst);
				prev->next = curr->next;
				prev->unlock(); curr->unlock();
				ebr.reuse(reinterpret_cast<LFNODE*>(curr));  // 노드 재사용을 위해 EBR에 전달
				ebr.end_epoch();  // EBR epoch 종료
				return true;
			}
		}
	}

	bool Contains(int x)
	{
		ebr.start_epoch();  // EBR epoch 시작
		auto curr = head.next;
		while (curr->key < x) {
			curr = curr->next;
		}
		bool result = (false == curr->removed) && (curr->key == x);
		ebr.end_epoch();  // EBR epoch 종료
		return result;
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

class DUMMYMUTEX {
public:
	void lock() {}
	void unlock() {}
};
class STD_SET { // std::set lock free 구현
	std::set <int>m_set;
	std::mutex set_lock;
public:
	STD_SET()
	{
	}
	void clear()
	{
		m_set.clear();
	}
	bool Add(int x)
	{
		// locking을 한 블럭에서 빠져나올 때 unlock을 해줌.
		std::lock_guard<std::mutex> aa{ set_lock };
		return m_set.insert(x).second;
	}
	bool Remove(int x)
	{
		std::lock_guard<std::mutex> aa{ set_lock };
		if (0 != m_set.count(x)) {
			m_set.erase(x);
			return true;
		}
		return false;
	}
	bool Contains(int x)
	{
		std::lock_guard<std::mutex> aa{ set_lock };
		return 0 != m_set.count(x);
	}
	void print20()
	{
		int count = 20;
		for (auto x : m_set) {
			if (count-- == 0) break;
			std::cout << x << ", ";
		}
		std::cout << std::endl;
	}
};

struct RESPONSE {
	bool m_bool; // 리턴 값?
};

enum METHOD { M_ADD, M_REMOVE, M_CONTAINS, M_CLEAR, M_PRINT20 };
struct INVOCATION {
	METHOD m_method;
	int x; // set에 넣어주는 파라미터
};

class SEQOBJECT { // std::set lock free 구현
	std::set<int> m_set;
public:
	SEQOBJECT()
	{
	}
	RESPONSE apply(INVOCATION& inv)
	{
		RESPONSE r{ true };
		switch (inv.m_method)
		{
		case M_ADD:
			r.m_bool = m_set.insert(inv.x).second;
			break;

		case M_REMOVE:
			r.m_bool = (0 != m_set.count(inv.x));
			if(r.m_bool == true)
				m_set.erase(inv.x);
			break;

		case M_CONTAINS:
			r.m_bool = (0 != m_set.count(inv.x));
			break;
		case M_CLEAR:
			m_set.clear();
			break;
		case M_PRINT20:
			int count = 20;
			for (auto x : m_set) {
				if (count-- == 0) break;
				std::cout << x << ", ";
			}
			std::cout << std::endl;
			break;
		}
		return r;
	}
};
class STD_SEQ_SET { // std::set lock free 구현
	SEQOBJECT m_set;
public:
	STD_SEQ_SET()
	{
	}
	void clear()
	{
		INVOCATION a{ M_CLEAR, 0 };
		m_set.apply(a);
	}
	bool Add(int x)
	{
		INVOCATION a{ M_ADD, x };
		return m_set.apply(a).m_bool;
		
	}
	bool Remove(int x)
	{
		INVOCATION a{ M_REMOVE, x };
		return m_set.apply(a).m_bool;
	}
	bool Contains(int x)
	{
		INVOCATION a{ M_CONTAINS, x };
		return m_set.apply(a).m_bool;
	}
	void print20()
	{
		INVOCATION a{ M_PRINT20, 0 };
		m_set.apply(a);
	}
};

STD_SEQ_SET my_set;
//LF_SET my_set1;
//EBR_L_SET my_set2;
//EBR_LF_SET my_set3;

const int NUM_TEST = 4000000;
const int KEY_RANGE = 1000;

class HISTORY
{
public:
	int op;
	int i_value;
	bool o_value;

	HISTORY(int o, int i, bool re) : op(o), i_value(i), o_value(re) {}
};

std::array<std::vector<HISTORY>, 16> history;

template <class T>
void worker_check(int num_threads, int th_id, T& my_set)
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
	while (false == m_free_queue.empty()) {
		auto p = m_free_queue.front();
		m_free_queue.pop();
		delete p;
	}
	while (false == m_l_free_queue.empty()) {
		auto p = m_free_queue.front();
		m_free_queue.pop();
		delete p;
	}
}

template <class T>
void check_history(int num_threads, T& my_set)
{
	std::array<int, KEY_RANGE> survive = {};
	std::cout << "Checking Consistency : ";
	if (history[0].size() == 0) {
		std::cout << "No history.\n";
		return;
	}
	for (int i = 0; i < num_threads; ++i) {
		for (auto& op : history[i]) {
			if (!op.o_value) continue;
			if (op.op == 0) survive[op.i_value]++;
			if (op.op == 1) survive[op.i_value]--;
		}
	}
	for (int i = 0; i < KEY_RANGE; ++i) {
		if (survive[i] < 0) {
			std::cout << "ERROR: The value " << i << " removed but not in the set.\n";
			exit(-1);
		}
		else if (survive[i] > 1) {
			std::cout << "ERROR: The value " << i << " added but already exists in the set.\n";
			exit(-1);
		}
		else if (survive[i] == 0 && my_set.Contains(i)) {
			std::cout << "ERROR: The value " << i << " should not exist.\n";
			exit(-1);
		}
		else if (survive[i] == 1 && !my_set.Contains(i)) {
			std::cout << "ERROR: The value " << i << " should exist.\n";
			exit(-1);
		}
	}
	std::cout << " OK\n";
}

template <class T>
void benchmark(int num_thread, const int th_id, T& my_set)
{
	thread_id = th_id;
	for (int i = 0; i < NUM_TEST / num_thread; i++) {
		int key = rand() % KEY_RANGE;
		switch (rand() % 3) {
		case 0: my_set.Add(key); break;
		case 1: my_set.Remove(key); break;
		case 2: my_set.Contains(key); break;
		}
	}
	while (false == m_free_queue.empty()) {
		auto p = m_free_queue.front();
		m_free_queue.pop();
		delete p;
	}
	while (false == m_l_free_queue.empty()) {
		auto p = m_free_queue.front();
		m_free_queue.pop();
		delete p;
	}
}

int main()
{
	using namespace std::chrono;

	std::cout << "------ 검사 ------" << std::endl;
	for (int n = 1; n <= MAX_THREADS; n *= 2) {
		my_set.clear();
		for (auto& v : history) v.clear();
		std::vector<std::thread> tv;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			tv.emplace_back(worker_check<STD_SET>, n, i, std::ref(my_set));
		}
		for (auto& th : tv) th.join();
		auto end_t = high_resolution_clock::now();
		std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
		my_set.print20();
		check_history(n, my_set);
	}
	my_set.clear();

	// 성능확인
	std::cout << std::endl;
	std::cout << "------ 성능 ------" << std::endl;
	for (int n = 1; n <= MAX_THREADS; n *= 2) {
		my_set.clear();
		for (auto& v : history) v.clear();
		std::vector<std::thread> tv;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < n; ++i) {
			tv.emplace_back(benchmark<STD_SEQ_SET>, n, i, std::ref(my_set));
		}
		for (auto& th : tv) th.join();
		auto end_t = high_resolution_clock::now();
		std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
		my_set.print20();
	}
	my_set.clear();

}
