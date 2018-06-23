/*
nginx内存池实现：

    考虑内存池的生命周期，释放内存池

    每个内存池对象ngx_pool_t，包含小内存池ngx_pool_data_t和大内存池。

    各个内存池对象，通过表示小内存池的成员ngx_pool_data_t的next指针相连。
    内存池对象的current成员，指向当前考虑分配的内存池对象。

    默认小块内存的最大大小，一个页面大小
    超过的为大内存分配，直接从内存堆中分配

    每个pool预先从进程堆中分配。
    当需要较大内存时，从堆中分配，将分配地址添加到large链表可复用结点中alloc值。
        或者从小内存池中创建large结构体，插入large头结点。
    当需要较小内存时，从current开始搜寻小内存池，分配出去。
        若不存在，从堆中分配一个新的pool，分配指定字节，并插入pool链表尾
*/
#ifndef NGX_PALLOC_H_
#define NGX_PALLOC_H_

#include<cstdint>
#include<cstddef>

#define NGX_POOL_ALIGNMENT       16
// 默认小块内存的最大大小，一个页面大小
extern unsigned int ngx_pagesize;
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)

#ifndef NGX_ALIGNMENT
#define NGX_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#endif

#define ngx_free free

// 返回从p开始的a对齐的地址
#define ngx_align_ptr(p, a)  \
    (unsigned char*)(  ((uintptr_t)(p)+((uintptr_t)a-1)) & ~((uintptr_t)a-1)  )


class ngx_log_t;








// 资源销毁方法
typedef void (*ngx_pool_cleanup_pt)(void *data);
typedef struct ngx_pool_cleanup_s ngx_pool_cleanup_t;

struct ngx_pool_cleanup_s{
    ngx_pool_cleanup_pt handler;//handler初始化为NULL，需要设置为清理方法
    void *data;//ngx_pool_cleanup_add方法的size>0时，data不为NULL。此时改写data指向的内存。
    //用于为handler指向的方法传递必要的参数。
    ngx_pool_cleanup_t *next;//由ngx_pool_cleanup_add方法设置next成员，
    //用于将当前ngx_pool_cleanup_t添加到ngx_pool_t的cleanup链表中
};


// 从内存堆中分配的大块内存
typedef struct ngx_pool_large_s ngx_pool_large_t;

struct ngx_pool_large_s{
    ngx_pool_large_t *next;//连接大块内存的指针
    void *alloc;//指向分配除的大块内存，调用ngx_free后alloc可能是NULL
    // 复用ngx_pool_large_s结构
};

// 一个内存池结构
// 新增的ngx_pool_t结构体中与小块内存无关的其他成员是无意义的，包括max、large、cleanup等。
typedef struct ngx_pool_s ngx_pool_t;

// 小块内存池结构
typedef struct{
    unsigned char *last;//指向未分配的空闲内存的首地址
    unsigned char *end;//指向当前小块内存池的尾部
    ngx_pool_t *next;//同属于一个pool的多个小块内存池间，通过next相连
    unsigned int failed;//每当剩余空间不足以分配出小块内存时，failed加1
    //failed大于4后，ngx_pool_t的current将移向下一个小块内存池
}ngx_pool_data_t;

struct ngx_pool_s{
    // 不是指针
    ngx_pool_data_t d;// 小块内存池，通过d中的next成员构成单链表
    size_t max;// 评估申请内存是大块还是小块的标准
    ngx_pool_t *current;//多个小块内存池构成链表时，current指向分配内存时遍历的第一个小块内存池
    //ngx_chain_t *chain;//
    ngx_pool_large_t *large;//大块内存都直接从进程堆中分配。为了能够在销毁内存池时同时释放大块内存，
                    //就把每一次分配的大块内存通过ngx_pool_large_t组成单链表
    ngx_pool_cleanup_t *cleanup;//所有待清理资源构成单链表
    ngx_log_t *log;//内存池执行中输出日志的对象
};

/*
一个pool结构示意图：通过d.next形成链表

    d：
        last——last与end之间是可分配内存
        end
        next ------------------->下一个pool结构地址（之后的pool结构只有d成员有效）
        failed
    max大小内存的界限
    current---------->当前可分配的pool结构地址
    large------------>堆内存链表,通过next相连
    cleanup---------->清理资源链表,通过next相连
    log日志对象指针

    //内存中，之后是预分配内存首地址、last指针、end指针
*/

typedef struct{
    int fd;
    char *name;
    ngx_log_t *log;
}ngx_pool_cleanup_file_t;


ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log);
void ngx_destroy_pool(ngx_pool_t *pool);
// 重置内存池，释放大内存，重用小内存
void ngx_reset_pool(ngx_pool_t *pool);

// 分配地址对齐内存
void *ngx_palloc(ngx_pool_t *pool, size_t size);
// 分配内存时不进行地址对齐
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);
// 分配地址对齐内存后，初始化内存为0
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);
// 分配内存时按alignment对齐
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);

// 提前释放大块内存
int ngx_pfree(ngx_pool_t *pool, void *p);


ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);
void ngx_pool_run_cleanup_file(ngx_pool_t *p, int fd);
void ngx_pool_cleanup_file(void *data);
void ngx_pool_delete_file(void *data);


#endif
