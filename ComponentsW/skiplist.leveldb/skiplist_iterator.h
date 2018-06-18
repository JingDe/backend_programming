#ifndef SKIPLIST_ITERATOR_H_
#define SKIPLIST_ITERATOR_H_

#include"common/arena.h"

#include<cassert>

template<typename T, typename Comparator>// Comparator����Ҫ����̳й�ϵ��ֻҪ�����compara����
class SkipList
{
private:
	struct Node;

public:
	explicit SkipList(int max_level, Comparator cmp, Arena* arena);


	void Insert(const T& key);
	void Remove(const T& key);
	bool Contains(const T& key);
	void Display();

	// for test
	int NodeSize() {
		return sizeof(Node);
	}

	class Iterator {
	public:
		explicit Iterator(const SkipList* list);
		bool Valid() const;
		const T& key() const;
		void Next();
		void Prev();
		void Seek(const T& target);
		void SeekToFirst();
		void SeekToLast();

	private:
		const SkipList* list_;
		Node* node_;
	};

private:
	int RandomLevel();
	Node * NewNode(const T& key, int level);
	int Compare(const T& a, const T& b);

	// TODO:����FindGreaterOrEqualʵ���޸ģ�������pre���飬ʹ��һ����������������·��
	bool FindPre(const T& key, Node** pre, int* level); // Ѱ��key��ǰ���ڵ��key����level����key��������level��Χ[1, MAXLEVEL]

	Node* FindLessThan(const T& key) const; // Ѱ��key֮ǰ�Ľ�㣬��head_ʱ����NULL
	Node* FindGreaterOrEqual(const T& key, Node** prev) const; // ���أ���һ�㣩���ڻ����key�������㣬ͨ��pre����key��ÿһ���ǰ�����
	bool KeyIsAfterNode(const T& key, Node* n) const; // �ж�key�Ƿ���ڽ��n��key
	Node* FindLast() const;

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
	T const key;
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
void SkipList<T, Comparator>::Remove(const T& key)
{
	Node** pre = new Node*[max_height_];
	int level = 0;

	if (FindPre(key, pre, &level) == false)
		return;

	for (int l = level - 1; l >= 0; l--)
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
void SkipList<T, Comparator>::Insert(const T& key)
{
	int level = RandomLevel();
	int l;

	// Ѱ��[0, level-1]����������ǰ���ڵ�
	Node** pre = new Node*[max_height_];

	//for (l = level - 1; l >= 0; l--)
	//{
	//	SkipList::Node* p = (l == level - 1) ? head_ : pre[l + 1];
	//	//while (p->next_[l] != nullptr && Compare(p->next_[l]->key, key)<0) //comparator_(p->next[l]->key, key)<0 //p->next[l]->key < key // Compare(p->next[l]->key, key)<0
	//	while (p->Next(l) != nullptr && Compare(p->Next(l)->key, key)<0)
	//		p = p->Next(l);
	//	pre[l] = p;
	//}
	int dummy;
	bool exist = FindPre(key, pre, &dummy);
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
bool SkipList<T, Comparator>::Contains(const T& key)
{
	int dummy = 0;
	return FindPre(key, nullptr, &dummy);
}

// Ѱ��key��ǰ���ڵ��key����level����key��������level��Χ[1, MAXLEVEL]
// ��preΪ�ǿ�ָ��ʱ�����÷�����pre���ͷ�
// ��key���������У�level����key����level����������true�����򷵻�false
template<typename T, typename Comparator>
bool SkipList<T, Comparator>::FindPre(const T& key, typename SkipList<T, Comparator>::Node** pre, int* level)
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
				*level = l + 1;
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

template<typename T, class Comparator>
typename SkipList<T, Comparator>::Node* SkipList<T, Comparator>::FindLast() const
{
	int level = max_height_-1;
	Node* x = head_;
	while (true)
	{
		Node* next = x->Next(level);
		if (next == NULL)
		{
			if (level == 0)
				return x;
			else
				level--;
		}
		else
			x = next;
	}
}

// Ѱ�ң���һ�㣩key֮ǰ�Ľ�㣬��head_ʱ����NULL
template<typename T, class Comparator>
typename SkipList<T, Comparator>::Node* SkipList<T, Comparator>::FindLessThan(const T& key) const
{
	Node* x = head_;
	int level = max_height_ - 1;
	while (true)
	{
		assert(x == head_ || comparator_(x->key, key) < 0);
		Node* next = x->Next(level);
		if (next == NULL || comparator_(next->key, key) >= 0)
		{
			// if(prev)
			//		prev[level]=x;
			if (level == 0)
				return x;
			else
				level--;
		}
		else
			x = next;
	}
}

// ���أ���һ�㣩���ڻ����key�������㣬ͨ��pre����key��ÿһ���ǰ�����
template<typename T, class Comparator>
typename SkipList<T, Comparator>::Node* SkipList<T, Comparator>::FindGreaterOrEqual(const T& key, Node** prev) const
{
	Node* x = head_; 
	int level = max_height_ - 1;
	while (true)
	{
		Node* next = x->Next(level);
		if (KeyIsAfterNode(key, next))
			x = next;
		else
		{
			if (prev)
				prev[level] = x;
			if (level == 0)
				return next;
			else
				level--;
		}
	}
}

template<typename T, typename Comparator>
//bool SkipList<T, Comparator>::KeyIsAfterNode(const T& key, Node* n) const
//{
//	return (n!=NULL)  &&  (comparator_(key, n->key) > 0);
//}
bool SkipList<T, Comparator>::KeyIsAfterNode(const T& key, Node* n) const {
	// NULL n is considered infinite
	
	return (n != NULL) && (comparator_(n->key, key) < 0);
}

template<typename T, class Comparator>
inline SkipList<T, Comparator>::Iterator::Iterator(const SkipList* list)
{
	list_ = list;
	node_ = NULL;
}

template<typename T, class Comparator>
inline bool SkipList<T, Comparator>::Iterator::Valid() const
{
	return node_ != NULL;
}

template<typename T, class Comparator>
inline const T& SkipList<T, Comparator>::Iterator::key() const {
	assert(Valid());
	return node_->key;
}

template<typename T, class Comparator>
inline void SkipList<T, Comparator>::Iterator::Next() {
	assert(Valid());
	node_ = node_->Next(0);
}

template<typename T, class Comparator>
inline void SkipList<T, Comparator>::Iterator::Prev()
{
	assert(Valid());
	node_ = list_->FindLessThan(node_->key);
	if (node_ == list_->head_)
		node_ = NULL;
}

template<typename T, class Comparator>
inline void SkipList<T, Comparator>::Iterator::Seek(const T& target)
{
	node_ = list_->FindGreaterOrEqual(target, NULL);
}

template<typename T, typename Comparator>
inline void SkipList<T, Comparator>::Iterator::SeekToFirst()
{
	node_ = list_->head_->Next(0);
}

template<typename T, typename Comparator>
inline void SkipList<T, Comparator>::Iterator::SeekToLast()
{
	node_ = list_->FindLast();
	if (node_ == list_->head_)
		node_ = NULL;
}

#endif