//#include <iostream>
//#include <thread>
//#include <vector>
//#include <chrono>
//#include <mutex>
//using namespace std;
//
//class NODE {
//public: 
//	int key;
//	NODE* volatile next; // 포인터 자체를 volatil으로 만듦?
//
//	NODE(int x) : key(x), next(nullptr) { }
//};
//
//class C_SET {
//	NODE head{ INT_MIN }, tail{ (int)(0x7FFFFFFF) };
//	mutex set_lock;
//public:
//	C_SET() 
//	{
//		auto a = &tail;
//		head.next = &tail;
//	}
//
//	void Clear()
//	{
//		while (head.next != &tail) {
//			auto p = head.next;
//			head.next = p->next;
//			delete p;
//		}
//	}
//
//	void Print20()
//	{
//		auto p = head.next;
//		for (int i = 0; i < 20; ++i) {
//			if (p == &tail) break;
//			cout << p->key << ", ";
//		}
//		cout << endl;
//	}
//
//	bool Add(int x) 
//	{
//		NODE* n_node = new NODE(x);
//		auto p = &head;
//
//		if (Contains(x)) {
//			return false;
//		}
//
//		set_lock.lock();
//		while (true) {
//			if (p->key < n_node->key and p->next->key > n_node->key) {
//				n_node->next = p->next;
//				p->next = n_node;
//				set_lock.unlock();
//				return true;
//			}
//			p = p->next; // 왜안됨?
//		}
//	}
//
//	bool Remove(int x)
//	{
//		auto p = &head;
//		if (Contains(x)) {
//			return false;
//		}
//
//		set_lock.lock();
//		while (true) {
//			if (p->next->key == x) {
//				auto target = p->next;
//				p->next = target->next;
//				delete target;
//
//				set_lock.unlock();
//				return true;
//			}
//			p = p->next;
//		}
//	}
//
//	bool Contains(int x)
//	{
//		set_lock.lock();
//		auto p = &head;
//		while (p->next->key != INT_MIN) {
//			if (p->key == x) {
//				set_lock.unlock();
//				return true;
//			}
//			p = p->next;
//		}
//		set_lock.unlock();
//		return false;
//	}
//};
//
//C_SET my_set;
//const int NUM_TEST = 4000000;
//const int KEY_RANGE = 1000;
//
//void benchmark(const int num_thread)
//{
//	int key;
//
//	for (int i = 0; i < NUM_TEST / num_thread; i++) {
//		switch (rand() % 3) {
//		case 0: key = rand() % KEY_RANGE;
//			my_set.Add(key);
//			break;
//		case 1: key = rand() % KEY_RANGE;
//			my_set.Remove(key);
//			break;
//		case 2: key = rand() % KEY_RANGE;
//			my_set.Contains(key);
//			break;
//		default: cout << "Error\n";
//			exit(-1);
//		}
//	}
//}
//
//int main()
//{
//	using namespace chrono;
//
//	for (int i = 1; i <= 16; i = i * 2) {
//		vector <thread> th;
//		my_set.Clear();
//		auto start = high_resolution_clock::now();
//
//		for (int j = 0; j < i; ++j)
//			th.emplace_back(benchmark, i);
//
//		for (auto& t : th)
//			t.join();
//
//		auto du = duration_cast<milliseconds>(high_resolution_clock::now() - start).count();
//
//		cout << i << "threads: " << du << "ms" << endl;
//
//		my_set.Print20();
//	}
//}