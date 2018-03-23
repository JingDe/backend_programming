#include"skiplist_lite.h"

#include<cstdlib>
#include<cstdio>
 
typename SkipList::Node* SkipList::NewNode(const int& key, int level) // level取值范围[0, MAXLEVEL-1]
{
	//char* p = new char[sizeof(Node*) * (level - 1)];
	//return new(p) Node(key);
	Node* result = new Node(key);
	result->level = level;
	result->next_ = new Node*[level];
	return result;
}

 
int SkipList::RandomLevel()
{
	int level = 0;
	while (rand() % 2  && level < MAXLEVEL-1)
		level++;
	return level;
}

 
SkipList::SkipList(int max_level, Comparator cmp)
	:MAXLEVEL(max_level), comparator_(cmp)
{
	head_ = NewNode(0, MAXLEVEL); // 0
	tail_ = NewNode(0, 0);
	for (int i = 0; i < MAXLEVEL; i++)// [0, MAXLEVEL-1]层
	{
		head_->next_[i] = tail_;
		//tail_->next_[i] = NULL;
	}
	tail_->next_[0] = nullptr;
}

 
SkipList::~SkipList()
{
	/*Node* p = head_;
	Node* q;
	while (p != tail_)
	{
		q = p->next_[0];
		delete[]p->next_;
		delete p;
		p = q;
	}
	delete tail_;*/
	
	Node* curr = nullptr;
	while (head_->next_[0] != tail_)
	{
		curr = head_->next_[0];
		head_->next_[0] = curr->next_[0];
		delete []curr->next_;
		delete curr;
	}
	delete []head_->next_;
	delete head_;
	delete tail_;

	printf("end deconstruct\n");
}

 
int SkipList::Compare(const int& a, const int& b)
{
	//return comparator_.compare(a, b);
	return comparator_(a, b);
}

 
void SkipList::Insert(int key)
{
	int level = RandomLevel(); // level取值[0, MAXLEVEL-1]
	int l;

	// 寻找[0, level]层待插入结点的前驱节点
	SkipList::Node** pre = new Node*[level + 1]; // 暂存新节点的前驱结点
	for (l = level; l >= 0; l--)
	{
		SkipList::Node* p = (l == level) ? head_ : pre[l + 1];
		while (p->next_[l] != tail_ && Compare(p->next_[l]->key, key)<0) //comparator_(p->next_[l]->key, key)<0 //p->next_[l]->key < key // Compare(p->next_[l]->key, key)<0
			p = p->next_[l];
		pre[l] = p;
	}

	// 创建新节点，插入[0, level]层
	SkipList::Node* newnode = NewNode(key, level+1);
	for (l = level; l >= 0; l--)
	{
		newnode->next_[l] = pre[l]->next_[l];
		pre[l]->next_[l] = newnode;
	}
}



 
void SkipList::Remove(int key)
{
	Node** pre=new Node*[MAXLEVEL];
	int level = -1;

	if (FindPre(key, pre, &level) == false)
		return;

	for (int l = level; l >= 0; l--)
	{
		pre[l]->next_[l] = pre[l]->next_[l]->next_[l];
	}
	delete[] pre[0]->next_[0]->next_;
	delete pre[0]->next_[0];

	delete[] pre;
}

 
bool SkipList::Contains(int key)
{
	int dummy;
	return FindPre(key, NULL, &dummy);
}

// 查找key的前驱节点和所在level,level范围[0, MAXLEVEL-1]
bool SkipList::FindPre(const int& key, SkipList::Node** pre, int* level)
{
	bool rm = false;
	if (pre == nullptr)
	{
		pre = new Node*[MAXLEVEL];
		rm = true;
	}
	*level = -1;
	for (int l = MAXLEVEL-1; l >= 0; l--)
	{
		Node* p = (l == MAXLEVEL-1) ? head_ : pre[l + 1];
		while (p->next_[l] != tail_ && p->next_[l]->key < key)
			p = p->next_[l];
		if (p->next_[l] != tail_ && p->next_[l]->key == key)
		{
			pre[l] = p;
			if (*level == -1)
				*level = l;
		}
		else
		{
			pre[l] = head_;
		}
	}

	if (rm)
		delete[] pre;

	return *level != -1;
}

 
void SkipList::Display()
{
	for (Node* p = head_->next_[0]; p != tail_; p = p->next_[0])
	{
		printf("key, level = %d, %d\n", p->key, p->level);
	}
}