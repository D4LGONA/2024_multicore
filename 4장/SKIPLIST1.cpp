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

class NODE {
public:
	int	key;
	NODE* volatile next;
	std::mutex  n_lock;
	bool removed;
	NODE(int x) : removed(false), key(x), next(nullptr) {}
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
		if (curr->key != x) {
			prev->unlock(); curr->unlock();
			return false;
		}
		else {
			prev->next = curr->next;
			prev->unlock(); curr->unlock();
			delete curr;
			return true;
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
		return (prev == p) && (curr == c);
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
				prev->next = curr->next;
				prev->unlock(); curr->unlock();
				return true;
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

class SP_NODE {
public:
	int	key;
	std::shared_ptr<SP_NODE> next;
	std::mutex  n_lock;
	bool removed;
	SP_NODE(int x) : removed(false), key(x), next(nullptr) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};

class SP_L_SET {
	std::shared_ptr<SP_NODE> head, tail;
public:
	SP_L_SET()
	{
		head = std::make_shared<SP_NODE>( std::numeric_limits<int>::min() );
		tail = std::make_shared<SP_NODE>(std::numeric_limits<int>::max());
		head->next = tail;
	}
	void clear()
	{
		head->next = tail;
	}
	bool validate(std::shared_ptr<SP_NODE> prev, std::shared_ptr<SP_NODE> curr)
	{
		return  (false == prev->removed) && (false == curr->removed) && (curr == prev->next);
	}

	bool Add(int x)
	{
		auto p = std::make_shared<SP_NODE>( x );
		while (true) {
			// 검색 Phase
			auto prev = head;
			auto curr = prev->next;
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}
			// Locking
			prev->lock(); curr->lock();
			// Valid 검사
			if (false == validate(prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->key == x) {
				prev->unlock(); curr->unlock();
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
			auto prev = head;
			auto curr = prev->next;
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (false == validate(prev, curr)) {
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
			auto curr = head->next;
			while (curr->key < x) {
				curr = curr->next;
			}
			return (false == curr->removed) && (curr->key == x);
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

//class ASP_L_SET {
//	std::shared_ptr<SP_NODE> head, tail;
//public:
//	ASP_L_SET()
//	{
//		head = std::make_shared<SP_NODE>(std::numeric_limits<int>::min());
//		tail = std::make_shared<SP_NODE>(std::numeric_limits<int>::max());
//		head->next = tail;
//	}
//	void clear()
//	{
//		head->next = tail;
//	}
//	bool validate(std::shared_ptr<SP_NODE> prev, std::shared_ptr<SP_NODE> curr)
//	{
//		return  (false == prev->removed) && (false == curr->removed) && (curr == prev->next);
//	}
//
//	bool Add(int x)
//	{
//		auto p = std::make_shared<SP_NODE>(x);
//		while (true) {
//			// 검색 Phase
//			auto prev = head;
//			auto curr = std::atomic_load(&prev->next);
//			while (curr->key < x) {
//				prev = curr;
//				curr = std::atomic_load(&curr->next);
//			}
//			// Locking
//			prev->lock(); curr->lock();
//			// Valid 검사
//			if (false == validate(prev, curr)) {
//				prev->unlock(); curr->unlock();
//				continue;
//			}
//			if (curr->key == x) {
//				prev->unlock(); curr->unlock();
//				return false;
//			}
//			else {
//				// auto p = new NODE{ x };
//				p->next = curr;
//				std::atomic_exchange(&prev->next, p);
//				prev->unlock(); curr->unlock();
//				return true;
//			}
//		}
//	}
//	bool Remove(int x)
//	{
//		while (true) {
//			auto prev = head;
//			auto curr = std::atomic_load(&prev->next);
//			while (curr->key < x) {
//				prev = curr;
//				curr = std::atomic_load(&curr->next);
//			}
//			prev->lock(); curr->lock();
//			if (false == validate(prev, curr)) {
//				prev->unlock(); curr->unlock();
//				continue;
//			}
//			if (curr->key != x) {
//				prev->unlock(); curr->unlock();
//				return false;
//			}
//			else {
//				curr->removed = true;
//				std::atomic_thread_fence(std::memory_order_seq_cst);
//				std::atomic_exchange(&prev->next, std::atomic_load(&curr->next));
//				prev->unlock(); curr->unlock();
//				return true;
//			}
//		}
//	}
//	bool Contains(int x)
//	{
//		auto curr = std::atomic_load(&head->next);
//		while (curr->key < x) {
//			curr = std::atomic_load(&curr->next);
//		}
//		return (false == curr->removed) && (curr->key == x);
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

class C20_SP_NODE {
public:
	int	key;
	std::atomic<std::shared_ptr<C20_SP_NODE>> next;
	std::mutex  n_lock;
	bool removed;
	C20_SP_NODE(int x) : removed(false), key(x), next(nullptr) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};

class C20_ASP_L_SET {
	std::shared_ptr<C20_SP_NODE> head, tail;
public:
	C20_ASP_L_SET()
	{
		head = std::make_shared<C20_SP_NODE>(std::numeric_limits<int>::min());
		tail = std::make_shared<C20_SP_NODE>(std::numeric_limits<int>::max());
		head->next = tail;
	}
	void clear()
	{
		head->next = tail;
	}
	bool validate(const std::shared_ptr<C20_SP_NODE> &prev, 
		const std::shared_ptr<C20_SP_NODE> &curr)
	{
		return  (false == prev->removed) && (false == curr->removed) && (curr == prev->next.load());
	}

	bool Add(int x)
	{
		const auto p = std::make_shared<C20_SP_NODE>(x);
		while (true) {
			// 검색 Phase
			std::shared_ptr<C20_SP_NODE> prev = head;
			auto curr = prev->next.load();
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}
			// Locking
			prev->lock(); curr->lock();
			// Valid 검사
			if (false == validate(prev, curr)) {
				prev->unlock(); curr->unlock();
				continue;
			}
			if (curr->key == x) {
				prev->unlock(); curr->unlock();
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
			auto prev = head;
			auto curr = prev->next.load();
			while (curr->key < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (false == validate(prev, curr)) {
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
				prev->next = curr->next.load();
				prev->unlock(); curr->unlock();
				return true;
			}
		}
	}
	bool Contains(int x)
	{
		auto curr = head->next.load();
		while (curr->key < x) {
			curr = curr->next;
		}
		return (false == curr->removed) && (curr->key == x);
	}
	void print20()
	{
		std::shared_ptr<C20_SP_NODE> p = head->next;
		for (int i = 0; i < 20; ++i) {
			if (p == tail) break;
			std::cout << p->key << ", ";
			p = p->next;
		}
		std::cout << std::endl;
	}
};

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
	LFNODE* get_ptr(bool *removed)
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

thread_local std::queue <LFNODE*> m_free_queue;
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
	void reuse(LFNODE *node)
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
		epoch_array[thread_id * 16] = 0;
	}

	// PLAN_B : 가능한 한 new/delete없이 재사용.
	LFNODE* get_node(int x)
	{
		if (true == m_free_queue.empty())
			return new LFNODE{ x };
		LFNODE* p = m_free_queue.front();
		for (int i = 0; i < MAX_THREADS; ++i) {
			if ((epoch_array[i * 16] != 0) && (epoch_array[i * 16] < p->ebr_number)) {
				return new LFNODE{ x };
			}
		}
		m_free_queue.pop();
		p->key = x;
		p->next.set_ptr(nullptr);
		return p;
	}
};

EBR ebr;

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


class STD_SET {
	std::set <int> m_set;
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
		//std::lock_guard<std::mutex> aa { set_lock };
		return m_set.insert(x).second;
	}
	bool Remove(int x)
	{
		//std::lock_guard<std::mutex> aa{ set_lock };
		if (0 != m_set.count(x)) {
			m_set.erase(x);
			return true;
		}
		else return false;
	}
	bool Contains(int x)
	{
		//std::lock_guard<std::mutex> aa{ set_lock };
		return 0 != m_set.count(x);

	}
	void print20()
	{
		int count = 20;
		for (auto x : m_set) {
			if (count-- == 0) break;
			std::cout <<x << ", ";
		}
		std::cout << std::endl;
	}
};

struct RESPONSE {
	bool m_bool;
};

enum METHOD {M_ADD, M_REMOVE, M_CONTAINS, M_CLEAR, M_PRINT20};

struct INVOCATION {
	METHOD m_method;
	int x;
};

class SEQOBJECT {
	std::set <int> m_set;
public:
	SEQOBJECT()
	{
	}
	void clear()
	{
		m_set.clear();
	}
	RESPONSE apply(INVOCATION& inv)
	{
		RESPONSE r{ true };
		switch (inv.m_method) {
		case M_ADD:
			r.m_bool = m_set.insert(inv.x).second;
			break;
		case M_REMOVE:
			r.m_bool = (0 != m_set.count(inv.x));
			if (r.m_bool == true)
				m_set.erase(inv.x);
			break;
		case M_CONTAINS:
			r.m_bool = (0 != m_set.count(inv.x));
			break;
		case M_CLEAR:
			m_set.clear();
			break;
		case M_PRINT20: {
			int count = 20;
			for (auto x : m_set) {
				if (count-- == 0) break;
				std::cout << x << ", ";
			}
			std::cout << std::endl;
		}
					  break;
		}
		return r;
	}
};

class STD_SEQ_SET {
	SEQOBJECT m_set;
public:
	STD_SEQ_SET()
	{
	}
	void clear()
	{
		INVOCATION inv { M_CLEAR, 0 };
		m_set.apply(inv);
	}
	bool Add(int x)
	{
		INVOCATION inv{ M_ADD, x };
		return m_set.apply(inv).m_bool;
	}
	bool Remove(int x)
	{
		INVOCATION inv{ M_REMOVE, x };
		return m_set.apply(inv).m_bool;
	}
	bool Contains(int x)
	{
		INVOCATION inv{ M_CONTAINS, x };
		return m_set.apply(inv).m_bool;

	}
	void print20()
	{
		INVOCATION inv{ M_PRINT20, 0 };
		m_set.apply(inv);
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
	U_NODE* get_max_head()
	{
		U_NODE* h = m_head[0];
		for (int i = 1; i < MAX_THREADS; ++i)
			if (h->m_seq < m_head[i]->m_seq)
				h = m_head[i];
		return h;
	}
public : 
	LFUNV_OBJECT() {
		tail.m_seq = 0;
		tail.next = nullptr;
		for (auto& h : m_head) h = &tail;
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
		for (auto& h : m_head) h = &tail;
	}
	RESPONSE apply(INVOCATION& inv)
	{
		U_NODE* prefer = new U_NODE{ inv, 0, nullptr };
		while (0 == prefer->m_seq) {
			U_NODE* head = get_max_head();
			long long temp = 0;
			std::atomic_compare_exchange_strong(
				reinterpret_cast<volatile std::atomic_llong *>(&head->next), 
				&temp, 
				reinterpret_cast<long long>(prefer));
			U_NODE *after = head->next;
			after->m_seq = head->m_seq + 1;
			m_head[thread_id] = after;
		}

		SEQOBJECT std_set;
		U_NODE* p = tail.next;
		while (p != prefer) {
			std_set.apply(p->m_inv);
			p = p->next;
		}
		return std_set.apply(inv);
	}
};

class STD_LF_SET {
	LFUNV_OBJECT m_set;
public:
	STD_LF_SET()
	{
	}
	void clear()
	{
		//INVOCATION inv{ M_CLEAR, 0 };
		//m_set.apply(inv);
		m_set.clear();
	}
	bool Add(int x)
	{
		INVOCATION inv{ M_ADD, x };
		return m_set.apply(inv).m_bool;
	}
	bool Remove(int x)
	{
		INVOCATION inv{ M_REMOVE, x };
		return m_set.apply(inv).m_bool;
	}
	bool Contains(int x)
	{
		INVOCATION inv{ M_CONTAINS, x };
		return m_set.apply(inv).m_bool;

	}
	void print20()
	{
		INVOCATION inv{ M_PRINT20, 0 };
		m_set.apply(inv);
	}
};

constexpr int MAX_TOP = 9;

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

L_SK_SET my_set;

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
	while (false == m_free_queue.empty()) {
		auto p = m_free_queue.front();
		m_free_queue.pop();
		delete p;
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
	while (false == m_free_queue.empty()) {
		auto p = m_free_queue.front();
		m_free_queue.pop();
		delete p;
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

	for (int n = 1; n <= MAX_THREADS; n = n * 2 ) {
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