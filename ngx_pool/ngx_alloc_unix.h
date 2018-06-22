/*
    ngx_alloc调用malloc

    ngx_memalign，根据系统不同调用posix_memalign或者memalign
*/



void *ngx_alloc(size_t size, ngx_log_t *log);


#if (NGX_HAVE_POSIX_MEMALIGN || NGX_HAVE_MEMALIGN)

void *ngx_memalign(size_t alignment, size_t size, ngx_log_t *log);

#else

#define ngx_memalign(alignment, size, log) ngx_alloc(size, log)

#endif
