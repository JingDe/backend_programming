//#include "stdafx.h"
#include"FixedBuffer.h"


template<int SIZE>
const char* FixedBuffer<SIZE>::debugString()
{
	*cur_ = '\0';
	return data_;
}