


class Arena{
public:
	Arena();
	Arena(const Arena&)=delete;
	Arena& operator=(const Arena&)=delete;
	
	~Arena();
	
	char* Allocate(size_t bytes);
	char* AllocateAligned(sizes bytes);
	size_t MemoryUsage() const{
		return memory_usage_.load(std::memory_order_relaxed);
	}
	
	
private:
	char* AllocateFallback(size_t bytes);
	char* AllocateNewBlock(size_t block_bytes);

	// 分配状态
	char* alloc_ptr_;
	size_t alloc_bytes_remainming_;
	
	// 通过new[]分配的内存块
	std::vector<char*> blocks_;
	// 总的内存使用量
	std::atomc<size_t> memory_usage_;
};

Arena::Arena():alloc_ptr_(NULL), alloc_bytes_remainming_(0),memory_usage_(0)
{}

~Arena::Arena()
{
	for(size_t i=0; i<blocks_.size(); ++i)
		delete[] blocks_[i];
}

// inline char* Arena::Allocate(size_t bytes)
// {
	// char* ans=NULL;
	// assert(bytes>0);
	// if(alloc_bytes_remainming_>bytes)
	// {
		// ans=alloc_ptr_;
		// alloc_ptr_+=bytes;
		// alloc_bytes_remainming_-=bytes;
		// return ans;
	// }
	
	// char* block=new char[BLOCK_SIZE]
	// if(block==NULL)
		// return NULL;
	// blocks_.push_back(block);
	// alloc_ptr_=block;
	// alloc_bytes_remainming_=BLOCK_SIZE;
	// memory_usage_.set(memory_usage_.get()+BLOCK_SIZE);
	// return Allocate(bytes);
// }

inline char* Arena::Allocate(size_t bytes)
{
	assert(bytes>0);
	if(bytes<=alloc_bytes_remainming_)
	{
		char* result=alloc_ptr_;
		alloc_ptr_ += bytes;
		alloc_bytes_remainming_ -= bytes;
		return result;
	}
	return AllocateFallback(bytes);
}

// char* Arena::AllocateFallback(size_t bytes)
// {
	// char* block=new char[BLOCK_SIZE]
	// if(block==NULL)
		// return NULL;
	// blocks_.push_back(block);
	// char* result=block;
	// alloc_ptr_=block+bytes;
	// alloc_bytes_remainming_=BLOCK_SIZE-bytes;
	// memory_usage_.set(memory_usage_.get()+BLOCK_SIZE);
	// return result;
// }

static const int kBlockSize = 4096;

char* Arena::AllocateFallback(size_t bytes)
{
	if(bytes > kBlockSize/4) // 当请求的内存足够大时，直接分配一个恰好满足请求的内存块, 减少内存碎片
	{
		char* result=AllocateNewBlock(bytes);
		return result;
	}
	// 当上一块内存块剩余内存不足要求时，丢弃，并分配新的内存块
	alloc_ptr_ = AllocateNewBlock(kBlockSize);
	alloc_bytes_remainming_ = kBlockSize;
	
	char* result=alloc_ptr_;
	alloc_ptr_ += bytes;
	alloc_bytes_remainming_ -= bytes;
	return result;
}


char* Arena::AllocateNewBlock(size_t size)
{
	char* block=new char[size];
	if(block==NULL)
		return NULL;
	blocks_.push_back(block);
	//memory_usage_.set(memory_usage_.get()+size);
	memory_usage_.fetch_add(size+sizeof(char*), std::memory_order_relaxed);
	// 原子地以当前值+(size+sizeof(char*))替换当前值
	return block;
}

// char* Arena::AllocateAligned(size_t size)
// {
	// if(alloc_ptr_  %  sizeof(void*))
	// {
		// int align_bytes =sizeof(void*)-(alloc_ptr_ % sizeof(void*));
		// int avail = alloc_bytes_remainming_ - align_bytes;
		
		// if(avail < size)
		// {
			
		// }
		
	// }
// }

char* Arena::AllocateAligned(size_t bytes)
{
	// 对齐字节等于指针大小，最小8个字节，必须是2的幂次方
	const int align= (sizeof(void*) > 8)  ?  sizeof(void*)  :  8;
	static_assert((align  &  (align-1)) == 0, "");
	
	size_t current_mod = reinterpret_cast<uintptr_t>(alloc_ptr_) &  (align-1);
	size_t slop = (current_mod==0 ? 0 : align-current_mod);
	size_t needed = bytes+slop;
	
	char* result;
	if(needed <= alloc_bytes_remainming_)
	{
		result = alloc_ptr_ + slop;
		alloc_ptr_ += needed;
		alloc_bytes_remainming_ -= needed;
	}
	else
	{
		result=AllocateFallback(bytes); // 返回地址是new返回的内存地址，保证是对齐的。
	}
	assert((reinterpret_cast<uintptr_t>(result)  &  (align-1))==0);
	return result;
}

