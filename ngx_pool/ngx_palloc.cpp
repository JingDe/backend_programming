
#include"ngx_palloc.h"

ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log)
{
    ngx_pool_t *p;

    //根据目标平台，使用posix_memalign/memalign/malloc
    p=ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
    if(p==NULL)
        return NULL;

    // last指向ngx_pool_t结构体之后的内存
    p->d.last=(unsigned char*)p+sizeof(ngx_pool_t);
    p->d.end=(unsigned char*)p+size;
    p->d.next=NULL;
    p->d.failed=0;

    size=size-sizeof(ngx_pool_t);
    p->max=(size<NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    p->current=p;
    p->chain=NULL;
    p->large=NULL;
    p->cleanup=NULL;
    p->log=log;

    return p;
}

void ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_pool_t *p, *n;
    ngx_pool_large_t *l;
    ngx_pool_cleanup_t *c;

    // 清理资源
    for(c=pool->cleanup; c; c=c->next)
    {
        if(c->handler)
        {
            LOG_DEBUG<<"run cleanup "<<c;
            c->handler(c->data);
        }
    }

#if (NGX_DEBUG)
    // 打印释放大内存和小内存的信息，（log可能在释放过程中释放）
    for(l=pool->large; l; l=l->next)
    {
        LOG_DEBUG<<"free "<<l->alloc;
    }
    for(p=pool, n=pool->d.next; ; p=n, n=n->d.next)
    {
        LOG_DEBUG<<"free "<<p<<", unused "<<p->d.end-p->d.last;
        if(n==NULL)
            break;
    }
#endif

    for(l=pool->large; l; l=l->next)
    {
        if(l->alloc)
            free(l->alloc);
    }

    for(p=pool, n=pool->d.next; ; p=n; n=n->d.next)
    {
        free(p);
        if(n==NULL)
            break;
    }
}

void* ngx_palloc(ngx_pool_t *pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC) // 非debug模式，分配小内存
    if(size<=pool->max)
        return ngx_palloc_small(pool, size, 1);
#endif

    return ngx_palloc_large(pool, size);
}

static void *ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
    void *p;
    unsigned int n;
    ngx_pool_large_t *large;

    p=ngx_alloc(size, pool->log);
    
}
