/*
g++ ngx_pool_test.cpp ngx_palloc.cpp ngx_alloc_unix.cpp ngx_log.cpp -o ngx_pool_test.out -g -Wall -llogging -L../lib -I../ -std=c++11

*/

#include"ngx_palloc.h"

#include"logging.muduo/Logging.h"

#define NGX_HAVE_POSIX_MEMALIGN 1

int main()
{
    Logger::setLogLevel(Logger::DEBUG);

    ngx_pool_t *pool1=ngx_create_pool(4000, NULL);
    void *p=ngx_palloc(pool1, 40);
    LOG_INFO<<"p="<<p;

    ngx_destroy_pool(pool1);

    return 0;
}
