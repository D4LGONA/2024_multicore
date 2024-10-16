#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <array>
#include <atomic>

class SPTR
{
    std::atomic<long long> sptr;
public:
    SPTR() : sptr(0) {}
    LFNODE* get_ptr()
    {
        return reinterpret_cast<LFNODE*>(sptr & 0xFFFFFFFFFFFFFFFE);
    }
    bool get_removed()
    {
        return 1 == (sptr & 1);
    }
    bool CAS(LFNODE* old_p, LFNODE* new_p, bool old_m, bool new_m)
    {
        long long old_v = reinterpret_cast<long long>(old_p);
        if (true == old_m) old_v = old_v | 1;
        else { std::cout << "POINTER ERROR!!\n"; exit(-1); }
        long long new_v = reinterpret_cast<long long>(new_p);
        if (true == new_m) new_v = new_v | 1;
        else { std::cout << "POINTER ERROR!!\n"; exit(-1); }
        return std::atomic_compare_exchange_strong(&sptr, &old_v, new_v);
    }
};

class LFNODE
{
public:
    int key;
    SPTR next;
    LFNODE(int v) : key(v) {}
};

class LF_SET
{
    LFNODE head{ (int)(0x80000000) }, tail{ (int)(0x7FFFFFFF) };
public:
    LF_SET()
    {
        head.next.CAS(head.next.get_ptr(), &tail, false, false);
    }
    void clear()
    {
        LFNODE* curr = head.next.get_ptr();
        while (curr != &tail) {
            LFNODE* next = curr->next.get_ptr();
            delete curr;
            curr = next;
        }

        while (!head.next.CAS(head.next.get_ptr(), &tail, false, false)) {
        }
    }
    bool validate(const int x, LFNODE* prev, LFNODE* curr)
    {
        return !prev->next.get_removed() && !curr->next.get_removed() && (curr == prev->next.get_ptr());
    }

    bool Add(int x)
    {
        while (true) {
            LFNODE* prev;
            LFNODE* curr;
            find(prev, curr, x);

            if (curr->key == x) {
                return false; // 이미 키가 존재함
            }
            else {
                LFNODE* newNode = new LFNODE(x);
                newNode->next.CAS(nullptr, curr, false, false); // newNode의 next를 curr로 설정
                if (prev->next.CAS(curr, newNode, false, false)) {
                    return true; // 성공적으로 추가
                }
                else {
                    delete newNode; // 실패시 메모리 해제
                }
            }
        }
    }
    bool Remove(int x) {
        while (true) {
            LFNODE* prev;
            LFNODE* curr;
            find(prev, curr, x);

            if (curr->key != x) {
                return false; // 키가 존재하지 않음
            }

            LFNODE* succ = curr->next.get_ptr();
            if (!curr->next.CAS(succ, succ, false, true)) { // 마킹 시도
                continue; // 마킹 실패시 재시도
            }

            // 마킹에 성공했으면 성공 반환, prev->next를 갱신하는 것은 시도하지만 실패해도 무방
            prev->next.CAS(curr, succ, false, false);
            return true;
        }
    }

    bool Contains(int x) {
        LFNODE* prev;
        LFNODE* curr;
        find(prev, curr, x); // find 함수를 사용하여 적절한 위치를 찾음

        return curr != &tail && curr->key == x && !curr->next.get_removed(); // 마킹 상태 확인
    }

    void print20()
    {
        auto p = head.next.get_ptr();
        for (int i = 0; i < 20; ++i) {
            if (p == &tail) break;
            std::cout << p->key << ", ";
            p = p->next.get_ptr();
        }
        std::cout << std::endl;
    }

    void find(LFNODE*& prev, LFNODE*& curr, int x)
    {
        while (true) {
        retry:
            prev = &head;
            curr = prev->next.get_ptr();

            while (true) {
                // 청소
                bool removed = false;
                do {
                    LFNODE* succ = curr->next.get_ptr();
                    if (removed == true) {
                        if (false == prev->next.CAS(curr, succ, false, false)) goto retry;
                        curr = succ;
                    }
                } while (removed == false);

                while (curr->key >= x) return;
                prev = curr;
                curr = curr->next.get_ptr();
            }
        }
    }
};

LF_SET my_set;

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

    std::cout << "------ 검사 ------" << std::endl;
    for (int n = 1; n <= 32; n = n * 2) {
        my_set.clear();
        for (auto& v : history)
            v.clear();
        std::vector<std::thread> tv;
        auto start_t = high_resolution_clock::now();
        for (int i = 0; i < n; ++i) {
            tv.emplace_back(worker_check<LF_SET>, n, i, std::ref(my_set));
        }
        for (auto& th : tv)
            th.join();
        auto end_t = high_resolution_clock::now();
        auto exec_t = end_t - start_t;
        size_t ms = duration_cast<milliseconds>(exec_t).count();
        std::cout << n << " Threads,  " << ms << "ms.";
        my_set.print20();
        check_history(n, my_set);
    }

    // 성능 체크
    std::cout << "------ 성능체크 ------" << std::endl;
    for (int n = 1; n <= 32; n = n * 2) {
        my_set.clear();
        std::vector<std::thread> tv;
        auto start_t = high_resolution_clock::now();
        for (int i = 0; i < n; ++i) {
            tv.emplace_back(benchmark<LF_SET>, n, std::ref(my_set));
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