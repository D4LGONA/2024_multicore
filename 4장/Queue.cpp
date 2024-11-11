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
};  // ��带 ��ŷ�� �ʿ�� x

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
			if (v == -1) break; // ����ٴ� ��
			std::cout << v << ", ";
		}
		std::cout << std::endl;
	}
};

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
			if (nullptr == next) return -1; // empty queue
			if (first == last) {
				CAS(&tail, last, next);
				continue;
			}
			int value = first->key;
			if (false == CAS(&head, first, next)) continue;
			delete first;
			// delete�� �ϸ� �ȵǴ� ����: delete�� ��带 ������ �ٶ󺸴ϱ�
			// 
			return value;
		}
	}
	void print20()
	{
		for (int i = 0; i < 20; ++i) {
			int v = Dequeue();
			if (v == -1) break; // ����ٴ� ��
			std::cout << v << ", ";
		}
		std::cout << std::endl;
	}
};

class STPTR
{
public:
	std::atomic_llong m_stptr;
	STPTR(STNODE* p, int st)
	{
		set_ptr(p, st);
	}
	STPTR(const STPTR& new_v)
	{
		m_stptr.store(new_v.m_stptr.load());
	}
	void set_ptr(STNODE* p, int st)
	{
		long long t = reinterpret_cast<unsigned int>(p);
		m_stptr = (t << 32) + st;
	}

	STNODE* get_ptr()
	{
		return reinterpret_cast<STNODE*> (m_stptr >> 32);
	}
	int get_stamp()
	{
		return m_stptr & 0xFFFFFFFF; // 32��Ʈ�� ����..?
	}


	bool CAS(STNODE* old_ptr, STNODE* new_ptr, int old_st, int new_st)
	{
		long long old_v = reinterpret_cast<unsigned int>(old_ptr);
		old_v = (old_v << 32) + old_st;
		long long new_v = reinterpret_cast<unsigned int>(new_ptr);
		new_v = (new_v << 32) + new_st;
		return std::atomic_compare_exchange_strong(&m_stptr, &old_v, new_v);
	}
};

class STNODE
{
public:
	int	key;
	STPTR next;
	STNODE(int x) : key(x), next(nullptr, 0) {}
};  

class ST_LF_QUEUE {
	STPTR head{nullptr, 0}, tail{ nullptr, 0 };
public:
	ST_LF_QUEUE()
	{
		auto n = new STNODE{ -1 };
		head.set_ptr(n, 0);
		tail.set_ptr(n, 0);
	}
	void clear()
	{
		while (-1 != Dequeue());
	}
	
	void Enqueue(int x)
	{
		STNODE* e = new STNODE(x);
		while (true) {
			STPTR last{ tail };
			STPTR next{ tail.get_ptr()->next }; // last�� next�� �ֱ�
			if (last.get_ptr() != tail.get_ptr()) continue; // �ٸ��ְ� �ǵ������.
			// stamp�� ���ؾ� ������ ������ �ڿ��� cas�� ���� �״� ���� ���ص���
			if (nullptr != next.get_ptr()) { // ���� �߰��� ���� �ǵ��������
				tail.CAS(last.get_ptr(), next.get_ptr(), last.get_stamp(), last.get_stamp() + 1);
				continue;
			}
			// ������ ����� next�� ī��.
			if (true == last.get_ptr()->next.CAS(nullptr, e, next.get_stamp(), next.get_stamp() + 1)) {
				tail.CAS(last.get_ptr(), e, last.get_stamp(), last.get_stamp() + 1);
				return;
			}
		}
	}
	int Dequeue()
	{
		while (true) {
			STPTR first{ head };
			STPTR next{ head.get_ptr()->next }; // ���� ��.
			STPTR last{ tail }; // ����..
			if (first.get_ptr() != head.get_ptr()) continue;
			if (nullptr == next.get_ptr()) return -1; // empty queue
			if (first.get_ptr() == last.get_ptr()) { // ť�� ���� �ʾ����� �ֽ��� ���� ������ ���� �ʴ�?
				tail.CAS(last.get_ptr(), next.get_ptr(), last.get_stamp(), last.get_stamp() + 1); // �ֽ��� �� ������ �ϱ�.
				continue;
			}

			if (true == head.CAS(first.get_ptr(), next.get_ptr(), first.get_stamp(), first.get_stamp() + 1)) {
					int value = first.get_ptr()->key; // ���� ���� ���� �˾Ƴ���.
					delete first.get_ptr();
					return value;
			}
		}
	}
	void print20()
	{
		for (int i = 0; i < 20; ++i) {
			int v = Dequeue();
			if (v == -1) break; // ����ٴ� ��
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

	// ����Ȯ��
	std::cout << "------ ���� ------" << std::endl;
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

