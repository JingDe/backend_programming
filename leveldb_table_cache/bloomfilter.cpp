
class BloomFilter{
private:
	static int DEFAULT_SIZE = 1<<24;
	static vector<int> seeds({5, 7, 11, 13, 31, 37, 61});
	
	std::bitset<DEFAULT_SIZE> bits;
	
	// k个哈希函数——一个哈希函数，k个参数
	std::vector<HashFunc> func;
	
public:
	BloomFilter();
	void add(const string& value);
	bool contains(const string& value);
};

BloomFilter::BloomFilter()
{
	func.reserve(seeds.size());
	for(int i=0; i<seeds.length(); i++)
	{
		func.emplace_back(seeds[i], DEFAULT_SIZE);
	}
}

void BloomFilter::add(const string& value)
{
	for(HashFunc f : fun)
	{
		bits.set(f.hash(value), true);
	}
}

// 所有哈希函数计算出来的位都为1时，返回true；否则返回false
bool BloomFilter::contains(const string& value)
{
	if(value.empty())
		return false;
	bool ans=true;
	for(HashFunc f :  func)
	{
		if(bits.test(f.hash(value)) == false)
		{
			ans=false;
			break;
		}
	}
	return ans;
}

bool BloomFilter::contains(const string& value)
{
	
}


////////////////////////////////
class HashFunc{
public:
	HashFunc(int seed, int cap);
	int hash(const string& value);
	
private:
	int seed;
	int cap; // bit数组的长度是 2^cap
};

int HashFunc::hash(const string& value)
{
	int result=0;
	int len=value.length();
	for(int i=0; i<len; ++i)
	{
		result = seed * result + value[i]-'0';
	}
	return result & (cap-1);
}




