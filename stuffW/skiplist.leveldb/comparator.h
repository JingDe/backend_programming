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

template<>
class Comparator<int>;





#endif
