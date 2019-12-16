
// item结构体定义
typedef struct _stritem{
	// 用户LRU链表
	struct _stritem *next;
	struct _stritem *prev;
	// 用于哈希表冲突链
	struct _stritem *h_next; 
	
	// 属于哪一类item
	uint8_t slabs_clsid;
}item;

/*memcached的每一类的item，使用一个lru队列。
LARGEST_ID是不同大小item的个数。
heads[i]指向第i类LRU链表的第一个item，
size[i]表示第i类LRU链表的item的个数。
*/
static item* heads[LARGEST_ID];
static item* tails[LARGEST_ID];
static unsigned int sizes[LARGEST_ID];

// 将item插入lru链表表头
static void item_link_q(item* it)
{
	item **head, **tail;
	assert(it->slabs_clsid <  LARGEST_ID);
	assert((it->it_flags  &  ITEM_SLABBED) ==0);
	
	head=&heads[it->slabs_clsid];
	tail=&tails[it->slabs_clsid];
	
	it->prev=0;
	it->next=*head;
	if(it->next)
		it->next->prev=it;
	*head=it;
	if(*tail == 0)
		*tail =it;
	sizes[it->slabs_clsid]++;
}

static void item_unlink_q(item *it)
{
	item **head, **tail;
	head=&heads[it->slabs_clsid];
	tail=&tails[it->slabs_clsid];
	
	if(*head==it)
	{
		*head=it->next;
	}
	if(*tail==it)
	{
		*tail=it->prev;
	}
	
	if(it->next)
		it->next->prev=it->prev;
	if(it->prev)
		it->prev->next=it->next;
	sizes[it->slabs_clsid]--;
}

void do_item_update(item *it)
{
	if(it->time < current_time - ITEM_UPDATE_INTERVAL)
	{
		if((it->it_flags  &  ITEM_LINKED) != 0)
		{
			item_unlink_q(it);
			it->time=current_time;
			item_link_q(it);
		}
	}
}
