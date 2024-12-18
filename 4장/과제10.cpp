#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <array>
#include <atomic>
#include <queue>
#include <set>

constexpr int MAX_THREADS = 32;
class LFNODE;

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
class LFNODE
{
public:
	int key;
	SPTR next;
	int ebr_number;
	LFNODE(int v) : key(v), ebr_number(0) {}
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
class DUMMYMUTEX {
public:
	void lock() {}
	void unlock() {}
};

thread_local int thread_id;


class LF_SET {
	LFNODE head{ (int)(0x80000000) }, tail{ (int)(0x7FFFFFFF) };
public:
	LF_SET()
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
		LFNODE* curr = head.next.get_ptr();
		while (curr->key < x) {
			curr = curr->next.get_ptr();
		}
		return (false == curr->next.get_removed()) && (curr->key == x);
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
class S_STD_SET { // std::set lock free 구현
	std::set <int>m_set;
	DUMMYMUTEX set_lock;
public:
	S_STD_SET()
	{
	}
	void clear()
	{
		m_set.clear();
	}
	bool Add(int x)
	{
		return m_set.insert(x).second;
	}
	bool Remove(int x)
	{
		if (0 != m_set.count(x)) {
			m_set.erase(x);
			return true;
		}
		return false;
	}
	bool Contains(int x)
	{
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
			if (r.m_bool == true)
				m_set.erase(inv.x);
			else
				r.m_bool = false;
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
	void clear()
	{
		m_set.clear();
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
class U_NODE {
public:
	INVOCATION m_inv;
	int m_seq;
	U_NODE* volatile next;
};
class LFUNV_OBJECT {
	U_NODE* volatile m_head[MAX_THREADS];
	U_NODE tail;
	U_NODE* get_max_head() // 제일끝에잇는 노드 검색해 리턴..?
	{
		U_NODE* h = m_head[0];
		for (int i = 1; i < MAX_THREADS; ++i)
			if (h->m_seq < m_head[i]->m_seq)
				h = m_head[i];
		return h;
	}
public:
	LFUNV_OBJECT() {
		tail.m_seq = 0;
		tail.next = nullptr;
		for (auto& h : m_head)
			h = &tail;
	}
	RESPONSE apply(INVOCATION& inv)
	{
		U_NODE* prefer = new U_NODE{ inv, 0, nullptr };
		while (0 == prefer->m_seq) {
			U_NODE* head = get_max_head(); // 가장 큰 헤드값 얻기
			long long temp = 0;
			std::atomic_compare_exchange_strong(
				reinterpret_cast<volatile std::atomic_llong*>(&head->next),
				&temp, reinterpret_cast<long long>(prefer)); // next에 prefer 삽입을 시도한것 ?
			U_NODE* after = head->next; // 이제는.. nullptr이 아니다?
			after->m_seq = head->m_seq + 1;
			m_head[thread_id] = after;
		}
		// prefer 삽입을 성공하면
		SEQOBJECT std_set;
		U_NODE* p = tail.next;
		while (p != prefer) { // 순회하면서 현재의 log들 적용
			std_set.apply(p->m_inv);
			p = p->next;
		}
		return std_set.apply(inv); // prefer가 적용되고 난 후의 상태 반환
	}
	void clear()
	{
		U_NODE* p = tail.next;
		while (nullptr != p) {
			U_NODE* old_p = p;
			p = p->next;
			delete old_p;
		}
		tail.next = nullptr;
		for (auto& h : m_head)
			h = &tail;
	}
};
class STD_LF_SET { // std::set lock free 구현
	LFUNV_OBJECT m_set;
public:
	STD_LF_SET()
	{
	}
	void clear() // 로그에 넣지 말고 진짜 clear 해줘야 함
	{
		m_set.clear();
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
	void print20() // 얘 적용할때마다 화면에 출력이 됨... 그렇지 않게 해야죵
	{
		INVOCATION a{ M_PRINT20, 0 };
		m_set.apply(a);
	}
};
class WF_UNV_OBJECT {
	U_NODE* volatile announce[MAX_THREADS];
	U_NODE* volatile m_head[MAX_THREADS];
	U_NODE tail;


	U_NODE* get_max_head() {
		U_NODE* h = m_head[0];
		for (int i = 1; i < MAX_THREADS; ++i)
			if (h->m_seq < m_head[i]->m_seq)
				h = m_head[i];
		return h;
	}

public:
	WF_UNV_OBJECT() {
		tail.m_seq = 1;
		tail.next = nullptr;
		for (int i = 0; i < MAX_THREADS; ++i) {
			m_head[i] = &tail;
			announce[i] = &tail;
		}
	}

	RESPONSE apply(INVOCATION& inv) {
		announce[thread_id] = new U_NODE{ inv, 0, nullptr };
		m_head[thread_id] = get_max_head(); // 가장 큰 헤드값 얻기
		while (0 == announce[thread_id]->m_seq) {
			U_NODE* before = m_head[thread_id];
			U_NODE* help = announce[(before->m_seq + 1) % MAX_THREADS];
			U_NODE* prefer;
			if (help->m_seq == 0) prefer = help;
			else prefer = announce[thread_id];
			long long temp = 0;
			std::atomic_compare_exchange_strong(
				reinterpret_cast<volatile std::atomic_llong*>(&m_head[thread_id]->next),
				&temp, reinterpret_cast<long long>(prefer)); // next에 prefer 삽입을 시도한것 ?
			U_NODE* after = m_head[thread_id]->next;
			before->next = after;
			after->m_seq = before->m_seq + 1;
			m_head[thread_id] = after;
		}

		// prefer 삽입을 성공하면
		SEQOBJECT std_set;
		U_NODE* p = tail.next;
		while (p != announce[thread_id]) { // 순회하면서 현재의 log들 적용
			std_set.apply(p->m_inv);
			p = p->next;
		}
		m_head[thread_id] = announce[thread_id];
		return std_set.apply(inv); // prefer가 적용되고 난 후의 상태 반환
	}


	void clear() {
		U_NODE* p = tail.next;
		while (nullptr != p) {
			U_NODE* old_p = p;
			p = p->next;
			delete old_p;
		}
		tail.next = nullptr;
		for (int i = 0; i < MAX_THREADS; ++i) {
			m_head[i] = &tail;
			announce[i] = &tail;
		}
	}
};
class STD_WF_SET {
	WF_UNV_OBJECT m_set;

public:
	void clear() {
		m_set.clear();
	}

	bool Add(int x) {
		INVOCATION a{ M_ADD, x };
		return m_set.apply(a).m_bool;
	}

	bool Remove(int x) {
		INVOCATION a{ M_REMOVE, x };
		return m_set.apply(a).m_bool;
	}

	bool Contains(int x) {
		INVOCATION a{ M_CONTAINS, x };
		return m_set.apply(a).m_bool;
	}

	void print20() {
		INVOCATION a{ M_PRINT20, 0 };
		m_set.apply(a);
	}
};

S_STD_SET real_set;
STD_WF_SET my_set;
STD_SET my_set1;
LF_SET my_set2;
STD_LF_SET my_set3;

const int NUM_TEST = 40000;
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
}

int main()
{
	using namespace std::chrono;

	/*{
		std::cout << "------ S_STD_SET 검사 ------" << std::endl;
		for (int n = 1; n <= 1; n *= 2) {
			real_set.clear();
			for (auto& v : history) v.clear();
			std::vector<std::thread> tv;
			auto start_t = high_resolution_clock::now();
			for (int i = 0; i < n; ++i) {
				tv.emplace_back(worker_check<S_STD_SET>, n, i, std::ref(real_set));
			}
			for (auto& th : tv) th.join();
			auto end_t = high_resolution_clock::now();
			std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
			check_history(n, real_set);
			real_set.print20();
		}
		real_set.clear();

		std::cout << "------ S_STD_SET 성능 ------" << std::endl;
		for (int n = 1; n <= 1; n *= 2) {
			real_set.clear();
			for (auto& v : history) v.clear();
			std::vector<std::thread> tv;
			auto start_t = high_resolution_clock::now();
			for (int i = 0; i < n; ++i) {
				tv.emplace_back(benchmark<S_STD_SET>, n, i, std::ref(real_set));
			}
			for (auto& th : tv) th.join();
			auto end_t = high_resolution_clock::now();
			std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
			real_set.print20();
		}
		real_set.clear();
	}*/

	{
		std::cout << "------ STD_WF_SET 검사 ------" << std::endl;
		for (int n = 1; n <= MAX_THREADS; n *= 2) {
			my_set.clear();
			for (auto& v : history) v.clear();
			std::vector<std::thread> tv;
			auto start_t = high_resolution_clock::now();
			for (int i = 0; i < n; ++i) {
				tv.emplace_back(worker_check<STD_WF_SET>, n, i, std::ref(my_set));
			}
			for (auto& th : tv) th.join();
			auto end_t = high_resolution_clock::now();
			std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
			check_history(n, my_set);
			my_set.print20();
		}
		my_set.clear();

		std::cout << std::endl;
		std::cout << "------ STD_WF_SET 성능 ------" << std::endl;
		for (int n = 1; n <= MAX_THREADS; n *= 2) {
			my_set.clear();
			for (auto& v : history) v.clear();
			std::vector<std::thread> tv;
			auto start_t = high_resolution_clock::now();
			for (int i = 0; i < n; ++i) {
				tv.emplace_back(benchmark<STD_WF_SET>, n, i, std::ref(my_set));
			}
			for (auto& th : tv) th.join();
			auto end_t = high_resolution_clock::now();
			std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
			my_set.print20();
		}
		my_set.clear();
	}

	/*{
		std::cout << "------ STD_SET 검사 ------" << std::endl;
		for (int n = 1; n <= MAX_THREADS; n *= 2) {
			my_set1.clear();
			for (auto& v : history) v.clear();
			std::vector<std::thread> tv;
			auto start_t = high_resolution_clock::now();
			for (int i = 0; i < n; ++i) {
				tv.emplace_back(worker_check<STD_SET>, n, i, std::ref(my_set1));
			}
			for (auto& th : tv) th.join();
			auto end_t = high_resolution_clock::now();
			std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
			check_history(n, my_set1);
			my_set1.print20();
		}
		my_set1.clear();

		std::cout << "------ STD_SET 성능 ------" << std::endl;
		for (int n = 1; n <= MAX_THREADS; n *= 2) {
			my_set1.clear();
			for (auto& v : history) v.clear();
			std::vector<std::thread> tv;
			auto start_t = high_resolution_clock::now();
			for (int i = 0; i < n; ++i) {
				tv.emplace_back(benchmark<STD_SET>, n, i, std::ref(my_set1));
			}
			for (auto& th : tv) th.join();
			auto end_t = high_resolution_clock::now();
			std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
			my_set1.print20();
		}
		my_set1.clear();
	}*/

	/*{
		std::cout << "------ LF_SET 검사 ------" << std::endl;
		for (int n = 1; n <= MAX_THREADS; n *= 2) {
			my_set2.clear();
			for (auto& v : history) v.clear();
			std::vector<std::thread> tv;
			auto start_t = high_resolution_clock::now();
			for (int i = 0; i < n; ++i) {
				tv.emplace_back(worker_check<LF_SET>, n, i, std::ref(my_set2));
			}
			for (auto& th : tv) th.join();
			auto end_t = high_resolution_clock::now();
			std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
			check_history(n, my_set2);
			my_set2.print20();
		}
		my_set2.clear();

		std::cout << "------ LF_SET 성능 ------" << std::endl;
		for (int n = 1; n <= MAX_THREADS; n *= 2) {
			my_set2.clear();
			for (auto& v : history) v.clear();
			std::vector<std::thread> tv;
			auto start_t = high_resolution_clock::now();
			for (int i = 0; i < n; ++i) {
				tv.emplace_back(benchmark<LF_SET>, n, i, std::ref(my_set2));
			}
			for (auto& th : tv) th.join();
			auto end_t = high_resolution_clock::now();
			std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
			my_set2.print20();
		}
		my_set2.clear();
	}*/

	/*{
		std::cout << "------ STD_LF_SET 검사 ------" << std::endl;
		for (int n = 1; n <= MAX_THREADS; n *= 2) {
			my_set3.clear();
			for (auto& v : history) v.clear();
			std::vector<std::thread> tv;
			auto start_t = high_resolution_clock::now();
			for (int i = 0; i < n; ++i) {
				tv.emplace_back(worker_check<STD_LF_SET>, n, i, std::ref(my_set3));
			}
			for (auto& th : tv) th.join();
			auto end_t = high_resolution_clock::now();
			std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
			check_history(n, my_set3);
			my_set3.print20();
		}
		my_set.clear();

		std::cout << "------ STD_LF_SET 성능 ------" << std::endl;
		for (int n = 1; n <= MAX_THREADS; n *= 2) {
			my_set3.clear();
			for (auto& v : history) v.clear();
			std::vector<std::thread> tv;
			auto start_t = high_resolution_clock::now();
			for (int i = 0; i < n; ++i) {
				tv.emplace_back(benchmark<STD_LF_SET>, n, i, std::ref(my_set3));
			}
			for (auto& th : tv) th.join();
			auto end_t = high_resolution_clock::now();
			std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms." << std::endl;
			my_set3.print20();
		}
		my_set3.clear();
	}*/

}

