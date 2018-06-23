/*
    ngx_alloc调用malloc

    ngx_memalign，根据系统不同调用posix_memalign或者memalign
*/
#ifndef NGX_ALLOC_UNIX_H_
#define NGX_ALLOC_UNIX_H_

class ngx_log_t;

extern void *ngx_alloc(size_t size, ngx_log_t *log);

// #define NGX_HAVE_POSIX_MEMALIGN 1
//
// #if (NGX_HAVE_POSIX_MEMALIGN || NGX_HAVE_MEMALIGN)
extern void *ngx_memalign(size_t alignment, size_t size, ngx_log_t *log);
// #else
// #define ngx_memalign(alignment, size, log) ngx_alloc(size, log)
// #endif


#endif
