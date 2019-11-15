
// Nginx双向链表，ngx_queue_t

typedef struct ngx_queue_s ngx_queue_t;
struct ngx_queue_s{
	ngx_queue_t *prev;
	ngx_queue_t *next;
};
// 链表容器
// 链表元素

// 用法：
typedef struct{
	u_char* str;
	ngx_queue_t qEle;
	int num;
}TestNode;

ngx_int_t compTestNode(const ngx_queue_t* a, const ngx_queue_t* b)
{
	TestNode* aNode=ngx_queue_data(a, TestNode, qEle);
	TestNode* bNode=ngx_queue_data(b, TestNode, qEle);
	return aNode->num > bNode->num;
}

//ngx_queue_data() // 根据ngx_queue_t在完整队列结构体中的位置/命名，从ngx_queue_t指针获得完整结构体的指针
#define ngx_queue_data(q, type, link) \
	(type*)((u_char*)q - offsetof(type, link))
// size_t offsetof(type, member); 返回member在type结构体中的偏移量
#define offsetof(type, member) (size_t)&(((type*)0)->member)


ngx_queue_t queueContainer;
ngx_queue_init(&queueContainer);

#define ngx_queue_init(q) \
	(q)->prev=q; \
	(q)->next=q
/*初始化一个只包含一个节点的双向链表。*/

#define ngx_queue_insert_head(h, x) \
	(x)->next=(h)->next; \
	(x)->next->prev=x; \
	(x)->prev=h; \
	(h)->next=x
/*将x插入到h后面。*/

#define ngx_queue_insert_tail(h, x) \
	(x)->prev=(h)->prev; \
	(x)->prev->next=x; \
	(x)->next=h; \
	(h)->prev=x
/*将x插入到h前面。
	h是双向链表的头结点。h的前一个节点是最后一个节点。
*/

ngx_queue_t* q;
for(q=ngx_queue_head(&queueContainer); q!=ngx_queue_sentinel(&queueContainer); q=ngx_queue_next(q))
{
	TestNode* eleNode=ngx_queue_data(Q, TestNode, qEle);
	
}

#define ngx_queue_head(h) \
	(h)->next

#define ngx_queue_sentinel(h) \
	(h)
/*
双向链表的第一个节点不是第一个头结点。
头结点作为sentinel节点。
*/
#define ngx_queue_next(q) \
	(q)->next

ngx_queue_sort(&queueContainer, compTestNode);

void ngx_queue_sort(ngx_queue_t* queue, 
		ngx_int_t (*cmp)(const ngx_queue_t*, const ngx_queue_t *))
{
	ngx_queue_t *q, *prev, *next;
	q=ngx_queue_head(queue);
	
	if(q==ngx_queue_last(queue))
		return;
	
	// 插入排序
	for(q=ngx_queue_next(q); q!=ngx_queue_sentinel(queue); q=next)
	{
		prev=ngx_queue_prev(q);
		next=ngx_queue_next(q);
		
		ngx_queue_remove(q);
		
		do{
			if(cmp(prev, q)<=0)
				break;
			prev=ngx_queue_prev(prev);
		}while(prev!=ngx_queue_sentinel(queue));
		ngx_queue_insert_after(prev, q);
	}
}

#if (NGX_DEBUG)
#define ngx_queue_remove(x) \
	(x)->next->prev=(x)->prev; \
	(x)-prev->next=(x)->next; \
	(x)->next=NULL; \
	(x)->prev=NULL
#else
#define ngx_queue_remove(x) \
	(x)->next->prev=(x)->prev; \
	(x)->prev->next=(x)->next
#endif

/*
1, 当容器为空，prev和next都将指向容器本身。
空容器，包含一个ngx_queue_t结构体。

2，非空容器，头结点是ngx_queue_t结构体，也是sentinel。
通过prev和next指针，链接一条包含其他数据元素的结构体组成的双向链表。

*/



// ******************************************************************
// libevent TAILQ_QUEUE队列
#define TAILQ_HEAD(name, type) \
struct name{ \
	struct type* tqh_first; \
	struct type** tqh_last; \
}
// 队列头，tqh_first指向队列第一个元素，tqe_last指向最后一个元素的tqe_next成员。


#define TAILQ_ENTRY(type) \
struct { \
	struct type* tqe_next; \
	struct type** tqe_prev; \
}
// tqe_next指向队列中下一个节点，tqe_prev指向上一个节点的tqe_next成员。

#define TAILQ_FIRST(head) ((head)->tqh_first)
/*
TAILQ_HEAD(Container, ElmNode);
Container head;
ElmNode* first=TAILQ_FIRST(head);
*/
#define TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)
/*
struct ElmNode{
	TAILQ_ENTRY(ElmNode) field;
};

ElmNode elm;
ElmNode* next=TAILQ_NEXT(elm, field);
*/

#define TAILQ_INIT(head) do{ \
	(head)->tqh_first=NULL; \
	(head)->tqh_last=&(head)->tqh_first; \
}while(0)
/*
TAILQ_HEAD(Container, ElmNode);
Container head;
TAILQ_INIT(head);

当队列为空，队列头的first指针是NULL，last指向first指针。
当队列非空时，队列头的first指向第一个节点，last指向最后一个节点的next指针。
	第一个节点的prev指向first指针。最后一个节点的next是NULL。
*/

#define TAILQ_INSERT_TAIL(head, elm, field) do{ \
	(elm)->field.tqe_next=NULL; \
	(elm)->field.tqe_prev=(head)->tqh_last; \
	*(head)->tqh_last=(elm); \
	(head)->tqh_last=&(elm)->field.tqe_next; \
}while(0)
/*先修改elm之前一个元素的next指针，后修改队列头的last指针*/

// #define TAILQ_INSERT_HEAD(head, elm, field) do{ \
	// (elm)->field.tqe_next=head->tqh_first; \
	// (elm)->field.tqe_prev=NULL; \ // ???
	// (head)->tqh_first=(elm); \
	// (head)
// }
#define TAILQ_INSERT_HEAD(head, elm, field) do{\
	if(((elm)->field.tqe_next = (head)->tqh_first)) != NULL) \
		(head)->tqh_first->field.tqe_prev = &(elm)->field.tqe_next; \
	else \
		(head)->tqh_last= &(elm)->field.tqe_next; \
	(head)->tqh_first=(elm); \
	(elm)->field.tqe_prev=&(head)->tqh_first; \
}while(0)
/*
当队列为空时，需要修改队列头的last指针。
(当队列不为空时，修改队列第一个元素的prev指向新插入节点。)
队列的第一个元素的prev，始终指向队列头的first指针。
*/


/*#define TAILQ_REMOVE(head, elm, field) do{ \
	if((elm)->field.tqe_prev==(head)->tqe_first) \
		(head)->tqh_first=(elm)->next; \
	if((head)->tqh_last==&((elm)->field.tqh_next)) \
		(head)->tqh_last=
		
	(elm)->field.tqe_prev=(elm)->field.tqe_next;
	
}
删除一个节点elm：
	当被删除节点是第一个节点，需要更新first指针。
	当被删除节点是最后一个节点，需要更新last指针。
	当存在前一个节点，前一个节点的next需要更新为elm的下一个节点。
	当存在后一个节点，后一个节点的prev更新为elm的prev。
	
正确，删除一个节点elm：
	如果elm不是最后一个节点，更新elm的后一个节点的prev。
	否则，更新last指针。
		当elm是唯一的节点，elm的prev指向first指针，此时更新last指向first。
		当elm有前一个节点，elm的prev指向前一个节点的next指针，此时更新last指向前一个节点的next指针。
	最后，更新elm的prev指针的内容。
		不需要特殊处理elm是否是第一个节点?:
		当elm是第一个节点时：elm的prev指向first指针，需要更新(其内容|first指针)为elm的next。
		当不是的时候：elm的prev指向前一个节点的next指针，同样需要更新其内容(此next)为elm的next。
*/

#define TAILQ_REMOVE(head, elm, field)  do{ \
	if(((elm)->field.tqe_next)==NULL) \
		(head)->tqh_last=(elm)->field.tqe_prev; \
	else \
		(elm)->field.tqe_next->field.tqe_prev=(elm)->field.tqe_prev; \
	*(elm)->field.tqe_prev=(elm)->field.tqe_next; \
}while(0)


// 使用示例：
struct queue_entry_t{
	int value;
	TAILQ_ENTRY(queue_entry_t)entry;
};

/*
	struct {
		struct queue_entry_t *tqe_next;
		struct queue_entry_t **tqe_prev;
	}entry;
*/

TAILQ_HEAD(queue_head_t, queue_entry_t);
/*
	struct queue_head_t{
		struct queue_entry_t *tqh_first;
		struct queue_entry_t **tqh_last;
	};
*/

int main()
{
	struct queue_head_t queue_head;
	struct queue_entry_t *q, *p, *s, *new_item;
	int i;
	
	TAILQ_INIT(&queue_head);
	/*
	queue_head.tqh_first=NULL;
	qeuee_head.tqh_last=&(queue_head.tqh_first);
	*/
	
	for(i=0; i<3; ++i)
	{
		p=(struct queue_entry_t*)malloc(sizeof(struct queue_entry_t));
		p->value=i;
		TAILQ_INSERT_TAIL(&queue_head, p, entry);
		/*
		将节点p插入到队列queue_head的尾部：
		p->entry.tqe_prev=queue_head.tqh_last;
		p->entry.tqe_next=NULL;
		*(queue_head.tqh_last)=p;
		queue_head.tqh_last=&(p->entry.tqe_next);
		*/
	}
	
	q=(struct queue_entry_t*)malloc(sizeof(struct queue_entry_t));
	q->value=10;
	TAILQ_INSERT_HEAD(&queue_head, q, entry);
	/*
	将节点p插入到队列queue_head的头部
	if(queue_head.tqh_first==NULL)
		queue_head.tqh_last=&(q.entry.tqe_next);
	else
		queue_head.tqh_first->entry.tqe_prev=&(q.entry.tqe_next);
	q.entry.tqe_next=queue_head.tqh_first;	
	q.entry.tqe_prev=&(queue_head.tqh_first);
	queue_head.tqh_first=q;
	*/
	
	s=(struct queue_entry_t*)malloc(sizeof(struct queue_entry_t));
	s->value=20;
	TAILQ_INSERT_AFTER(&queue_head, q, s, entry);
	/*
	将节点s插入到节点q的后面
	if(q->entry.tqe_next==NULL)
		queue_head.tqh_last=&(s.entry.tqe_next);
	else
		q->entry.tqe_next->entry.tqe_prev=&(s.entry.tqe_next);
	s.entry.tqe_prev=&(q.entry.tqe_next);
	s.entry.tqe_next=q.entry.tqe_next;
	q.entry.tqe_next=s;
	*/
	
	s=(struct queue_entry_t*)malloc(sizeof(struct queue_entry_t));
	s->value=30;
	TAILQ_INSERT_BEFORE(p, s, entry);
	/*
	将节点s插入到节点p之前
	s.entry.tqe_prev=p.entry.tqe_prev;
	s.entry.tqe_next=p;
	*(p.entry.tqe_prev)=s;
	p->entry.tqe_prev=&(s->entry.tqe_next);
	*/
	
	s=TAILQ_FIRST(&queue_head);
	/*
	s=queue_head->tqh_first;
	*/
	
	s=TAILQ_NEXT(s, entry);
	/*
	s=s.entry.tqe_next;
	*/
	
	TAILQ_REMOVE(&queue_head, s, entry);
	free(s);
	/*
	从队列queue_head中删除节点s
	//if(queue_head.tqh_last=&(s.entry.tqe_next))
	if(s.entry.tqe_next==NULL)
		queue_head.tqh_last=s.entry.tqe_prev;
	else
		s.entry.tqe_next->entry.tqe_prev=s.entry.tqe_prev;
	*(s.entry.tqe_prev)=s.entry.tqe_next;
	*/
	
	new_item=(struct queue_entry_t*)malloc(sizeof(struct queue_entry_t));
	new_item->value=100;
	
	s=TAILQ_FIRST(&queue_head);
	TAILQ_REPLACE(&queue_head, s, new_item, entry);
	/*
	用new_item替换队列queue_head中的节点s：
	new_item.entry.tqe_prev=s.entry.tqe_prev;
	new_item.entry.tqe_next=s.entry.tqe_next;
	new_item.entry.tqe_next->entry.tqe_prev=&(new_item.entry.tqe_next); //错误，需要判断是是否更新tqh_last
	*(new_item.entry.tqe_prev)=new_item;
	
	//s.entry.tqe_next=NULL;
	//*(s.entry.tqe_prev)=NULL; 
	//s.entry.tqe_prev=NULL;
	*/
	
	/*
	使用节点elm2代替节点:	
	#define TAILQ_REPLACE(head, elm, elm2, field) do{\
		if(((elm)->field.tqe_next) !=NULL) \
			(elm2)->field.tqe_next->field.tqe_prev=&(elm2->field.tqe_next);
		else
			(head)->tqh_last=&(elm2->field.tqe_next);
		(elm2)->field.tqe_next=(elm)->field.tqe_next;
		(elm2)->field.tqe_prev=(elm)->field.tqe_prev;
		*((elm2)->field.tqe_next)=(elm2);
	}while(0)
	*/
	
	
	i=0;
	TAILQ_FOREACH(p, &queue_head, entry)
	{
		printf("the %dth entry is %d\n", i++, p->value);
	}
	/*
	for(p=queue_head.tqh_first; p!=NULL; p=p->entry.tqe_next)
	*/

	p=TAILQ_LAST(&queue_head, queue_head_t);
	printf("last is %d\n", p->value);
	/*
	
	p=TAILQ_LAST(&queue_head, queue_entry_t);
	
	queue_entry_t **tmp=queue_head.tqh_last;
	p=(queue_entry_t*)((char*)tmp-offsetof(queue_entry_t, entry));
	
	???
	*/
	
	p=TAILQ_PREV(p, queue_head_t, entry);
	printf("the entry before last is %d\n", p->value);
	/*
	queue_entry_t **tmp=p->entry.tqe_prev;
	p=(queue_entry_t*)((char*)tmp-offsetof(queue_entry_t, entry));
	
	???
	*/
}

#define TAILQ_FOREACH(var, head, field) \
	for((var)=TAILQ_FIRST(head); (var)!=TAILQ_END(head); (var)!=TAILQ_NEXT(var, field))
/*
	for((var)=head->tqh_first; (var)!=NULL; (var)!=(var)->entry.tqe_next)
*/

#define TAILQ_END(head) NULL
#define TAILE_EMPTY(head) \
	return (TAILQ_FIRST(head)==TAILQ_END(head))
/*
	return (head->tqh_first==NULL)
*/

/*
TAILQ_PREV
TAILQ_LAST
*/
#define TAILQ_LAST(head, headname) \
	(*(((struct headname*)((head)->tqh_last))->tqh_last))
/*
【
(head)->tqh_last指向的是最后一个节点的tqe_next变量。
将此位置开始的内存使用一个struct headname指针指向，
	将导致紧随tqe_next变量之后的tqe_prev变量被看做一个匿名的struct headname结构体的tqh_last成员。】
	访问此tqh_last变量，等价于访问tqe_prev变量。
	因为tqe_prev变量，是最后一个节点的tqe_prev成员，指向的是倒数第二个节点的tqe_next成员的地址，
		对其解引用的指针就是最后一个节点的地址。

p=TAILQ_LAST(&queue_head, queue_head_t);
p = *(((struct queue_head_t*)((&queue_head)->tqh_last))->tqh_last);
*/

// #define TAILQ_LAST(head, entryname) \

#define TAILQ_PREV(elm, headname, field) \
	(*(((struct headname*)((elm)->field.tqe_prev))->tqh_last))
/*
elm的tqe_prev指向的是前一个节点的tqe_next成员。
将tqe_prev指针强制转换成headname指针，使得通过headname指针访问的tqh_last成员，
	就是访问前一个节点的tqe_prev成员。
	此tqe_prev指向的是再往前一个节点的tqe_next/tqe_first地址，解引用就是前一个节点的地址。

当elm是第一个节点，elm的tqe_prev指向的是tqh_first变量，也就是真正的headname结构体。
	此时，通过headname指针访问的tqh_last成员，就是真正的tqh_last成员。
	tqh_last成员指向的最后一个节点的tqe_next成员，其值就是NULL。
*/


#define TAILQ_FOREACH_REVERSE(var, head,headname,  field) \
	for((var)=TAILA_LAST(head, headname); (var)!=TAILQ_END(head); (var)=TAILQ_PREV(var, headname, field))



