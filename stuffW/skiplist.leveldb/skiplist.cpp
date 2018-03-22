#include"skiplist.h"

template<typename T, typename Comparator>
typename SkipList<T, Comparator>::Node* SkipList<T, Comparator>::NewNode(const T& key, int level)
{
	char* p = new char[sizeof(Node*) * (level-1)];
	return new (p) Node(key);
}

template<typename T, typename Comparator>
int SkipList<T, Comparator>::RandomLevel()
{
	int level = 0;
	while (rand() % 2 && level < MAXLEVEL)
		level++;
	return level;
}

template<typename T, typename Comparator>
SkipList<T, Comparator>::SkipList(int max_level, Comparator cmp)
	:MAXLEVEL(max_level),comparator_(cmp)
{
	head_ = NewNode(0, max_level); // 0
	tail_ = NewNode(0, max_level);	
	for (int i = 0; i < MAXLEVEL; i++)
	{
		head_->next_[i] = tail_;
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
int SkipList<T, Comparator>::Compare(const T& a, const T& b)
{
	//return comparator_.compare(a, b);
	return comparator_(a, b);
}

template<typename T, typename Comparator>
void SkipList<T, Comparator>::Insert(T key)
{
	int level = RandomLevel(); // level取值[0, MAXLEVEL]
	int l;

	// 寻找[0, level]层待插入结点的前驱节点
	SkipList::Node** pre = new Node*[level + 1]; // 暂存新节点的前驱结点
	for (l = level; l >= 0; l--)
	{
		SkipList::Node* p = (l==level) ? head_  :  pre[l+1];
		while (p->next[l] != tail_  && Compare(p->next[l]->key, key)<0) //comparator_(p->next[l]->key, key)<0 //p->next[l]->key < key // Compare(p->next[l]->key, key)<0
			p = p->next[l];
		pre[l] = p;
	}

	// 创建新节点，插入[0, level]层
	SkipList<T, Comparator>::Node* newnode = NewNode(key, level);
	for (l = level; l >= 0; l--)
	{
		newnode->next[l] = pre[l]->next[l];
		pre[l]->next[l] = newnode;
	}
}



template<typename T, typename Comparator>
void SkipList<T, Comparator>::Remove(T key)
{
	Node* pre[MAXLEVEL];
	int level = -1;
	
	if (FindPrev(pre, &level) == false)
		return;

	for (int l = level; l >= 0; l--)
	{
		pre[l]->next[l] = pre[l]->next[l]->next[l];
	}
	delete[] pre[0]->next[0]->next;
	delete pre[0]->next[0];
}

template<typename T, typename Comparator>
bool SkipList<T, Comparator>::Get(T key)
{
	return FindPre(NULL, 0);
}

template<typename T, typename Comparator>
bool SkipList<T, Comparator>::FindPre(typename SkipList<T, Comparator>::Node** pre, int* level)
{
	bool rm = false;
	if (pre == nullptr)
	{
		pre = new Node*[MAXLEVEL];
		rm = true;
	}
	*level = -1;
	for (int l = MAXLEVEL; l >= 0; l--)
	{
		Node* p = (l == level) ? head_ : pre[l + 1];
		while (p->next[l] != tail_ && p->next[l]->key < key)
			p = p->next[l];
		if (p->next[l] != tail_ && p->next[l]->key == key)
		{
			pre[l] = p;
			if (*level == -1)
				*level = l;
		}
	}

	if (rm)
		delete[] pre;

	return level != -1;
}

template<typename T, typename Comparator>
void SkipList<T, Comparator>::Display()
{
	for (Node* p = head_->next; p != tail_; p = p->next[0])
	{
		printf("key, level = %d, %d\n", p->key, p->level);
	}
}