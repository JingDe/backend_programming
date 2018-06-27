


/* 用原子变量+信号量实现互斥锁
*	lock变量表示当前锁的状态
*		//lock为0或正数，表示没有进程持有锁
*		lock为0表示未被持有锁，否则为进程ID
*	sem或spin决定ngx_shmtx_lock阻塞锁的行为
*		spin为-1时，不使用信号量
*/
struct ngx_shmtx_t{
	ngx_atomic_t *lock;// 原子变量锁
	
	ngx_atomic_t *wait;
	unsigned int semaphore;// 1表示获取锁将可能使用到信号量
	sem_t sem;
	
	unsigned int spin;// 自旋次数
};


/*
*	等待互斥锁：自旋或者等待信号量
*			使用自旋，周期性的等待并检查，以及让出处理器
*			使用信号量，等待sem_wait，进程睡眠
*	Nginx优先使用自旋锁，好处是：不让进程主动睡眠，不降低进程并发性能，避免睡眠锁的进程间切换开销
*/
void ngx_shmtx_lock(ngx_shmtx_t *mtx)
{
	unsigned int i, n;
	
	LOG_DEBUG<<"shmtx lock";
	
	while(true)
	{
		/*
		lock一般在共享内存中，可能会时刻变化。不使用本地临时变量暂存。
		lock等于0表示锁未被持有。ngx_atomic_cmp_set再次检查lock是否为0
		ngx_pid=getpid();
		*/
		if(*mtx->lock==0 &&  ngx_atomic_cmp_set(mtx->lock, 0, ngx_pid))
			return;
		
		// 在多处理器状态下spin值才有意义
		// unsigned int ngx_ncpu=sysconf(_SC_NPROCESSORS_ONLN); linux系统获得配置信息：可用的CPU个数
		if(ngx_ncpu >1)
		{
			// 循环执行PAUSE，检查锁是否已经释放
			for(n=1; n<mtx->spin; n<<=1)
			{
				// 随着长时间没有获得锁，将会执行更多次PAUSE才会检查锁
				for(i=0; i<n; i++)
					ngx_cpu_pause();// __asm__ (".byte 0xf3, 0x90")或__asm__ ("pause")等等等
				
				if(*mtx->lock==0  &&  ngx_atomic_cmp_set(mtx->lock, 0, ngx_pid))
					return;
			}
		}

# if (NGX_HAVE_POSIX_SEM)		
		if(mtx->semphore)
		{
			// wait表示调用sem_wait的次数
			(void)ngx_atomic_fetch_add(mtx->wait, 1);
			
			if(*mtx->lock==0  &&  ngx_atomic_cmp_set(mtx->lock, 0, ngx_pid))
			{
				(void)ngx_atomic_fetch_add(mtx->wait, -1);
				return;
			}
			
			/*
			wait成功返回0，信号量值减1。失败返回-1，信号量不变。
			*/
			while(sem_wait(&mtx->sem)==-1)
			{
				int err;
				err=errno;
				
				if(err!=EINTR)
				{
					LOG_ERROR<<"sem_wait() failed while waiting on shmtx";
					break;
				}
			}
			
			LOG_DEBUG<<"shmtx awoke";
			continue;
		}
#endif
		sched_yield();//使用自旋锁时，<shed.h>让出处理器。或者usleep(1)睡眠1毫秒。（均对线程）
	}
}