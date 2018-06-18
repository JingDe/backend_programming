#ifndef SKIPLIST_H_
#define SKIPLIST_H_


template<typename T, typename Comparator>// Comparator不需要满足继承关系，只要求具有compara方法
class SkipList 
{
private:
	struct Node;

public:
	explicit SkipList(int max_level, Comparator cmp);
	~SkipList();

	void Insert(T key);
	void Remove(T key);
	bool Contains(T key);
	void Display();

private:
	int RandomLevel();
	Node * NewNode(const T& key, int level);
	int Compare(const T& a, const T& b);
	bool FindPre(T key, Node** pre, int* level);

	Node * head_;
	Node* tail_;

	const Comparator comparator_;// 比较对象
	// Arena *arena_;
	const int MAXLEVEL;
};


template<typename T, typename Comparator>
struct SkipList<T, Comparator>::Node {
	T key;
	int level;
	SkipList::Node* next_[1];
	//SkipList::Node** next_;

	explicit Node(const T& k) :key(k) {}
	Node(const T& k, int l) :key(k), level(l) {}
};



template<typename T, typename Comparator>
typename SkipList<T, Comparator>::Node* SkipList<T, Comparator>::NewNode(const T& key, int level)
{
	char* p = new char[sizeof(Node) + sizeof(Node*) * (level - 1)];
	return new (p) Node(key, level); // 析构时负责释放p，或使用Arena代替
	/*Node* result = new Node(key);
	result->level = level;
	result->next_ = new Node*[level];
	return result;*/
}

// 返回值取值[1, MAXLEVEL]
template<typename T, typename Comparator>
int SkipList<T, Comparator>::RandomLevel()
{
	int level = 1;
	while (rand() % 2 && level < MAXLEVEL)
		level++;
	return level;
}

template<typename T, typename Comparator>
SkipList<T, Comparator>::SkipList(int max_level, Comparator cmp)
	:MAXLEVEL(max_level), comparator_(cmp)
{
	head_ = NewNode(0, max_level); // 0
	tail_ = NewNode(0, 1);
	for (int i = 0; i < MAXLEVEL; i++)
	{
		head_->next_[i] = tail_;		
	}
	tail_->next_[0] = nullptr;
}

template<typename T, typename Comparator>
SkipList<T, Comparator>::~SkipList()
{
	Node* curr = nullptr;
	while (head_->next_[0] != tail_)
	{
		curr = head_->next_[0];
		head_->next_[0] = curr->next_[0];
		// delete[]curr->next_; // 不适合 SkipList::Node* next_[1]; new (p) Node(key, level) 方式，此语句后，curr可能为NULL？
		// 只直接delete curr能否删除p指向内存？？
		delete curr;		
	}
	// delete[]head_->next_;
	delete head_;
	delete tail_;
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
	int level = RandomLevel(); 
	int l;

	// 寻找[0, level]层待插入结点的前驱节点
	SkipList::Node** pre = new Node*[level]; // 暂存新节点的前驱结点
	for (l = level-1; l >= 0; l--)
	{
		SkipList::Node* p = (l == level-1) ? head_ : pre[l + 1];
		while (p->next_[l] != tail_ && Compare(p->next_[l]->key, key)<0) //comparator_(p->next[l]->key, key)<0 //p->next[l]->key < key // Compare(p->next[l]->key, key)<0
			p = p->next_[l];
		pre[l] = p;
	}

	// 创建新节点，插入[0, level-1]层
	SkipList<T, Comparator>::Node* newnode = NewNode(key, level);
	for (l = level-1; l >= 0; l--)
	{
		newnode->next_[l] = pre[l]->next_[l];
		pre[l]->next_[l] = newnode;
	}
}



template<typename T, typename Comparator>
void SkipList<T, Comparator>::Remove(T key)
{
	Node* pre[MAXLEVEL];
	int level = -1;

	if (FindPre(key, pre, &level) == false)
		return;

	for (int l = level; l >= 0; l--)
	{
		pre[l]->next[l] = pre[l]->next[l]->next[l];
	}
	delete[] pre[0]->next[0]->next;
	delete pre[0]->next[0];
}

template<typename T, typename Comparator>
bool SkipList<T, Comparator>::Contains(T key)
{
	int dummy = 0;
	return FindPre(key, nullptr, &dummy);
}

template<typename T, typename Comparator>
bool SkipList<T, Comparator>::FindPre(T key, typename SkipList<T, Comparator>::Node** pre, int* level)
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
			if (*level == -1)
				*level = l;
		}
		pre[l] = p;
	}

	if (rm)
		delete[] pre;

	return *level != -1;
}

template<typename T, typename Comparator>
void SkipList<T, Comparator>::Display()
{
	for (Node* p = head_->next_[0]; p != tail_; p = p->next_[0])
	{
		printf("key, level = %d, %d\n", p->key, p->level);
	}
}
#endif
