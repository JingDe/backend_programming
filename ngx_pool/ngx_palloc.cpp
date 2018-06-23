
#include"ngx_palloc.h"
#include"ngx_alloc_unix.h"
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#include"logging.muduo/Logging.h"

unsigned int ngx_pagesize=getpagesize();// <unistd.h>获得内存页面大小

static inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size, unsigned int align);
static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size);

ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log)
{
    ngx_pool_t *p;

    //根据目标平台，使用posix_memalign/memalign/malloc
    p=(ngx_pool_t*)ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
    if(p==NULL)
        return NULL;
    LOG_DEBUG<<"ngx_memalign "<<p;

    // last指向ngx_pool_t结构体之后的内存
    p->d.last=(unsigned char*)p+sizeof(ngx_pool_t);
    LOG_DEBUG<<"pool struct "<<sizeof(ngx_pool_t)<<", last"<<p->d.last;
    p->d.end=(unsigned char*)p+size;
    p->d.next=NULL;
    p->d.failed=0;

    size=size-sizeof(ngx_pool_t);
    p->max=(size<NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    p->current=p;
    //p->chain=NULL;
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

    for(p=pool, n=pool->d.next; ; p=n, n=n->d.next)
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

    // debug模式下，均分配堆内存
    return ngx_palloc_large(pool, size);
}

// 分配大块、堆内存
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
    void *p;
    unsigned int n;
    ngx_pool_large_t *large;

    p=ngx_alloc(size, pool->log);// C库malloc
    if(p==NULL)
        return NULL;

     n=0;//只搜寻前4个large结点，寻找可复用。

     // 将分配的堆内存添加到现有的large链表的结构体中
     for(large=pool->large; large; large=large->next)
     {
         if(large->alloc==NULL)
         {
             large->alloc=p;
             return p;
         }
         if(n++>3)
            break;
     }

     // 创建新的large结构体，插入到large链表首节点
     // large结构体从小内存池分配
     large=(ngx_pool_large_t*)ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
     if(large==NULL)
     {
         free(p);
         return NULL;
     }
     large->alloc=p;
     large->next=pool->large;
     pool->large=large;

     return p;
}

// 从current开始搜寻小内存池分配size字节，返回地址
// 若不存在满足大小的小内存池，调用ngx_palloc_block
static inline void*
ngx_palloc_small(ngx_pool_t *pool, size_t size, unsigned int align)
{
    LOG_DEBUG<<"in ngx_palloc_small";
    unsigned char *m;
    ngx_pool_t *p;

    p=pool->current;
    do{
        m=p->d.last;
        LOG_DEBUG<<"ngx_align_ptr (before)"<<m;
        if(align)
            m=ngx_align_ptr(m, NGX_ALIGNMENT);//从m地址开始找到一个对齐的内存起始地址
        LOG_DEBUG<<"ngx_align_ptr (get)"<<m;

        if((size_t)(p->d.end-m) >=size)
        {
            LOG_DEBUG<<"found small "<<m<<"~"<<m+size;
            p->d.last=m+size;
            return m;
        }

        p=p->d.next;
    }while(p);

    LOG_DEBUG<<"no enough small";
    return ngx_palloc_block(pool, size);
}

// 预分配一块堆内存，作为pool结构体和可用小内存池，并从内存池中分配出去size字节
static void*
ngx_palloc_block(ngx_pool_t *pool, size_t size)
{
    LOG_DEBUG<<"in ngx_palloc_block";
    unsigned char *m;
    size_t psize;
    ngx_pool_t *p, *newp;

    // 获得一个pool预分配堆内存的大小（包括pool结构体和可用内存）
    psize=(size_t)(pool->d.end-(unsigned char*)pool);

    m=(unsigned char*)ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log);
    if(m==NULL)
        return NULL;
    LOG_DEBUG<<"in ngx_palloc_block : ngx_memalign "<<m;
    newp=(ngx_pool_t*)m;
    newp->d.end=m+psize;
    newp->d.next=NULL;
    newp->d.failed=0;

    // 从新创建的pool中分配一个首地址对齐的size字节内存
    m+=sizeof(ngx_pool_data_t);
    LOG_DEBUG<<"after pool struct "<<m;
    m=ngx_align_ptr(m, NGX_ALIGNMENT);
    LOG_DEBUG<<"align get "<<m;
    newp->d.last=m+size;
    LOG_DEBUG<<"asign "<<size<<", last "<<newp->d.last;

    // 将新分配的pool插入到pool链表末尾
    // 更新current指针为当前current开始第一个failed不超过4的pool结点
    for(p=pool->current; p->d.next; p=p->d.next)
    {
        if((p->d.failed)++ > 4)
            pool->current=p->d.next;
    }
    p->d.next=newp;
    LOG_DEBUG<<"pool->current "<<pool->current;

    return m;
}

void ngx_reset_pool(ngx_pool_t *pool)
{

}

void *ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
    return NULL;
}
void *ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    return NULL;
}
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment)
{
    return NULL;
}
int ngx_pfree(ngx_pool_t *pool, void *p)
{
    return 0;
}


// 添加资源释放方法
ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size)
{
    ngx_pool_cleanup_t *c;

    c=(ngx_pool_cleanup_t *)ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
    if(c==NULL)
        return NULL;

    if(size)
    {
        c->data=ngx_palloc(p, size);
        if(c->data==NULL)
            return NULL;
    }
    else
    {
        c->data=NULL;
    }
    c->handler=NULL;
    c->next=p->cleanup;
    p->cleanup=c;
    LOG_DEBUG<<"add cleanup "<<c;
    return c;
}

void ngx_pool_run_cleanup_file(ngx_pool_t *p, int fd)
{
    ngx_pool_cleanup_t *c;
    ngx_pool_cleanup_file_t *cf;

    for(c=p->cleanup; c; c=c->next)
    {
        if(c->handler== ngx_pool_cleanup_file)
        {
            cf=(ngx_pool_cleanup_file_t *)c->data;
            if(cf->fd==fd)
            {
                c->handler(cf);
                c->handler=NULL;
                return ;
            }
        }
    }
}

// TODO 跨平台
void ngx_pool_cleanup_file(void *data)
{
    ngx_pool_cleanup_file_t *c=(ngx_pool_cleanup_file_t *)data;

    LOG_DEBUG<<"file cleanup "<<c->fd;

    if(close(c->fd)==-1)
    {
        LOG_ERROR<<"close failed "<<c->name;
    }
}

// unlink + close
// TODO 跨平台
void ngx_pool_delete_file(void *data)
{
    ngx_pool_cleanup_file_t *c=(ngx_pool_cleanup_file_t*)data;

    int err;

    LOG_DEBUG<<"file cleanup "<<c->fd<<", "<<c->name;

    if(unlink((const char*)c->name)==-1) // 从文件系统删除文件名
    {
        err=errno;

        if(err!=ENOENT)
            LOG_ERROR<<"unlink "<<c->name<<" failed "<<strerror(errno);
    }
    // 文件名是最后一个连接，保留到最后一个close，之后被释放

    if(close(c->fd)==-1)
        LOG_DEBUG<<"close "<<c->name<<" failed "<<strerror(errno);
}
