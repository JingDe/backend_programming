
/* 哈希表结构体，每个字典包含两个dictht结构体，用于rehash。
	*/
typedef struct dictht{
	dictEntry **table; /**/
	unsigned long size; /*哈希表的大小，table数组的长度 */
	unsigned long sizemask; /*size-1, 用于对哈希值取余*/
	unsigned long used; // 实际元素个数
}dictht;

/*哈希表节点*/
typedef struct dictEntry{
	void *key;
	union{
		void *val;
		uint64_t u64;
		int64_t s64;
		double d;
	}v;
	struct dictEntry* next; // 连接多个哈希值相同的节点
}dictEntry;

/*字典*/
typedef struct dict{
	dictType *type; // 一簇用于操作特定类型键值对的函数
	void *privatedata; // 保存需要传递给类型特定函数的可选参数
	dictht ht[2];
	long rehashidx; // 当前rehash的进度
	unsigned long iterators;
}dict;

typedef struct dictType{
	uint64_t (*hashFunction)(const void *key);
	void* (*keyDup)(void *privdata, const void *key);
	void* (*valDup)(void *privdata, const void *obj);
	int (*keyCompare)(void *privdata, const void *key1, const void *key2);
	void (*keyDestructor)(void *privdata, void *key);
	void (*valDestructor)(void *privdata, void *obj);
};

// 创建字典
dict* dictCreate(dictType *type, void *privDataPtr)
{
	dict *d=zmalloc(sizeof(*d));
	_dictInit(d, type, privDataPtr);
	return d;
}

int _dictInit(dict *d, dictType *type, void *privDataPtr)
{
	_dictReset(&d->ht[0]);
	_dictReset(&d->ht[1]);
	d->type=type;
	d->privDataPtr=privDataPtr;
	d->rehashidx=-1;
	d->iterators=0;
	return DICT_OK;
}

// 重置哈希表
static void _dictReset(dictht *ht)
{
	ht->table=NULL;
	ht->size=0;
	ht->sizemask=0;
	ht->used=0;
}

// 将键值对key/val插入到字典d中
int dictAdd(dict *d, void *key, void *val)
{
	dictEntry *entry=dictAddRaw(d, key, NULL);
	if(!entry)
		return DICT_ERR;
	dictSetVal(d, entry, val); // 设置值
	return DICT_OK;
}

// 在字典d中添加一个记录
dictEntry* dictAddRaw(dict *d, void *key, dictEntry **existing)
{
	long index;
	dictEntry *entry;
	dictht *ht;
	
	if(dictIsRehashing(d))
		_dictRehashStep(d);
	
	// 获得新元素的索引， -1表示元素已经存在
	if((index=_dictKeyIndex(d, key, dictHashKey(d, key), existing)) == -1)
		return NULL;
	
	// 当字典在rehash，只插入到新哈希表中
	ht=dictIsRehashing(d) ? &d->ht[1] : &d->ht[0];
	entry=zmalloc(sizeof(*entry));
	entry->next=ht->table[index]; // 将新条目插入到对应桶的开头
	ht->table[index]=entry;
	ht->used++;
	
	dictSetKey(d, entry, key); // 设置键, 设置字典d中的条目entry指向的键为key
	return entry;
}

// #define dictSetKey(d, entry, _key_) do{\
	// if((d)->type->keyDup) \
		// (entry)->key = (d)->type->keyDup((d)->privdata, _key_); \
	// else \
		// (entry)->key = (_key_);
// }while(0)

#define dictIsRehashing(d) ((d)->rehashidx!=-1)

static inline dictSetKey(dict* d, dictEntry* entry, void* key)
{
	if(d->type->keyDup)
		entry->key = d->type->keyDup(d->privDataPtr, key);
	else
		entry->key = key;
}


static inline dictSetVal(dict* d, dictEntry* entry, void* val)
{
	if(d->type->valDup)
		entry->val = d->type->valDup(d->privDataPtr, val);
	else
		entry->val = val;
}

/*返回一个空闲的slot，可以
	实际是检查是否存在ht[0]或者ht[1]中，否则计算mod的桶索引，
	不再rehash的时候，返回的是ht[0]中的，在rehash的时候，返回ht[1]中的桶号 */
static long _dictKeyIndex(dict* d, const void* key, uint64_t hash, dictEntry** existing)
{
	unsigned long idx, table;
	dictEntry *he;
	if(existing)
		*existing=NULL;
	
	if(_dictExpandIfNeeded(d)==DICT_ERR) // 是否扩展字典
		return -1;
	for(table=0; table <= 1; table++)
	{
		idx=hash & d->ht[table].sizemask;
		he = d->ht[table].table[idx];
		while(he)
		{
			if(key==he->key  ||  dictCompareKeys(d, key, he->key))
			{
				if(existing)
					*existing=he;
				return -1;
			}
			he=he->next;
		}
		if(!dictIsRehashing(d))
			break;
	}
	return idx;
}

static int _dictExpandIfNeeded(dict *d)
{
	if(dictIsRehashing(d)) // 字典已经在rehash
		return DICT_OK;
	
	if(d->ht[0].size==0)
		return dictExpand(d, DICT_HT_INITIAL_SIZE);
	
	if(d->ht[0].used >= d->ht[0].size  &&  
		(dict_can_resize  ||  
		d->ht[0].used/d->ht[0].size > dict_force_resize_ratio))
	{
		return dictExpand(d, d->ht[0].used*2);
	}
	return DICT_OK;
}

// 扩展ht[1]或者创建哈希表ht[0]
int dictExpand(dict *d, unsigned long size)
{
	if(dictIsRehashing(d)  ||  d->ht[0].used>size)
		return DICT_ERR;
	
	dictht n;
	unsigned long realsize=_dictNextPower(size);
	
	if(realsize == d->ht[0].size)
		return DICT_ERR;
	
	n.size=realsize;
	n.sizemask=realsize-1;
	n.table=zmalloc(realsize*sizeof(dictEntry*));
	u.used=0;
	
	if(d->ht[0].table==NULL)
	{
		d->ht[0]=n;
		return DICT_OK;
	}
	d->ht[1]=n;
	d->rehashidx=0;
	return DICT_OK;
}

static unsigned long _dictNextPower(unsigned long size)
{
	unsigned long i=DICT_HT_INITIAL_SIZE;
	if(size>= LONG_MAX) // 有符号long的最大值
		return LONG_MAX+1LU; // 加一，是2的幂
	while(1)
	{
		if(i>=size)
			return i;
		i*=2;
	}
}

// 添加给定的键值对，如果已经存在，则替换val
int dictReplace(dict *d, void* key, void* val)
{
	dictEntry *entry, *existing, auxentry;
	entry=dictAddRaw(d, key, &existing); // 返回NULL表示已经存在（*existing），否则返回新节点
	if(entry)
	{
		dictSetVal(d, entry, val);
		return 1;
	}
	
	// 
	auxentry=*existing; // 复制相同key的节点，获得val指针
	dictSetVal(d, existing, val); // 替换
	dictFreeVal(d, &auxentry); // 通过val指针，释放掉val指向的内存
	//dictFreeVal(d, 
}

// 释放掉entry节点的val成员 可能指向的内存
#define dictFreeVal(d, entry) \
	if((d)->type->valDestructor) \
		(d)->type->valDestructor((d)->privDataPtr, (entry)->v.val)

void* dictFetchValue(dict* d, const void* key)
{
	dictEntry *he;
	he=dictFind(d, key);
	return he ? dictGetVal(he) : NULL;
}


#define dictGetVal(he) ((he)->v.val)

dictEntry* dictFind(dict* d, const void* key)
{
	dictEntry *he;
	uint64_t h, idx, table;
	
	if(d->ht[0].used + d->ht[1].used==0)
		return NULL;
	
	if(dictIsRehashing(d))
		_dictRehashStep(d);
	
	h=dictHash(d, key);
	for(table =0; table<=1; table++)
	{
		idx = h & d->ht[table].sizemask;
		he = d->ht[table].table[idx];
		while(he)
		{
			if(key==he->key  ||  dictCompareKeys(d, key, he->key))
				return he;
			he=he->next;
		}
		if(!dictIsRehashing(d))
			return NULL;
	}
	return NULL;
}

static void _dictRehashStep(dict* d)
{
	if(d->iterators==0) // TODO 
		dictRehash(d, 1);
}

// 进行n个桶的rehash，返回0表示完成rehash过程
int dictRehash(dict* d, int n)
{
	int empty_visits=n*10; // 允许访问的空桶的最大个数
	if(!dictIsRehashing(d))
		return 0;
	
	while(n--  &&  d->dt[0].used!=0) // 最多迁移n个有元素的桶
	{
		dictEntry *de, *nextde;
		
		assert(d->ht[0].size > (unsigned long)d->rehashidx);
		while(d->ht[0].table[d->rehashidx] == NULL) // 寻找下一个不是空的桶索引
		{
			d->rehashidx++;
			if(--empty_visits==0)
				return 1;
		}
		de=d->ht[0].table[d->rehashidx];
		while(de)
		{
			uint64_t h;
			nextde=de->next;
			h=dictHashKey(d, de->key) & d->ht[1].sizemask;
			de->next=d->ht[1].table[h];
			d->ht[1].table[h]=de;
			d->ht[0].used--;
			d->ht[1].used--;
			de=nextde;
		}
		d->ht[0].table[d->rehashidx]=NULL;
		d->rehashidx++;
	}
	
	if(d->ht[0].used==0)
	{
		zfree(d->ht[0].table);
		d->ht[0]=d->ht[1];
		_dictReset(&d->ht[1]);
		d->rehashidx=-1;
		return 0;
	}
	
	return 1;
}

// 从字典中随机返回一个键值对
dictEntry* dictGetRandomKey(dict *d)
{
	dictEntry *he， *origin;
	unsigned long h;
	int listlen, listele;
	
	if(dictSize(d)==0)
		return NULL;
	if(dictIsRehashing(d))
		_dictRehashStep(d);
	if(dictIsRehashing(d))
	{
		do{
			// 在rehashidx之后的索引范围内计算随机数
			h=d->rehashidx + (random() % (d->ht[0].size+d->ht[1].size-d->rehashidx));
			he= (h>=d->ht[0].size) ? d->ht[1].table[h-d->ht[0].size] : d->ht[0].table[h];
		}while(he==NULL);
	}
	else
	{
		do{
			h=random()  &  d->ht[0].sizemask;
			he=d->ht[0].table[h];
		}while(he==NULL);		
	}
	
	listlen=0;
	orighe=he;
	while(he)
	{
		he=he->next;
		listlen++;
	}
	listele=random() % listlen;
	he=origin;
	while(listele--)
		he=he->next;
	return he;
}

// 释放给定字典，以及字典中包含的所有键值对
void dictRelease(dict *d)
{
	_dictClear(d, &d->ht[0], NULL); // 释放字典d的哈希表
	_dictClear(d, &d->ht[1], NULL);
	zfree(d);
}

// 释放整个哈希表， 释放所有桶里面的所有entry的key和val的内存，释放所有entry内存
int _dictClear(dict *d, dictht *ht, void (callback)(void*))
{
	unsigned long i;
	
	for(i=0; i<ht->size  &&  ht->used>0; i++)
	{
		dictEntry *he, *nextHe;
		
		if(callback  &&  (i & 65535)==0) // 2^16-1 ,并且i是2^16的倍数
			callback(d->privdata);
		
		if((he=ht->table[i])==NULL)
			continue;
		while(he)
		{
			nextHe=he->next;
			dictFreeKey(d, he);
			dictFreeVal(d, he);
			zfree(he);
			ht->used--;
			he=nextHe;
		}
	}
	zfree(ht->table);
	_dictReset(ht);
	return 
}

// 从字典中删除给定键对应的键值对
int dictDelete(dict *ht, const void *key)
{
	return dictGenericDelete(ht, key, 0) ? DICT_OK : DICT_ERR;
}

/*查询和删除一个元素。*/
static dictEntry* dictGenericDelete(dict *d, const void* key, int nofree)
{
	uint64_t h, idx;
	dictEntry *he, *prevHe;
	int table;
	
	if(d->ht[0].used==0  &&  d->ht[1].used==0)
		return NULL;
	
	if(dictIsRehashing(d)) _dictRehashStep(d);
	h=dictHashKey(d, key);
	
	for(table =0; table<=1; table++)
	{
		idx=h  &  d->ht[table].sizemask;
		he=d->ht[table].table[idx];
		prevHe=NULL;
		while(he)
		{
			if(key==he->key  ||  dictCompareKeys(d, key, he->key))
			{
				if(prev)
					prevHe->next=he->next;
				else
					d->ht[table].table[idx]=he->next;
				if(!nofree)
				{
					dictFreeKey(d, he);
					dictFreeVal(d, he);
					zfree(he);
				}
				d->ht[table].used--;
				return he;
			}
			prevHe=he;
			he=he->next;
		}
		if(!dictIsRehashing(d))
			break;
	}
	return NULL;
}

// 返回字典ht中key对应的条目，并将其从其中移除
dictEntry* dictUnlink(dict* ht, void* key)
{
	return dictGenericDelete(ht, key, 1) ? DICT_OK : DICT_ERR;
}

void dictFreeUnlinkedEntry(dict *d, dictEntry *he)
{
	if(he==NULL)
		return;
	dictFreeKey(d, he);
	dictFreeVal(d, he);
	zfree(he);
}

// 清空字典
void dictEmpty(dict* d, void (callback)(void*))
{
	_dictClear(d, &d->ht[0], callback);
	_dictClear(d, &d->ht[1], callback);
	d->rehashidx=-1;
	d->iterators=0;
}

/*
字典 迭代器：
	dictNext
	dictGetIterator
	dictGetSafeIterator
	dictReleaseIterator

	
*/

// 迭代器结构体
typedef struct dictIterator{
	dict *d; // 当前字典
	long index; // 当前正遍历到 d->ht[table].table[index]桶
	int table, safe; // 字典的第几个哈希表, 0或者1
					/*safe设置为1，表示安全迭代器。
					安全迭代器表示在迭代的同时，可以进行dictAdd/dictFind等其他对字典的操作。
					非安全的迭代器只能在迭代时，执行dictNext操作。*/
	dictEntry *entry, *nextEntry;
	long long fingerprint; // 避免不安全的迭代器
}dictIterator;

dictIterator* dictGetIterator(dict* d)
{
	dictIterator* iter=zmalloc(sizeof(*iter));
	
	iter->d=d;
	iter->table=0;
	iter->index=-1;
	iter->safe=0;
	iter->entry=NULL;
	iter->nextEntry=NULL;
	return iter;
}

dictIterator* dictGetSafeIterator(dict *d)
{
	dictIterator* i=dictGetIterator(d);
	i->safe=1;
	return i;
}

dictEntry* dictNext(dictIterator* iter)
{
	while(1)
	{
		if(iter->entry==NULL) // 当前迭代器的entry为NULL，表示？？
		{
			dictht* ht=&iter->d->ht[iter->table];
			if(iter->index == -1  &&  iter->table==0) // 新创建的迭代器
			{
				if(iter->safe)
					iter->d->iterators++; // 要实现安全跌代器，需要记录字典当前的迭代器个数
				else // 非安全的迭代器
					iter->fingerprint = dictFingerprint(iter->d);
			}
			iter->index++; // 当前遍历的索引
			if(iter->index >= (long)ht->size)
			{
				if(dictIsRehashing(iter->d)  &&  iter->table==0)
				{
					iter->table++;
					iter->index=0;
					ht=&iter->d->ht[1];
				}
				else
				{
					break; // 遍历完成
				}
			}
			iter->entry = ht->table[iter->index];
		}
		else
		{
			iter->entry=iter->nextEntry; // 可能为NULL
		}
		
		if(iter->entry)
		{
			iter->nextEntry=iter->entry->next;
			return iter->entry;
		}
		// 当找到iter->entry，返回，否则继续迭代
	}
	return NULL;
}

/*
fingerprint是一个64位的数字，表示字典在某个时刻的状态。
通过一些字典的属性组合。
当初始化一个非安全的迭代器时，获得一个fingerprint，在释放迭代器的时候，检查fingerprint。
如果两个fingerprint不相同，表示用户在迭代非安全的迭代器的时候，执行了不允许的操作。
*/
/*
Result = hash(hash(hash(int1)+int2)+int3) ...
不同顺序的相同的整数集合计算的hash值是不同的。
*/
long long dictFingerprint(dict* d)
{
	long long integers[6], hash=0;
	int j;
	
	integers[0]=(long) d->ht[0].table;
	integers[1]=d->ht[0].size;
	integers[2]=d->ht[0].used;
	integers[3]=(long)d->ht[1].table;
	integers[4]=d->ht[1].size;
	integers[5]=d->ht[1].used;
	
	for(j=0; j<6; j++)
	{
		hash += integers[j];
		/* For the hashing step we use Tomas Wang's 64 bit integer hash. */
        hash = (~hash) + (hash << 21); // hash = (hash << 21) - hash - 1;
        hash = hash ^ (hash >> 24); 
        hash = (hash + (hash << 3)) + (hash << 8); // hash * 265
        hash = hash ^ (hash >> 14); 
        hash = (hash + (hash << 2)) + (hash << 4); // hash * 21
        hash = hash ^ (hash >> 28); 
        hash = hash + (hash << 31); 
	}
	return hash;
}

void dictReleaseIterator(dictIsRehashing *iter)
{
	// 不是刚刚初始化的迭代器，需要检查fingerprint
	if(!(iter->index==-1  &&  iter->table==0))
	{
		if(iter->safe)
			iter->d->iterators--;
		else
			assert(iter->fingerprint==dictFingerprint(iter->d));
	}
	zfree(iter);
}

