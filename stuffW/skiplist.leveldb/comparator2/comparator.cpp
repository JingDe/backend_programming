#include"comparator2.h"
#include"object.h"

template<typename T>
int Comparator<T>::compare(const T& a, const T& b) const
{
	if (a < b)
		return -1;
	else if (a == b)
		return 0;
	else
		return 1;
}

// ��ʾʵ����
template class Comparator<int>; 
template class Comparator<double>; 
template class Comparator<Object>;


// ���º�����������template class Comparator<Object>;ͬ�ļ��������ض���
bool operator==(const Object& a, const Object& b)
{
	return a.Data() == b.Data();
}

bool operator<(const Object& a, const Object& b)
{
	return a.Data() < b.Data();
}