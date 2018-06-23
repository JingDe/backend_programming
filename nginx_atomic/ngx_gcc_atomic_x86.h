

/*
基于x86的SMP多核架构实现原子操作

__asm__ volatile ( 
	汇编语句部分   
	: 输出部分  
	: 输入部分
	: 破坏描述部分 //通知编译器使用了哪些寄存器、内存。
	);

*/

#if (NGX_SMP)
#define NGX_SMP_LOCK  "lock;"
#else
#define NGX_SMP_LOCK
#endif

// ngx_atomic_t
// ngx_atomic_uint_t


/* cmpxchgl r,[m]语句首先会用m比较eax寄存器中的值，
如果相等，则把m的值设为r，同时将zf标志位设为1；
否则将zf标志位设为0。 
sete[m]，将zf标志位中的0或者1设置到m中。
*/


static inline ngx_atomic_uint_t
ngx_atomic_cmp_set(ngx_atomic_t *lock, ngx_atomic_uint_t old, ngx_atomic_uint_t set)
{
	unsigned char res;
	
	__asm__ volatile(
	//如果是SMP多核架构，锁住总线
	NGX_SMP_LOCK
	//将*lock的值与eax寄存器中的old相比较，如果相等，置*lock的值为set
	"    cmpxchgl  %3, %1;   "// cmpxchgl set *lock
	//cmpxchgl的比较若是相等，则把zf标志位1写入res变量，否则res为0
    "    sete      %0;       "

    : "=a" (res) // %0
	: "m" (*lock), "a" (old), "r" (set) // %1 %2 %3
		// lock变量在内存中，old变量写入eax寄存器，set变量写入通用寄存器中
	: "cc", "memory"
	);

    return res;
}


/*
xaddlr,[m]，执行后[m]值将为r和[m]之和，而r中的值为原[m]值。

*/

//*value原子变量的值加上add，同时返回原先*value的值。
static ngx_inline ngx_atomic_int_t
ngx_atomic_fetch_add(ngx_atomic_t *value, ngx_atomic_int_t add)
{
    __asm__ volatile (

         NGX_SMP_LOCK
    "    xaddl  %0, %1;   "// xaddl add *value

    : "+r" (add) : "m" (*value) : "cc", "memory");

    return add;
}
