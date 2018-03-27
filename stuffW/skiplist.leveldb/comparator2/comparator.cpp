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

// 显示实例化
template class Comparator<int>; 
template class Comparator<double>; 
template class Comparator<Object>;


// 以下函数定义需与template class Comparator<Object>;同文件，否则报重定义
bool operator==(const Object& a, const Object& b)
{
	return a.Data() == b.Data();
}

bool operator<(const Object& a, const Object& b)
{
	return a.Data() < b.Data();
}