#ifndef SKIPLIST_H_
#define SKIPLIST_H_

template<typename T>
class Comparator {
public:
	virtual bool compare(const T& a, const T& b) = 0;
};

// Comparator<int>类何时实例化？
template<>
bool Comparator<int>::compare(const int& a, const int& b)
{
	return a - b;
}

template<typename T, typename Comparator>
struct SkipList<T, Comparator>::Node {
	T key;

	int level;
	SkipList::Node* next[1];

	explicit Node(const T& k) :key(k) {}
};

template<typename T, typename Comparator>
class SkipList {
	struct Node;

public:
	explicit SkipList(int max_level);
	~SkipList();

	void Insert(int key, T data);
	void Remove(int key);
	T Get(int key);
	void Display();

private:
	Node * NewNode(int level);

	Node * head_;
	Node* tail_;

	Comparator comparator_;// 比较函数

	// Arena *arena_;

	const int MAXLEVEL;
};

/*
class Comparator {
public:
	virtual bool compare(const T& a, const T& b) = 0;
};

// Comparator派生类

template<typename T>
class SkipList{

	Comparator* comparator_; // Comparator基类
};
*/

#endif
