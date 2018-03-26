#ifndef SKIPLIST_H_
#define SKIPLIST_H_

#include"common/arena.h"

#include<cassert>

template<typename T, typename Comparator>// Comparator����Ҫ����̳й�ϵ��ֻҪ�����compara����
class SkipList
{
private:
	struct Node;

public:
	explicit SkipList(int max_level, Comparator cmp, Arena* arena);


	void Insert(T key);
	void Remove(T key);
	bool Contains(T key);
	void Display();

	// for test
	int NodeSize() {
		return sizeof(Node);
	}

private:
	int RandomLevel();
	Node * NewNode(const T& key, int level);
	int Compare(const T& a, const T& b);
	bool FindPre(T key, Node** pre, int* level);

	Node * head_;
	
	Arena* const arena_; // ָ�벻�䣬ָ��̶���ַ��Arena��Arena���Ըı�

	Comparator const comparator_;// �Ƚ϶���
								 
	const int max_height_; // ����������������ǳ������ʽ
	//enum { max_height_ = 12 }; // �ǳ������ʽ

	SkipList(const SkipList&);
	void operator=(const SkipList&);
};


template<typename T, typename Comparator>
struct SkipList<T, Comparator>::Node {
	T key;
	int level;

	port::AtomicPointer next_[1]; // void*
	//SkipList::Node* next_[1];
	//SkipList::Node** next_;

	explicit Node(const T& k) :key(k) {}
	Node(const T& k, int l) :key(k), level(l) {}

	Node* Next(int n)
	{
		assert(n >= 0);
		return reinterpret_cast<Node*>(next_[n].Acquire_Load());
	}

	void SetNext(int n, Node* x)
	{
		assert(n >= 0);
		next_[n].Release_Store(x);
	}

	Node* NoBarrier_Next(int n)
	{
		assert(n >= 0);
		return reinterpret_cast<Node*>(next_[n].NoBarrier_Load());
	}

	void NoBarrier_SetNext(int n, Node* x)
	{
		assert(n >= 0);
		next_[n].NoBarrier_Store(x);
	}
};



template<typename T, typename Comparator>
typename SkipList<T, Comparator>::Node* SkipList<T, Comparator>::NewNode(const T& key, int level)
{
	//char* p = new char[sizeof(Node) + sizeof(Node*) * (level - 1)];
	char* p = arena_->AllocateAligned(sizeof(Node) + sizeof(port::AtomicPointer)*(level - 1));
	return new (p) Node(key, level); // ����ʱ�����ͷ�p����ʹ��Arena����
	/*Node* result = new Node(key);
	result->level = level;
	result->next_ = new Node*[level];
	return result;*/
}

// ����ֵȡֵ[1, max_height_]
template<typename T, typename Comparator>
int SkipList<T, Comparator>::RandomLevel()
{
	int level = 1;
	while (rand() % 2 && level < max_height_)
		level++;
	return level;
}

template<typename T, typename Comparator>
SkipList<T, Comparator>::SkipList(int max_level, Comparator cmp, Arena* arena)
	:max_height_(max_level),
	comparator_(cmp),
	arena_(arena)
{
	head_ = NewNode(0, max_level); // 0
	for (int i = 0; i < max_height_; i++)
		head_->SetNext(i, NULL);
}



template<typename T, typename Comparator>
int SkipList<T, Comparator>::Compare(const T& a, const T& b)
{
	//return comparator_.compare(a, b);
	return comparator_(a, b);
}

template<typename T, typename Comparator>
void SkipList<T, Comparator>::Remove(T key)
{
	Node** pre=new Node*[max_height_];
	int level = 0;

	if (FindPre(key, pre, &level) == false)
		return;

	for (int l = level-1; l >= 0; l--)
	{
		//pre[l]->next[l] = pre[l]->next[l]->next[l];
		pre[l]->SetNext(l, pre[l]->Next(l)->Next(l));
	}
	// ��Arena�����ͷ�
	//delete[] pre[0]->next[0]->next_;
	//delete pre[0]->next[0];

	delete[]pre;
}

template<typename T, typename Comparator>
void SkipList<T, Comparator>::Insert(T key)
{
	int level = RandomLevel();
	int l;

	// Ѱ��[0, level-1]����������ǰ���ڵ�
	Node** pre=new Node*[max_height_];
	
	//for (l = level - 1; l >= 0; l--)
	//{
	//	SkipList::Node* p = (l == level - 1) ? head_ : pre[l + 1];
	//	//while (p->next_[l] != nullptr && Compare(p->next_[l]->key, key)<0) //comparator_(p->next[l]->key, key)<0 //p->next[l]->key < key // Compare(p->next[l]->key, key)<0
	//	while (p->Next(l) != nullptr && Compare(p->Next(l)->key, key)<0)
	//		p = p->Next(l);
	//	pre[l] = p;
	//}
	int dummy;
	bool exist=FindPre(key, pre, &dummy);
	// �����������ͬ��Ԫ��
	if (exist)
		return;

	// �����½ڵ㣬����[0, level-1]��
	Node* newnode = NewNode(key, level);
	for (l = level - 1; l >= 0; l--)
	{
		//newnode->next_[l] = pre[l]->next_[l];
		newnode->SetNext(l, pre[l]->Next(l));
		//pre[l]->next_[l] = newnode;
		pre[l]->SetNext(l, newnode);
	}
	delete[]pre;
}





template<typename T, typename Comparator>
bool SkipList<T, Comparator>::Contains(T key)
{
	int dummy = 0;
	return FindPre(key, nullptr, &dummy);
}

// Ѱ��key��ǰ���ڵ������level����key��������level��Χ[1, MAXLEVEL]
// ��preΪ�ǿ�ָ��ʱ�����÷�����pre���ͷ�
// ��key���������У�level����key����level����������true�����򷵻�false
template<typename T, typename Comparator>
bool SkipList<T, Comparator>::FindPre(T key, typename SkipList<T, Comparator>::Node** pre, int* level)
{
	bool rm = false;
	if (pre == nullptr)
	{
		pre = new Node*[max_height_];
		rm = true;
	}
	*level = 0;
	for (int l = max_height_ - 1; l >= 0; l--)
	{
		Node* p = (l == max_height_ - 1) ? head_ : pre[l + 1];
		while (p->Next(l) != nullptr && p->Next(l)->key < key)
			p = p->Next(l);
		if (p->Next(l) != nullptr && p->Next(l)->key == key)
		{
			if (*level == 0)
				*level = l+1;
		}
		pre[l] = p;
	}

	if (rm)
		delete[] pre;

	return *level != 0;
}

template<typename T, typename Comparator>
void SkipList<T, Comparator>::Display()
{
	printf("SkipList: ");
	for (Node* p = head_->Next(0); p != nullptr; p = p->Next(0))
	{
		printf("(%d, %d) ", p->key, p->level);
	}
	printf("\n");
}
#endif
