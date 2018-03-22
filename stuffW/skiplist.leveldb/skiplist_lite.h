#ifndef SKIPLIST_LITE_H_
#define SKIPLIST_LITE_H_

struct Comparator {
	/*int compare(const int& a, int b) const
	{
		if (a < b)
			return -1;
		else if (a == b)
			return 0;
		else
			return 1;
	}*/

	int operator()(const int& a, const int& b) const {
		if (a < b) {
			return -1;
		}
		else if (a > b) {
			return +1;
		}
		else {
			return 0;
		}
	}
};

class SkipList
{
private:
	struct Node;

public:
	explicit SkipList(int max_level, Comparator cmp);
	~SkipList();

	void Insert(int key);
	void Remove(int key);
	bool Get(int key);
	void Display();

private:
	int RandomLevel();
	Node * NewNode(const int& key, int level);
	int Compare(const int& a, const int& b);
	bool FindPre(const int& key, Node** pre, int* level);

	Node * head_;
	Node* tail_;

	const Comparator comparator_;// 比较对象
								 // Arena *arena_;
	const int MAXLEVEL;
};



struct SkipList::Node {
	explicit Node(const int& k) :key(k) {}

	int key;
	int level;

	SkipList::Node** next_;	// SkipList::Node* next_[1];
};


#endif
