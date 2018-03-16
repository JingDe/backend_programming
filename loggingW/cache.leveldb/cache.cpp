#include"cache.h"
#include"common/MutexLock.h"
#include"port/port.h"

#include<cassert>

Cache::~Cache() {
}


struct LRUHandle {	
	void(*deleter)(const Slice&, void *value);
	LRUHandle* next_hash; //����HandleTable������LRUHandle��ָ��

	LRUHandle* next; // next��prev����LRUCache�����������ָ��
	LRUHandle* prev;	
	bool in_cache; // ��entry�Ƿ���cache��
	uint32_t refs;

	size_t charge;

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

	LRUHandle* Lookup(const Slice& key, uint32_t hash)
	{
		return *FindPointer(key, hash);
	}

	LRUHandle* Insert(LRUHandle* h)
	{
		LRUHandle** ptr = FindPointer(h->key(), h->hash);
		LRUHandle* old = *ptr;

	}

private:
	uint32_t length_; // bucket����ĳ���
	uint32_t elems_; // 
	LRUHandle** list_; // ����һ��bucket���飬ÿ��bucket��һ��cache entry�������е�Ԫ��hash�����bucket

	LRUHandle** FindPointer(const Slice& key, uint32_t hash)
	{
		LRUHandle** ptr = &list_[hash & (length_ - 1)];
		while (*ptr != NULL && ((*ptr)->hash != hash || key != (*ptr)->key()))
			ptr = &(*ptr)->next_hash;
		return ptr;
	}

	void Resize()
	{
		uint32_t new_length = 4;
		while(new_length < elems_)
		{
			new_length *= 2;
		}
		LRUHandle** new_list = new LRUHandle*[new_length]; // ����new_list���飬Ԫ����LRUHandle*
		memset(new_list, 0, sizeof(new_list[0]) * new_length);
		uint32_t count = 0;
		for (uint32_t i = 0; i < length_; i++) // ����bucket���飬��ÿ�����ȡ�����뵽new_list��
		{
			LRUHandle* h = list_[i]; // ��i��bucket��ͷ���
			while (h != NULL) // ������i��bucket��ÿ�����
			{
				LRUHandle* next = h->next_hash; 
				uint32_t hash = h->hash; // ����key�Ĺ�ϣֵ
				LRUHandle** ptr = &new_list[hash  & (new_length - 1)]; // �����new_list�е�λ��
				// ptr�洢new_listĳһԪ��
				h->next_hash = *ptr;
				*ptr = h;
				h = next;
				count++;
			}
		}
		assert(elems_ == count);
		delete[] list_;
		list_ = new_list;
		length_ = new_length;
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
		MutexLock l(mutex_);
		return usage_;
	}

private:
	void LRU_Remove(LRUHandle* e);
	void LRU_Append(LRUHandle*list, LRUHandle* e);
	void Ref(LRUHandle* e);
	void Unref(LRUHandle* e);
	bool FinishErase(LRUHandle* e);

	size_t capacity_; // �����������

	mutable port::Mutex mutex_;
	size_t usage_; // ��ʹ������

	LRUHandle lru_; // lru.prev�����µ�entry��lru.next����ɵ�entry��refs==1����in_cache==true 
	LRUHandle in_use_; // ���û�ʹ�ã�refs���ڵ���2����in_cache==true

	HandleTable table_;
};

LRUCache::LRUCache()
	: usage_(0) {
	lru_.next = &lru_;
	lru_.prev = &lru_;
	in_use_.next = &in_use_;
	in_use_.prev = &in_use_;
}

LRUCache::~LRUCache()
{
	assert(in_use_.next == &in_use_);
	for (LRUHandle* e = lru_.next; e != &lru_; )
	{
		LRUHandle* next = e->next;
		assert(e->in_cache);
		e->in_cache = false;
		assert(e->refs == 1);
		Unref(e);
		e = next;
	}
}

void LRUCache::Ref(LRUHandle* e) // ��lru_�е�e�ƶ���in_use_�У�����in_use_�е�e�����ü���
{
	if (e->refs == 1 && e->in_cache)
	{
		LRU_Remove(e);
		LRU_Append(&in_use_, e);
	}
	e->refs++;
}

void LRUCache::Unref(LRUHandle* e) // ��lru_�е�eɾ������in_use�е�e�ƶ���lru_��
{
	assert(e->refs > 0);
	e->refs--;
	if (e->refs == 0)
	{
		assert(!e->in_cache);
		(*e->deleter)(e->key(), e->value);
		free(e);
	}
	else if (e->in_cache  &&  e->refs == 1)
	{
		LRU_Remove(e);
		LRU_Append(&lru_, e);
	}
}

void LRUCache::LRU_Remove(LRUHandle* e)
{
	e->next->prev = e->prev;
	e->prev->next = e->next;
}

void LRUCache::LRU_Append(LRUHandle* list, LRUHandle* e)
{
	e->next = list;
	e->prev = list->prev;
	e->prev->next = e;
	e->next->prev = e;
}

Cache::Handle* LRUCache::Lookup(const Slice& key, uint32_t hash)
{
	MutexLock l(mutex_);
	LRUHandle* e = table_.Lookup(key, hash);
	if (e != NULL)
		Ref(e);
	return reinterpret_cast<Cache::Handle*>(e);
}

void LRUCache::Release(Cache::Handle* handle)
{
	MutexLock l(mutex_);
	Unref(reinterpret_cast<LRUHandle*>(handle));
}

Cache::Handle* LRUCache::Insert(const Slice& key, uint32_t hash, void* value, size_t charge,
	void(*deleter)(const Slice& key, void* value))
{
	MutexLock l(mutex_);

	LRUHandle* e = reinterpret_cast<LRUHandle*>(malloc(sizeof(LRUHandle) - 1 + key.size()));
	e->value = value;
	e->deleter = deleter;
	e->charge = charge;
	e->key_length = key.size();
	e->hash = hash;
	e->in_cache = false;
	e ->refs = 1;
	memcpy(e->key_data, key.data(), key.size());

	if (capacity_ > 0)
	{
		e->refs++;
		e->in_cache = true;
		LRU_Append(&in_use_, e);
		usage_ += charge;
		FinishErase(table_.Insert(e));
	}
	else
		e->next = NULL;
	while (usage_ > capacity_  &&  lru_.next != &lru_)
	{

	}
}