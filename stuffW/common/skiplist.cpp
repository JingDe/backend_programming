#include"skiplist.h"

template<typename T, typename Comparator>
Node* SkipList<T, Comparator>::NewNode(T key, int level)
{
	char* p = new char[sizeof(Node*) * (level-1)];
	return new (p) Node(key);
}

template<typename T, typename Comparator>
SkipList<T, Comparator>::SkipList(int max_level):MAXLEVEL(max_level)
{
	head_ = NewNode(0, max_level); // 0
	tail_ = NewNode(0, max_level);	
	for (int i = 0; i < MAXLEVEL; i++)
	{
		head->next_[i] = tail_;
		tail_->next_[i] = NULL;
	}
}

template<typename T, typename Comparator>
SkipList<T, Comparator>::~SkipList()
{
	Node* p=head_;
	Node* q;
	while (p != NULL)
	{
		q = p->next_[0];
		delete[]p->next_;
		delete p;
		p = q;
	}
}

template<typename T, typename Comparator>
void SkipList<T, Comparator>::Insert(T key)
{

}