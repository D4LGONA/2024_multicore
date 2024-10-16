#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <array>
#include <atomic>

class LFNODE;

class SPTR 
{
	std::atomic<long long> sptr;
public:
	SPTR() : sptr(0) {}
	LFNODE *get_ptr(bool removed = false)
        {
            return reinterpret_cast<LFNODE *>(sptr & 0xFFFFFFFFFFFFFFFE);
        }
        bool *get_removed()
        {
             return (1 == (sptr & 1));
        }
        bool CAS(LFNODE *old_p, LFNODE *new_p, bool old_m, bool new_m)
        {
             long long old_v = reinterpret_cast<long long>(old_p);
             if (true == old_m) old_v = old_v | 1;
             else { std::cout << "POINTER ERROR!!\n"; exit(-1); }
             long long new_v = reinterpret_cast<long long>(new_p);
             if (true == new_m) new_v = new_v | 1;
             else { std::cout << "POINTER ERROR!!\n"; exit(-1); }
             return std::atomic_compare_exchange_strong(&sptr, &old_v, new_v);
        }
}
class LFNODE
{
public:
	int key;
	SPTR next;
	LFNODE(int v) : key(v) {}
};

class LF_SET
{
    LFNODE head, tail;
public:
    LF_SET()
    {
        head.next.set_ptr(&tail);
    }

    void clear()
    {
        while (head.next.get_ptr() != &tail) {
            // 전진시키면서 delete
        }
    }
    void find(LFNODE *&prev, LFNODE *&curr, int x)
{
   while (true) {
   retry:
       prev = &head;
       curr = prev->next.get_ptr();
 
      while (true) {
      // 청소
      bool removed = false;
      do {
      LFNODE *succ = curr->next.get_ptr(&removed); // ptr을 가져오면서 removed 값도 가져오는 것..?
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


    bool add() {}
        // p.next.set_ptr(curr)
        // if (true == ptev->next.cas(curr,p,false, false) return true;


    bool remove(){}
    // 있을 때
    // succ = curr.next.get_ptr
    // 1. cas 한번으로 마킹하기(succ, succ, false, true) : 실패시 continue(처음부터 다시)
    // 2. 두번째 cas로 삭제하려는 시도하기: 실패해도 그냥 리턴.

    // contains는 그대로.









}