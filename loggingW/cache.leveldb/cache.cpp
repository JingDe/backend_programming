#include"cache.h"

struct LRUHandle {	
	void(*deleter)(const Slice&, void *value);
	LRUHandle* next_hash; //链接HandleTable的所有LRUHandle的指针

	LRUHandle* next; // next和prev链接LRUCache的两条链表的指针
	LRUHandle* prev;	
	bool in_cache; // 该entry是否在cache中
	uint32_t refs;

	char key_data[1]; // key的开头
	size_t key_length;
	uint32_t hash; // key()的哈希；用来快速分片和比较

	void* value;

	Slice key() const
	{
		assert(next != this); // next_等于this时，LRUHandle是一个空链表的头结点，头结点没有有意义的 key
		return Slice(key_data, key_length);
	}
};


class HandleTable {
public:
	HandleTable() : length_(0), elems_(0), list_(NULL) { Resize(); }
	~HandleTable() { delete[] list_; }

private:
	uint32_t length_; // bucket数组的长度
	uint32_t elems_; // 
	LRUHandle** list_; // 包含一个bucket数组，每个bucket是一个cache entry链表，其中的元素hash到这个bucket

	void Resize()
	{
		uint32_t new_length = 4;
		while(new_length < elems_)
		{
			new_lengh *= 2;
		}
		LRUHandle** new_list = new LRUHandle*[new_length];
		memset(new_list, 0, sizeof(new_list[0]) * new_length);
		uint32_t count = 0;
		for (uint32_t i = 0; i < length_; i++) // 遍历bucket数组
		{
			LRUHandle* h = list_[i]; // 第i个bucket的头结点
			while (h != NULL)
			{
				LRUHandle* next = h->next_hash;
				uint32_t hash = h->hash;
			}
		}
	}
};

class LRUCache {
public:
	LRUCache();
	~LRUCache();

	void SetCapacity(size_t capacity) { capacity_ = capacity; }

	Cache::Handle* Insert(const Slice& key, uint32_t hash,
		void* value, size_t charge,
		void(*deleter)(const Slice& key, void* value));
	Cache::Handle* Lookup(const Slice& key, uint32_t hash);
	void Release(Cache::Handle* handle);
	void Erase(const Slice& key, uint32_t hash);
	void Prune();
	size_t TotalCharge() const {
		MutexLock l(&mutex_);
		return usage_;
	}

private:
	void LRU_Remove(LRUHandle* e);
	void LRU_Append(LRUHandle*list, LRUHandle* e);
	void Ref(LRUHandle* e);
	void Unref(LRUHandle* e);
	bool FinishErase(LRUHandle* e);

	size_t capacity_;

	mutable port::Mutex mutex_;
	size_t usage_;

	LRUHandle lru_;
	LRUHandle in_use_;

	HandleTable table_;
};

LRUCache::LRUCache()
	: usage_(0) {
	lru_.next = &lru_;
	lru_.prev = &lru_;
	in_use_.next = &in_use_;
	in_use_.prev = &in_use_;
}