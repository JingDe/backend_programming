#ifndef ATOMIC_POINTER_H_
#define ATOMIC_POINTER_H_

// AtomicPointer提供对一个无锁指针的存储
// 1，如果平台提供memory barrier，使用它和原始指针；
// 2，如果存在atomic，使用atomic；
		// 优先使用memory barrier版本：性能更好。

#include <stdint.h>

#ifdef LEVELDB_ATOMIC_PRESENT
	#include <atomic>
#endif
#ifdef OS_WIN
	#include <windows.h>
#endif
#ifdef OS_MACOSX
	#include <libkern/OSAtomic.h>
#endif



// 定义MemeryBarrier()
#if defined(OS_WIN) && defined(COMPILER_MSVC) && defined(ARCH_CPU_X86_FAMILY)// Windows on x86
	// windows.h提供了MemroyBarrier()函数
	#define LEVELDB_HAVE_MEMORY_BARRIER 

#elif defined(ARCH_CPU_X86_FAMILY) && defined(__GNUC__)// Gcc on x86
	inline void MemoryBarrier() {
		__asm__ __volatile__("" : : : "memory");
	}
	#define LEVELDB_HAVE_MEMORY_BARRIER
	
#elif defined(ARCH_CPU_X86_FAMILY) && defined(__SUNPRO_CC)// Sun Studio
	inline void MemoryBarrier() {
		asm volatile("" : : : "memory");
	}
	#define LEVELDB_HAVE_MEMORY_BARRIER
	
#elif defined(OS_MACOSX)// Mac OS
	inline void MemoryBarrier() {
		OSMemoryBarrier();
	}
	#define LEVELDB_HAVE_MEMORY_BARRIER
	
#elif defined(ARCH_CPU_ARM_FAMILY)// ARM
	typedef void(*LinuxKernelMemoryBarrierFunc)(void);
	LinuxKernelMemoryBarrierFunc pLinuxKernelMemoryBarrier __attribute__((weak)) =
	(LinuxKernelMemoryBarrierFunc)0xffff0fa0;
	inline void MemoryBarrier() {
		pLinuxKernelMemoryBarrier();
	}
	#define LEVELDB_HAVE_MEMORY_BARRIER

#endif


// 实现AtomicPointer:
// 优先使用Memory barrier
#if defined(LEVELDB_HAVE_MEMORY_BARRIER)
class AtomicPoointer {
private:
	void* rep_;

public:
	AtomicPointer() {}
	explicit AtomicPointer(void* p) : rep_(p) {}
	// 均为inline函数，函数调用本身是barrier的，编译器不进行reorder
	inline void* NoBarrier_Load() const { return rep_;  }
	inline void NoBarrier_Store(void* v) { rep_ = v; }
	inline void* Acquire_Load() const {
		void* result = rep_;
		MemoryBarrier();// 保证取出rep_后，才进行其他读写
		return result;
	}
	inline void Release_Store(void *v) {
		MemoryBarrier();
		rep_ = v;
	}
};
// 其次使用<atomic>的版本
#elif defined(LEVELDB_ATOMIC_PRESENT)
	class AtomicPointer {
	private:
		std::atomic<void*> rep_;

	public:
		AtomicPointer() {}
		explicit AtomicPointer(void* v) :rep_(v) {}
		inline void* Acquire_Load() const {
			// load()原子地获取和返回rep_的当前值
			// memory_order_acquire表示当前线程之前的任何读写不能reorder，其他线程对rep_的写在当前线程可见
			return rep_.load(std::memory_order_acquire); 
		}
		inline void Release_Store(void* v) {
			// store()原子地替换rep_的当前值
			// memory_order_release表示store之后当前线程的任何读写不能reorder。
			// 当前线程的所有写对其他acquire rep_的线程可见，并且 carry a dependency into rep_的写对其他consume rep_的线程可见
			rep_.store(v, std::memory_order_release); 
		}
		inline void* NoBarrier_Load() const {
			// memory_order_relaxed只保证此操作的原子性，没有其他读写上的同步或者顺序限制
			return rep_.load(std::memory_order_relaxed); 
		}
		inline void NoBarrier_Store() {
			rep_.store(v, std::memory_order_relaxed);
		}
	};
#else
//#error Please implement AtomicPointer for this platform. // gcc
#endif


#endif 
