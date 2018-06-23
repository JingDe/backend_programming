/*
./nginx -s reload
新启动的Nginx进程会向实际运行的Nginx服务进程发送SIGHUP信号
新启动的Nginx进程退出


*/


// 描述接受到信号的行为
typedef struct{
    int signo;//需要处理的信号
    char *signame;//
    char *name;//信号对应着的nginx命令
    void (*handler)(int signo, siginfo_t *siginfo, void *ucontext);
        //收到signo信号后回调handler方法
}ngx_signal_t;


// 信号处理函数的初始化
/*
#include <signal.h>
int sigaction(int signum, const struct sigaction *act,
                     struct sigaction *oldact);
struct sigaction {
    void     (*sa_handler)(int);
    void     (*sa_sigaction)(int, siginfo_t *, void *);
    sigset_t   sa_mask;
    int        sa_flags;
    void     (*sa_restorer)(void);
};

*/
int ngx_init_signals(ngx_log_t* log)
{
    ngx_signal_t *sig;
    struct sigaction sa;

    for(sig=signals; sig->signo!=0; sig++)
    {
        bzero(&sa, sizeof(sturct sigaction));

        if(sig->handler)
        {
            sa.sa_sigaction=sig->handler;
            sa.sa_flags=SA_SIGINFO;
        }
        else
            sa.sa_handler=SIG_IGN;

        sigempty(&sa.sa_mask);
        if(sigaction(sig->signo, &sa, NULL)==-1)
        {
            return -1;
        }
    }
    return 0;
}
