/*
g++ ngx_pool_test.cpp ngx_palloc.cpp ngx_alloc_unix.cpp ngx_log.cpp -o ngx_pool_test.out -g -Wall -llogging -L../lib -I../ -std=c++11

*/

#include"ngx_palloc.h"

#include"logging.muduo/Logging.h"


#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#define NGX_HAVE_POSIX_MEMALIGN 1

void test1()
{
    Logger::setLogLevel(Logger::DEBUG);

    ngx_pool_t *pool1=ngx_create_pool(4000, NULL);
    void *p=ngx_palloc(pool1, 40);
    LOG_INFO<<"p="<<p;

    ngx_destroy_pool(pool1);
}

void test2()
{
    Logger::setLogLevel(Logger::DEBUG);

    do {
        ngx_pool_t *pool2=ngx_create_pool(4000, NULL);
        if(pool2==NULL)
            break;

        int fd=open("FILE.txt", O_CREAT, 0644);
        if(fd<0)
            break;

        ngx_pool_cleanup_t *cl=ngx_pool_cleanup_add(pool2, sizeof(ngx_pool_cleanup_file_t));
        if(cl==NULL)
            break;

        cl->handler=ngx_pool_delete_file;
        ngx_pool_cleanup_file_t *cft=(ngx_pool_cleanup_file_t *)cl->data;
        cft->fd=fd;

        //cft->name=new char[9]("FILE.txt");//不能括号初始化
        cft->name=new char[15];//字符数
        char name[]="FILE.txt";
        //<1>
        memcpy((void*)cft->name, (void*)name, sizeof(name));//拷贝的字节数
        //<2>
        //snprintf(cft->name, sizeof(name), "%s", name);//写arg2-1个字符

        // sizeof()返回值：字节数，包括0
        // strlen()返回值：0结尾的字符串的字符数，不包括0
        LOG_DEBUG<<"strlen = "<<strlen(cft->name);
        LOG_DEBUG<<"sizeof = "<<sizeof(cft->name);//得到的是指针的字节数8
        LOG_DEBUG<<"sizeof = "<<sizeof(char);
        LOG_DEBUG<<"sizeof = "<<sizeof(cft->name)/sizeof(char);

        LOG_DEBUG<<"strlen = "<<strlen(name);
        LOG_DEBUG<<"sizeof = "<<sizeof(name);

        cft->log=pool2->log;

        ngx_pool_run_cleanup_file(pool2, fd);

        ngx_destroy_pool(pool2);
    } while(0);

}

int main()
{
    test2();

    return 0;
}
