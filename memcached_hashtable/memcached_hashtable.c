

typedef struct _stritem{

	struct _striem *h_next; // 同一个桶中的item（具有相同哈希值的item），通过h_next相连
	uint8_t nkey; // key的长度
	
	union{
		uint64_t cas;
		char end;
	}data[];
	/* if it_flags & ITEM_CAS we have 8 bytes CAS */
    /* then null-terminated key */
    /* then " flags length\r\n" (no terminating null) */ 
    /* then data with terminating \r\n (no terminating null; it's binary!) */
}item;

// item的data成员的地址，就是key存储的首地址
#define ITEM_key(item) (((char*)&((item)->data)) + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))


static size_t item_make_header(const uint8_t nkey, const int flags, const int nbytes, char* suffix, uint8_t *nsuffix)
{
	*nsuffix = (uint8_t) snprintf(suffix, 40, " %d %d\r\n", flags, nbytes-2);
	return sizeof(item) + nkey+ *nsuffix + nbytes;
}

// item* do_item_alloc(char* key, const size_t nkey, const int flags, const rel_time_t exptime, const int nbytes, const uint32_t cur_hv)
// {
	// uint8_t nsuffix;
	// item* it=NULL;
	// char suffix[40];
	// size_t ntotal=item_make_header(nkey+1, flags, nbytes, suffix, &nsuffix);
	// if(settings.use_cas)
		// ntotal+=sizeof(uint64_t);
	
	// unsigned int id=slabs_clsid(ntotal);
	// if(id==0)
		// return 0;
	
	
	
	// it=slabs_alloc(ntotal, id); // 从第id个slab分配器中分配ntotal大小的内存
	// it->refcount=1;
	// it->it_flags=settings.use_cas ? ITEM_CAS : 0;
	// it->nkey=nkey;
	// it->nbytes=nbytes;
	// memcpy(ITEM_key(it), key, nkey);
	// memcpy(ITEM_suffix(it), suffix, (size_t)nsuffix);
	// it->nsuffix=nsuffix;
	// return it;
// }

/*-----------------------------------------------------------------*/

#define hashsize(n) ((unsigned long int)1<<(n))
#define hashmask(n) (hashsize(n)-1)


static item** primary_hashtable=NULL;
static item** old_hashtable=NULL;

bool expanding=false;
int expand_bucket=-1;

static int hashpower;


// item插入到哈希表中
int assoc_insert(item *it, const uint32_t hv)
{
	unsigned int oldbucket;
	
	if(expanding  &&  (oldbucket=(hv & hashmask(hashpower-1))) >= expand_bucket)
	{
		it->h_next = old_hashtable[oldbucket];
		old_hashtable[oldbucket]=it;
	}
	else
	{
		// unsigned int newbucket = (hv &  hashmask(hashpower*2-1));
		// it->h_next = new_hashtable[newbucket];
		// new_hashtable[newbucket]=it;
		it->h_next=primary_hashtable[hv & hashmask(hashpower)];
		primary_hashtable[hv & hashmask(hashpower)]=it;
	}
	
	hash_items ++;
	if(!expanding  &&  hash_items>(hashsize(hashpower)*3)/2)
		asso_start_expand();
	
	return 1;
}

// 查找key对应的item*, key的长度是nkey, key的哈希值是hv
// item* assoc_find(const char* key, const size_t nkey, const uint32_t hv)
// {
	// item* head;
	
	// unsigned int oldbucket;
	// if(expanding  &&  (oldbucket=(hv  &  hashmask(hashpower-1))) >= expand_bucket)
	// {
		// head = old_hashtable[oldbucket];
	// }
	// else
	// {
		// head=primary_hashtable[hv  &  hashmask(hashpower)];
	// }
	
	// while(head)
	// {
		// if(head->nkey==nkey  &&  )
	// }
// }

// 哈希表中查找item
item* assoc_find(const char* key, const size_t nkey, const uint32_t hv)
{
	item *it;
	unsigned int oldbucket;
	if(expanding  &&  (oldbucket=(hv  &  hashmask(hashpower-1))) >= expand_bucket)
	{
		it = old_hashtable[oldbucket];
	}
	else
	{
		it =primary_hashtable[hv  &  hashmask(hashpower)];
	}
	
	item *ret=NULL;
	int depth=0;
	while(it)
	{
		if(nkey==it->nkey  &&  (memcmp(key, ITEM_key(it), nkey)==0))
		{
			ret=it;
			break;
		}
		it=it->h_next;
		++depth;
	}
	return ret;
}

/* 从哈希表中删除key */
// void assoc_delete(const char* key, const size_t nkey, const uint32_t hv)
// {
	// item *it;
	// unsigned int oldbucket;
	// if(expanding  &&  (oldbucket=(hv  &  hashmask(hashpower-1))) >= expand_bucket)
	// {
		// it = old_hashtable[oldbucket];
	// }
	// else
	// {
		// it =primary_hashtable[hv  &  hashmask(hashpower)];
	// }
	
	// item *head;=it;
	// item *prev=NULL;
	// while(it)
	// {
		// if(nkey==it->nkey  &&  (memcmp(key, ITEM_key(it), nkey)==0))
		// {
			//删除节点it
			// if(prev==NULL)
			// {
				// head->h_next=it->h_next;				
			// }
			// else
			// {
				// prev->h_next=it->h_next;
			// }
			// delete it;
			// break;
		// }
		// prev=it;
		// it=it->h_next;
	// }
// }


void assoc_delete(const char* key, const size_t nkey, const uint32_t hv)
{
	item **before=_hashitem_before(key, nkey, hv); // 获得指向key节点指针的指针
	
	if(*before) // *before就是key对应节点，存在
	{
		item *nxt;
		hash_items--;
		
		nxt=(*before)->h_next; // key之后的节点
		(*before)->h_next=0; // 
		*before=nxt; // 修改被删除节点的内容，为后一个节点的地址
		return ;
	}
	assert(false);
	assert(*before!=NULL);
}


/*返回指向key对应节点指针的指针，也就是 key之前节点的h_next的地址 */
static item** _hashitem_before(const char *key, const size_t nkey, const uint32_t hv)
{
	item **pos;
	unsigned int  oldbucket;
	if(expanding  &&  (oldbucket=(hv  &  hashmask(hashpower-1))) >= expand_bucket)
	{
		pos = &old_hashtable[oldbucket]; // pos保存对应链表的头结点的地址
	}
	else
	{
		pos = &primary_hashtable[hv  &  hashmask(hashpower)];
	}
	
	while(*pos  &&  ((nkey!=(*pos)->nkey)  ||  memcmp(key, ITEM_key(*pos), nkey)))
	{
		pos = &(*pos)->h_next; // pos移动到，指向 当前匹配失败节点的h_next成员
	}
	
	// *pos等于NULL，或者 *pos指向的节点就是key对应节点
	return pos;	
}



static void* assoc_maintenance_thread(void* arg)
{
	while(do_run_maintenance_thread)
	{
		int ii=0;
		
		item_lock_global();
		mutex_lock(&cache_lock);
		
		for(ii=0; ii < hash_bulk_move  &&  expanding; ++ii)
		{
			item *it, *next;
			int bucket;
			
			// 依次迁移expand_bucket桶中的每个节点
			for(it=old_hashtable[expand_bucket]; NULL!=it; it=next)
			{
				next=it->h_next;
				
				bucket=hash(ITEM_key(key), it->nkey) & hashmask(hashpower); // 新的桶号
				it->h_next=primary_hashtable[bucket]; // 插入表头
				primary_hashtable[bucket]=it;
			}
			
			old_hashtable[expand_bucket]=NULL;
			expand_bucket++;
			if(expand_bucket == hashsize(hashpower-1))
			{
				expanding =false;
				free(old_hashtable);
				
			}
		}
		
		mutex_unlock(&cache_lock);
		item_unlock_global();
		
		if(!expanding)
		{
			// switch_item_lock_type(ITEM_LOCK_GRANULAR);
			// slabs_rebalancer_resume();
			
			mutex_lock(&cache_lock);
			started_expanding=false;
			pthread_cond_wait(&maintenance_cond, &cache_lock); 
			/*挂起迁移线程*/
			mutex_unlock(&cache_lock);
			
			//
			mutex_lock(&cache_lock);
			assoc_expand(); // 迁移线程被唤醒，申请更大的哈希表，把expanding设置为true
			mutex_unlock(&cache_lock);
		}
	}
	return NULL;
}

// 扩展哈希表的大小
// static void assoc_expand()
// {
	// old_hashtable=primary_hashtable;
	
	// hashpower++;
	// primary_hashtable=new item*[hashsize(hashpower)];
	
	// expand_bucket=0;
	// expanding=true;
// }

static void assoc_expand()
{
	old_hashtable=primary_hashtable;
	
	primary_hashtable=calloc(hashsize(hashpower+1), sizeof(void*));
	if(primary_hashtable)
	{
		hashpower++;
		expanding=true;
		expand_bucket=0;
	}
	else
	{
		primary_hashtable=old_hashtable;
	}
}



