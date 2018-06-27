
/*
	自旋锁，非睡眠锁
	
	自旋锁不会睡眠，主动让出处理器，只是在内核调度到其他进程才暂停执行。
	
	适用于：
		多处理器系统
		使用自旋锁的时间非常短
		不希望进程进入睡眠
		避免睡眠锁的进程间切换开销
		
	对单处理器一样有效，内核仍能调度其他处于可执行状态的进程
	
	不适用自旋锁：
		使用锁的时间长
		epoll中，自旋锁会影响其他请求的执行，应使用非阻塞的互斥锁
		
		
	if (*lock == 0 && ngx_atomic_cmp_set(lock, 0, value))，
	ngx_atomic_cmp_set对lock的检查，保证在两条语句直线，lock没有被修改
*/




// lock表示锁的状态，value表示当锁为被占有时设置lock的值表示已被占有，spin表示自旋锁等待的时间
void
ngx_spinlock(ngx_atomic_t *lock, ngx_atomic_int_t value, ngx_uint_t spin)
{

#if (NGX_HAVE_ATOMIC_OPS)

    ngx_uint_t  i, n;

    for ( ;; ) {

        if (*lock == 0 && ngx_atomic_cmp_set(lock, 0, value)) {
            return;
        }

        if (ngx_ncpu > 1) {

            for (n = 1; n < spin; n <<= 1) {

                for (i = 0; i < n; i++) {
					/* pause指令，告诉CPU当前处于自旋锁等待状态。
					*  当前进程没有让出处理器。
					*/
                    ngx_cpu_pause();
                }

                if (*lock == 0 && ngx_atomic_cmp_set(lock, 0, value)) {
                    return;
                }
            }
        }

		/* 进程仍然处于可执行状态，但暂时让出处理器。使得处理器优先调度其他可执行状态的进程。
		*/
        ngx_sched_yield();
    }

#else

	#if (NGX_THREADS)

	#error ngx_spinlock() or ngx_atomic_cmp_set() are not defined !

	#endif

#endif

}