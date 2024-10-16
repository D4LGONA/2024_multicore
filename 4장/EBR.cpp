#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <array>
#include <atomic>
#include <queue>

constexpr int MAX_THREAD = 16;

class LFNODE;

class SPTR
{
    std::atomic<long long> sptr;
public:
    SPTR() : sptr(0) {}

    void set_ptr_and_mark(LFNODE* new_p, bool mark)
    {
        long long value = reinterpret_cast<long long>(new_p);
        if (mark) {
            value |= 1;
        }
        sptr = value;
    }

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
        if (old_m) old_v |= 1;
        long long new_v = reinterpret_cast<long long>(new_p);
        if (new_m) new_v |= 1;
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

class LF_SET
{
    LFNODE head{ (int)(0x80000000) }, tail{ (int)(0x7FFFFFFF) };

public:
    LF_SET()
    {
        // head.next�� tail�� �ʱ�ȭ (��ŷ���� ���� ���·�)
        head.next.set_ptr_and_mark(&tail, false);
    }

    void clear()
    {
        LFNODE* curr = head.next.get_ptr();
        while (curr != &tail) {
            LFNODE* next = curr->next.get_ptr();
            delete curr;
            curr = next;
        }
        // head.next�� �ٽ� tail�� �ʱ�ȭ
        head.next.set_ptr_and_mark(&tail, false);
    }

    bool Add(int x)
    {
        while (true) {
            LFNODE* prev;
            LFNODE* curr;
            find(prev, curr, x);

            if (curr->key == x) {
                return false; // �̹� Ű�� ������
            }
            else {
                LFNODE* newNode = new LFNODE(x);
                newNode->next.set_ptr_and_mark(curr, false); // ���ο� ����� next�� ���� ��ġ�� ����
                if (prev->next.CAS(curr, newNode, false, false)) {
                    return true; // ���������� �߰�
                }
                else {
                    delete newNode; // ���н� �޸� ����
                }
            }
        }
    }

    bool Remove(int x)
    {
        while (true) {
            LFNODE* prev;
            LFNODE* curr;
            find(prev, curr, x);

            if (curr->key != x) {
                return false; // Ű�� �������� ����
            }

            LFNODE* succ = curr->next.get_ptr();
            if (!curr->next.CAS(succ, succ, false, true)) { // ��ŷ �õ�
                continue; // ��ŷ ���н� ��õ�
            }

            // ��ŷ�� ���������� �ٷ� ���� ��ȯ. ������ ������ �õ������� �����ص� ����
            prev->next.CAS(curr, succ, false, false);
            return true;
        }
    }

    bool Contains(int x)
    {
        LFNODE* prev;
        LFNODE* curr;
        find(prev, curr, x);

        return curr != &tail && curr->key == x && !curr->next.get_removed();
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

    void find(LFNODE*& prev, LFNODE*& curr, int x)
    {
        while (true) {
        retry:
            prev = &head;
            curr = prev->next.get_ptr();

            while (true) {
                bool removed = curr->next.get_removed();
                LFNODE* succ = curr->next.get_ptr();

                if (removed) {
                    if (!prev->next.CAS(curr, succ, false, false)) goto retry;
                    curr = succ;
                }
                else {
                    if (curr->key >= x) return;
                    prev = curr;
                    curr = curr->next.get_ptr();
                }
            }
        }
    }
};

thread_local std::queue<LFNODE*> m_free_queue; // �����帶�� ������ ť
// ó������ �������;��ϱ� ����
thread_local int thread_id; // ���� ������ �ֵ��� �ض�.

// ��Ȱ���� ���� ����:
class EBR
{
    std::atomic_int epoch_counter;
    std::atomic_int epoch_array[MAX_THREAD * 16]; // ĳ���浹.
public:
    EBR(): epoch_counter(1) { } // 0�� Ư���� �ǹ̷� ���
    ~EBR() 
    {
        clear();
    }
    void clear() // ��� ������ �����մϴ�.
    {
        while (false == m_free_queue.empty())
            m_free_queue.pop(); // queue���� clear�� ���ٴ� ���� ���
        epoch_counter = 1; // �ٽ� 1�� �����. overflow ����
    }
    void reuse(LFNODE* node) // free list�� �ִ� ��
    {
        node->ebr_number = epoch_counter;
        m_free_queue.push(node);
        // plan a: if(m_free_queue.size() > max_free_size)
        // : safe_delete();
        // ����Ǵ� �޸��� �ִ��� �����Ѵ�.
    }

    void start_epoch()
    {
        int epoch = epoch_counter++; // atomic
        epoch_array[thread_id * 16] = epoch;
    }
    void end_epoch()
    {
        epoch_array[thread_id] = 0;
    }
    LFNODE* get_node(int x) // ��带 ���� ����� �ִ� ��
    {
        // plan b: ������ �� new/delete ���� ����
        if (true == m_free_queue.empty())
            return new LFNODE(x);
        LFNODE* p = m_free_queue.front();
        for (int i = 0; i < MAX_THREAD; i += 16) { // ���� �̻���
            if (epoch_array[i] != 0 and epoch_array[i] < p->ebr_number) 
                return new LFNODE(x);
        }
        m_free_queue.pop();
        p->key = x;
        return p;
    }
};

EBR ebr;

class EBR_LF_SET
{
    LFNODE head{ (int)(0x80000000) }, tail{ (int)(0x7FFFFFFF) };

public:
    EBR_LF_SET()
    {
        head.next.set_ptr_and_mark(&tail, false);
    }

    void clear()
    {
        LFNODE* curr = head.next.get_ptr();
        while (curr != &tail) {
            LFNODE* next = curr->next.get_ptr();
            delete curr;
            curr = next;
        }
        head.next.set_ptr_and_mark(&tail, false);
    }

    bool Add(int x)
    {
        ebr.start_epoch(); // ���� method�� �����Ѵٰ� �˸�.
        while (true) {
            LFNODE* prev;
            LFNODE* curr;
            find(prev, curr, x);

            if (curr->key == x) {
                ebr.end_epoch(); // �޼ҵ� ������ ������
                return false; // �̹� Ű�� ������
            }
            else {
                LFNODE* newNode = ebr.get_node(x);
                newNode->next.set_ptr_and_mark(curr, false); // ���ο� ����� next�� ���� ��ġ�� ����
                if (prev->next.CAS(curr, newNode, false, false)) {
                    ebr.end_epoch(); // �޼ҵ� ������ ������: ��� ���Ͽ� �־��ֱ�
                    return true; // ���������� �߰�
                }
                else {
                    delete newNode; // ���н� �޸� ����
                }
            }
        }
    }

    bool Remove(int x)
    {
        ebr.start_epoch();
        while (true) {
            LFNODE* prev;
            LFNODE* curr;
            find(prev, curr, x);

            if (curr->key != x) {
                ebr.end_epoch();
                return false; // Ű�� �������� ����
            }

            LFNODE* succ = curr->next.get_ptr();
            if (!curr->next.CAS(succ, succ, false, true)) { // ��ŷ �õ�
                continue; // ��ŷ ���н� ��õ�
            }

            // ��ŷ�� ���������� �ٷ� ���� ��ȯ. ������ ������ �õ������� �����ص� ����
            if (true == prev->next.CAS(curr, succ, false, false))
                ebr.reuse(curr);
            ebr.end_epoch();
            return true;
        }
    }

    bool Contains(int x)
    {
        ebr.start_epoch();
        LFNODE* prev;
        LFNODE* curr;
        find(prev, curr, x);

        bool res = (curr != &tail && curr->key == x && !curr->next.get_removed());
        ebr.start_epoch();
        return res;
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

    void find(LFNODE*& prev, LFNODE*& curr, int x)
    {
        while (true) {
        retry:
            prev = &head;
            curr = prev->next.get_ptr();

            while (true) {
                bool removed = curr->next.get_removed();
                LFNODE* succ = curr->next.get_ptr();

                if (removed) {
                    if (!prev->next.CAS(curr, succ, false, false)) goto retry;
                    ebr.reuse(curr);
                    curr = succ;
                }
                else {
                    if (curr->key >= x) return;
                    prev = curr;
                    curr = curr->next.get_ptr();
                }
            }
        }
    }
};

EBR_LF_SET my_set;

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
void benchmark(int num_thread,const int th_id, T& my_set)
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

    std::cout << "------ �˻� ------" << std::endl;
    for (int n = 1; n <= MAX_THREAD; n *= 2) {
        my_set.clear();
        for (auto& v : history) v.clear();
        std::vector<std::thread> tv;
        auto start_t = high_resolution_clock::now();
        for (int i = 0; i < n; ++i) {
            tv.emplace_back(worker_check<EBR_LF_SET>, n, i, std::ref(my_set));
        }
        for (auto& th : tv) th.join();
        auto end_t = high_resolution_clock::now();
        std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms.";
        my_set.print20();
        check_history(n, my_set);
    }

    std::cout << "------ ���� üũ ------" << std::endl;
    for (int n = 1; n <= MAX_THREAD; n *= 2) {
        my_set.clear();
        std::vector<std::thread> tv;
        auto start_t = high_resolution_clock::now();
        for (int i = 0; i < n; ++i) {
            tv.emplace_back(benchmark<EBR_LF_SET>, n, i, std::ref(my_set));
        }
        for (auto& th : tv) th.join();
        auto end_t = high_resolution_clock::now();
        std::cout << n << " Threads, " << duration_cast<milliseconds>(end_t - start_t).count() << "ms.";
        my_set.print20();
    }
}
