/*

    linux系统函数：
    <1>
    #include <stdlib.h>
    int posix_memalign(void **memptr, size_t alignment, size_t size);
    分配size字节，首地址放在*memptr，是alignment的倍数。
    成功返回0
    alignment必须是2的指数，并且sizeof(void*)的倍数
    freebsd

    <2>
    #include <malloc.h>
    void *memalign(size_t alignment, size_t size);
    返回分配内存地址
    alignment必须是2的指数
    solaris


    C库函数：
    malloc
    分配的内存是MALLOC_ALIGNMENT对齐的。
    MALLOC_ALIGNMENT是2的指数、sizeof(void*)的整数倍。




    sizeof(void*)由编译器产生的目标平台的指令集决定

    目标平台的 软件约定 ， ABI （应用程序二进制接口，
        调用约定,类型表示和名称修饰这三者的统称）属于软件约定 的一个子集。

*/

#include<stdlib.h>
#include<malloc.h>

#include"ngx_log.h"
#include"logging.muduo/Logging.h"

void* ngx_alloc(size_t size, ngx_log_t *log)
{
    void* p;

    p=malloc(size);
    if(p==NULL)
    {
        LOG_FATAL<<"malloc failed "<<strerror(errno);
    }
    LOG_DEBUG<<"malloc "<<p<<" "<<size;
    return p;
}


// #if (NGX_HAVE_POSIX_MEMALIGN)

void *ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{
    void *p;
    int err;

    err=posix_memalign(&p, alignment, size);

    if(err)
    {
        //ngx_log_error(NGX_LOG_EMERG, log, err, "posix_memalign(%uz, %uz) failed", alignment, size);
        p=NULL;
    }
    //ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0, "posix_memalign: %p:%uz @%uz", p, size, alignment);
    return p;
}

// #elif (NGX_HAVE_MEMALIGN)
//
// void *ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
// {
//     void *p;
//
//     p=memalign(alignment, size);
//     if(p==NULL)
//     {
//         LOG_FATAL<<"memalign failed "<<stderror(errno);
//     }
//     LOG_DEBUG<<"memalign: "<<p<<" "<<size;
//     return p;
// }
//
// #endif
