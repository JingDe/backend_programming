#ifndef SKIPLIST_H_
#define SKIPLIST_H_


template<typename T, typename Comparator>// Comparator����Ҫ����̳й�ϵ��ֻҪ�����compara����
class SkipList 
{
private:
	struct Node;

public:
	explicit SkipList(int max_level, Comparator cmp);
	~SkipList();

	void Insert(T key);
	void Remove(T key);
	bool Get(T key);
	void Display();

private:
	int RandomLevel();
	Node * NewNode(const T& key, int level);
	int Compare(const T& a, const T& b);
	bool FindPre(Node** pre, int* level);

	Node * head_;
	Node* tail_;

	const Comparator comparator_;// �Ƚ϶���
	// Arena *arena_;
	const int MAXLEVEL;
};


template<typename T, typename Comparator>
struct SkipList<T, Comparator>::Node {
	T key;
	int level;
	SkipList::Node* next[1];

	explicit Node(const T& k) :key(k) {}
};


#endif
