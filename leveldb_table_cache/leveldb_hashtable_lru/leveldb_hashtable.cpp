
struct LRUHandle{
	
	LRUHandle *next_hash;
	LRUHandle *next;
	LRUHandle *prev;
	
	bool in_cache; // 是否在LRUCache中
	uint32_t refs; // 引用计数，包括在LRUCache中的一次
	
	char key_data[1];
	
};

// 哈希表
class HandleTable{
public:
	LRUHandle* Lookup(const Slice& key, uint32_t hash)
	{
		return *FindPointer(key, hash);
	}
	
	LRUHandle* Insert(LRUHandle* h)
	{
		LRUHandle** ptr=FindPointer(h->key(), h->hash);
		LRUHandle* old=*ptr;
		h->next_hash=(old==nullptr ? nullptr : old->next_hash);
		*ptr=h;
		if(old == nullptr)
		{
			++elems_;
			if(elems_ > length_)
				Resize();
		}
		return old;
	}
	
	LRUHandle* Remove(const Slice& key, uint32_t hash)
	{
		LRUHandle** ptr=FindPointer(h->key(), h->hash);
		if(*ptr)
		{
			*ptr = *ptr->next_hash;
			--elems_;
		}
		return result;
	}
	
	void Resize()
	{
		uint32_t new_length=4;
		while(new_length < elems_)
			new_length<<1;
		
		LRUHandle** new_list=new LRUHandle*[new_length];
		memset(new_list, 0, sizeof(new_list[0]*new_length));
		
		uint32_t count=0;
		for(uint32_t i=0; i<lenght_; i++)
		{
			LRUHandle* h=list_[i];
			while(h)
			{
				LRUHandle *next=h->next_hash;
				uint32_t hash=h->hash;
				LRUHandle** ptr=&new_list[hash  &  (new_length-1)];
				h->next_hash=*ptr;
				*ptr=h;
				h=next;
				count++;
			}
		}
		delete[] list_;
		list_=new_list;
		length=new_length;
	}
	
private:
	uint32_t length_; // 是2的n次方
	LRUHandle** list_;

	LRUHandle** FindPointer(const Slice& key, uint32_t hash)
	{
		LRUHandle** ptr=&list_[hash & (length_-1)];
		while(*ptr!=nullptr  &&  ((*ptr)->hash!=hash || key!=(*ptr)->key))
		{
			ptr = &(*ptr)->h_next;
		}
		return ptr;
	}
};