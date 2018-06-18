#ifndef COMPARATOR_H_
#define COMPARATOR_H_

template<typename T>
class Comparator {
public:
	int compare(const T& a, const T& b) const
	{
		if (a < b)
			return -1;
		else if (a == b)
			return 0;
		else
			return 1;
	}
};

// Comparator<int>���ʱʵ������
//template<>
//int Comparator<int>::compare(const int& a, const int& b)
//{
//	if (a < b)
//		return -1;
//	else if (a == b)
//		return 0;
//	else
//		return 1;
//}


/* ��һ��д���� Comparator����
class Comparator {
public:
virtual bool compare(const T& a, const T& b) = 0;
};

// ...����Comparator������

template<typename T>
class SkipList{

Comparator* comparator_;
};
*/


#endif
