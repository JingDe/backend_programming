
/*
包装函数：
PutFixed32
PutFixed64
PutVarint32
PutVarint64
PutLengthPrefixedSlice

GetVarint32
GetVarint64
GetLengthPrefixedSlice

*/

void PutFixed32(std::string* dst, uint32_t value)
{
	char buf[sizeof(value)];
	EncodeFixed32(buf, value);
	dst->append(buf, sizeof(buf));
}

void PutFixed64(std::string* dst, uint64_t value)
{}

void PutVarint32(std::string* dst, uint32_t v)
{
	char buf[5];
	char* ptr=EncodeVarint32(buf, v);
	dst->append(buf, ptr-buf);
}

void PutVarint64(std::string* dst, uint64_t value)
{}

void PutLengthPrefixedSlice(std::string* dst, const Slice& value)
{
	PutVarint32(dst, value.size());
	dst->append(value.data(), value.size());
}

// 从input指向的内存中解码变长uint32_t，input指向剩下的slice
bool GetVarint32(Slice* input, uint32_t* value)
{
	const char* p=input->data();
	const char* limit=p+input->size();
	const char* q=GetVarint32Ptr(p, limit, value);
	if(q==nullptr)
	{
		return false;
	}
	else
	{
		*input = Slice(q, limit-q);
		return true;
	}
}

bool GetVarint64(Slice* input, uint64_t* value)
{}

const char* GetLengthPrefixedSlice(const char* p, const char* limit, Slice* result)
{
	uint32_t len;
	p = GetVarint32Ptr(p, limit, &len);
	if(p==nullptr)
		return nullptr;
	if(p+len > limit)
		return nullptr;
	*result = Slice(p, len);
	return p+len;
}

// input指向的内存，包含一个变长的长度和对应长度的字符串
bool GetLengthPrefixedSlice(Slice* input, Slice* result)
{
	uint32_t len;
	if(GetVarint32(input, &len)  &&  input->size()>=len)
	{
		*result=Slice(input->data(), len);
		input->remove_prefix(len);
		return true;
	}
	else
	{
		return false;
	}
}