#include <iostream>

class SPTR {
	atomic long lonc sptr;
public:
	SPTR() :sptr(0) {}
	LFNODE* get_ptr()
	{

	}
};

class LFNODE
{
	int key;
	SPTR next;
public:
	LFNODE(int v) : key(v) {}
};

class LF_SET 
{
public:
	void find(LFNODE*& prev, LFNODE*& curr, int x) // 찾을 값
	{
		while (true) {
			retry:
			prev = &head;
			curr = prev->next;

			while (true) {
				// 청소
				bool removed = false;
				do {
					LFNODE* succ = curr->next.get_ptr(&removed);
					if (removed == true) {
						if (false == prev->next.CAS(curr, succ, false, false)) goto retry; // 실패시 다시 시작
						curr = succ;
					}
				} while (removed == false)

					while (curr->key >= x) return;
				prev = curr;
				curr = curr->next;
			}
		}
	}
};