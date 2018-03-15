#include"cache.h"

struct LRUHandle {	
	void(*deleter)(const Slice&, void *value);
	LRUHandle* next_hash; //����HandleTable������LRUHandle��ָ��

	LRUHandle* next; // next��prev����LRUCache�����������ָ��
	LRUHandle* prev;	
	bool in_cache; // ��entry�Ƿ���cache��
	uint32_t refs;

	char key_data[1]; // key�Ŀ�ͷ
	size_t key_length;
	uint32_t hash; // key()�Ĺ�ϣ���������ٷ�Ƭ�ͱȽ�

	void* value;

	Slice key() const
	{
		assert(next != this); // next_����thisʱ��LRUHandle��һ���������ͷ��㣬ͷ���û��������� key
		return Slice(key_data, key_length);
	}
};


class HandleTable {
public:
	HandleTable() : length_(0), elems_(0), list_(NULL) { Resize(); }
	~HandleTable() { delete[] list_; }

private:
	uint32_t length_; // bucket����ĳ���
	uint32_t elems_; // 
	LRUHandle** list_; // ����һ��bucket���飬ÿ��bucket��һ��cache entry�������е�Ԫ��hash�����bucket

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
		for (uint32_t i = 0; i < length_; i++) // ����bucket����
		{
			LRUHandle* h = list_[i]; // ��i��bucket��ͷ���
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