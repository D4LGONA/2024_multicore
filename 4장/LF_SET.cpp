class SPTR 
{
	std::atomic<long long> sptr;
public:
	SPTR() : sptr(0) {}
	LFNODE *get_ptr()
        {
             return reinterpret_cast<LFNODE *>(sptr & 0xFFFFFFFFFFFFFFFE;
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
             long long new_v = retinterpret_cast<long long>(new_p);
             if (true == new_m) new_v = new_v | 1;
             else { std::cout << "POINTER ERROR!!\n"; exit(-1); }
             return std::atomic_compare_exchange(&sptr, &old_v, new_v);
        }
}
class LFNODE
{
	int key;
	SPTR next;
public:
	LFNODE(int v) : key(v) {}
};

class LF_SET()
{
public:
void find(LFNODE *&prev, LFNODE *&curr, int x)
{
   while (true) {
   retry:
       prev = &head;
       curr = prev->next;
 
      while (true) {
      // 청소
      bool removed = false;
      do {
      LFNODE *succ = curr->next.get_ptr(&removed);
      if (removed == true) {
           if (false == prev->next.CAS(curr, succ, false, false)) goto retry;
           curr = succ;
      }
      } while (removed == false);
  
       while (curr->key >= x) return;
       prev = curr;
       curr = curr->next;
      }
   }
}
















}