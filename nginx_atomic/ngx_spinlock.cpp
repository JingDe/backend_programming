
/* lock是原子变量实现的锁。当lock是0时表示锁被释放，否则被占用。
value表示设置锁的值，以占用锁。
spin表示在多处理器系统中，线程没有获得锁，等待其他处理器释放锁的时间。*/
void ngx_spinlock(ngx_atomic_t *lock, ngx_atomic_int_t value, ngx_uint_t spin)
{
    ngx_uint_t  i, n;

    for ( ;; ) { 

		// 
        if (*lock == 0 && ngx_atomic_cmp_set(lock, 0, value)) {
            return;
        }   

        if (ngx_ncpu > 1) {

			/*线程不让出当前处理器，等待一段时间，等待其他处理器上的线程释放锁，减少线程切换。*/
            for (n = 1; n < spin; n <<= 1) {
				/*随着等待的次数越来越多，检查lock是否释放的越来越不频繁。*/
                for (i = 0; i < n; i++) {
					/*CPU指令。进程没有让出处理器*/
                    ngx_cpu_pause();
                }   
				/*检查锁是否被释放*/
                if (*lock == 0 && ngx_atomic_cmp_set(lock, 0, value)) {
                    return;
                }   
            }   
        }   
		/*进程保持可执行状态，暂时让出处理器。调度其他可执行状态的进程。*/
        ngx_sched_yield();
    }
}