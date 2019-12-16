
class LRUCache{
	
private:
	
	size_t capacity_;
	
	size_t usage_;
	
	LRUHandle lru_; // lru链表，refs等于1并且in_cache等于true
	LRUHandle in_use_; // 被客户端使用的链表，refs大于等于2
	HandleTable table_;
};

void LRUCache::Ref(LRUHandle* e)
{
	if(e->refs == 1  &&  e->in_cache)
	{
		LRU_Remove(e);
		LRU_Append(&in_use_, e);
	}
	e->refs++;
}

void LRUCache::Unref(LRUHandle* e)
{
	e->refs--;
	if(e->refs==0)
	{
		(*e->deleter)(e->key(), e->value);
		free(e);
	}
	else if(e->in_cache  &&  e->refs==1) // 从in_use_中移动到lru_中
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
	e->next=list;
	e->prev=list->prev;
	e->prev->next=e;
	e->next->prev=e;
}

Cache::Handle* LRUCache::Lookup(const Slice& key, uint32_t hash)
{
	LRUHandle* e=table_.Lookup(key, hash);
	if(e!=nullptr)
		Ref(e);
	return reinterpret_cast<Cache::Handle*>(e);
}

void LRUCache::Release(Cache::Handle* handle)
{
	Unref(reinterpret_cast<LRUHandle*>(handle));
}

Cache::Handle* LRUCache::Insert(const Slice& key, uint32_t hash, void* value, size_t charge, void (*deleter)(const Slice& key, void* value))
{
	LRUHandle* e=reinterpret_cast<LRUHandle*>(malloc(sizeof(LRUHandle)-1+key.size()));
	e->value=value;
	e->deleter=deleter;
	e->charge=charge;
	e->key_length=key.size();
	e->hash=hash;
	e->in_cache=false; // ???
	e->refs=1; // ???
	memcpy(e->key_data, key.data(), key.size());
	
	if(capacity_>0)
	{
		e->refs++;
		e->in_cache=true;
		LRU_Append(&in_use_, e);
		usage_ += charge;
		FinishErase(table_.Insert(e));
	}
	else // 关闭cache
	{
		e->next=nullptr;
	}
	while(usage_>capacity_  &&  lru_.next!=&lru_)
	{
		LRUHandle* old=lru_.next;
		assert(old->refs==1);
		bool erases=FinishErase(table_.Remove(old->key(), old->hash));
	}
	reinterpret_cast<Cache::Handle*>(e);
}

void LRUCache::FinishErase(LRUHandle* e)
{
	if(e)
	{
		assert(e->in_cache);
		LRU_Remove(e);
		e->in_cache=false;
		usage_ -= e->charge;
		Unref(e);
	}
}

void LRUCache::Erase(const Slice& key, uint32_t hash)
{
	FinishErase(table_.Remove(key, hash));
}

void LRUCache::Prune()
{
	while(lru_.next != &lru_)
	{
		LRUHandle* e=lru_.enxt;
		assert(e->refs==1);
		bool erased=FinishErase(table_.Remove(e->key(), e->hash));
		assert(erased);
	}
}

////////////////////////////////
Cache::Handle* LRUCache::Insert(const Slice& key, uint32_t hash, void* value, size_t charge, void (*deleter)(const Slice& key, void* value))
{
	LRUHandle* e=reinterpret_cast<LRUHandle*>(malloc(sizeof(LRUHandle)-1+key.size()));
	e->value=value;
	e->deleter=deleter;
	e->charge=charge;
	e->key_length=key.size();
	e->hash=hash;
	e->in_cache=true;
	e->refs=2;
	memcpy(e->key_data, key.data(), key.size());
	LRU_Append(e); // 插入到lru_链表中，没有in_use_链表
	usage_+=charge;
	
	LRUHandle* old=table_.Insert(e);
	if(old)
	{
		LRU_Remove(old);
		Unref(old);
	}
	
	while(usage_>capacity_  &&  lru.next!=&lru_)
	{
		LRUHandle* old=lru_.next;
		LRU_Remove(old);
		table_.Remove(old->key(), old->hash);
		Unref(old);
	}
	return reinterpret_cast<Cache::Handle*>(e);
}

void LRUCache::LRU_Append(LRUHandle* e)
{
	e->next=&lru_;
	e->prev=lru_.prev;
	e->prev->next=e;
	e->next->prev=e;
}

void LRUCache::Unref(LRUHandle* e) {
	assert(e->refs > 0);
	e->refs--;
	if (e->refs <= 0) {
		usage_ -= e->charge;
		(*e->deleter)(e->key(), e->value);
		free(e);
	}
}

////////////////////////////////