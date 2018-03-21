#ifndef ATOMIC_POINTER_H_
#define ATOMIC_POINTER_H_

// AtomicPointer提供对一个无锁指针的存储
// 1，如果平台提供memory barrier，使用它和原始指针；
// 2，如果存在<atomic>，使用<atomic>；
		// 优先使用memory barrier版本：性能更好。

#include <stdint.h>

#ifdef LEVELDB_ATOMIC_PRESENT
	#include <atomic>
#endif
#ifdef OS_WIN
	#include <windows.h> // windows两种实现，另一种port_win.h
#endif
#ifdef __APPLE__
	#include <libkern/OSAtomic.h>
#endif

// 不同CPU
#if defined(_M_X64) || defined(__x86_64__) // _M_X64编译器定义，面向 x64 处理器定义为整数值100。 否则，未定义。
#define ARCH_CPU_X86_FAMILY 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
#define ARCH_CPU_X86_FAMILY 1
#elif defined(__ARMEL__)
#define ARCH_CPU_ARM_FAMILY 1
#elif defined(__aarch64__)
#define ARCH_CPU_ARM64_FAMILY 1
#elif defined(__ppc__) || defined(__powerpc__) || defined(__powerpc64__)
#define ARCH_CPU_PPC_FAMILY 1
#elif defined(__mips__)
#define ARCH_CPU_MIPS_FAMILY 1
#endif

namespace port {

	// 定义MemeryBarrier()
#if defined(OS_WIN) && defined(COMPILER_MSVC) && defined(ARCH_CPU_X86_FAMILY)// Windows on x86
	// windows.h提供了MemroyBarrier宏，阻止CPU reorder读写，也可能阻止编译器reorder
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

#elif defined(__APPLE__)// Mac OS
	inline void MemoryBarrier() {
		OSMemoryBarrier();
	}
#define LEVELDB_HAVE_MEMORY_BARRIER

#elif defined(ARCH_CPU_ARM_FAMILY) && defined(__linux__)// ARM Linux
	typedef void(*LinuxKernelMemoryBarrierFunc)(void);
	// linux ARM内核提供一个高度优化的机器特定的memory barrier函数，
	// 该函数在一个固定的内存位置，并且映射到每一个用户进程中

	// 在单核机器上使用CPU特定的指令是不必要和昂贵的。
	// benchmark表明在多核机器上额外的函数调用是完全可忽略的
	inline void MemoryBarrier() {
		(*(LinuxKernelMemoryBarrierFunc)0xffff0fa0)();
	}
#define LEVELDB_HAVE_MEMORY_BARRIER

#elif defined(ARCH_CPU_ARM64_FAMILY)// ARM64
	inline void MemoryBarrier() {
		asm volatile("dmb sy" : : : "memory");
	}
#define LEVELDB_HAVE_MEMORY_BARRIER

#elif defined(ARCH_CPU_PPC_FAMILY) && defined(__GNUC__)// PPC
	inline void MemoryBarrier() {
		asm volatile("sync" : : : "memory");
	}
#define LEVELDB_HAVE_MEMORY_BARRIER

#elif defined(ARCH_CPU_MIPS_FAMILY) && defined(__GNUC__)// MIPS
	inline void MemoryBarrier() {
		__asm__ __volatile__("sync" : : : "memory");
	}
#define LEVELDB_HAVE_MEMORY_BARRIER

#endif


	// 实现AtomicPointer:
	// 优先使用特定平台的Memory barrier
#if defined(LEVELDB_HAVE_MEMORY_BARRIER)
	class AtomicPointer {
	private:
		void* rep_;

	public:
		AtomicPointer() {}
		explicit AtomicPointer(void* p) : rep_(p) {}
		// 均为inline函数，函数调用本身是barrier的，编译器不进行reorder
		inline void* NoBarrier_Load() const { return rep_; }
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

	// sparc处理器的memory barrier版本
#elif defined(__sparcv9) && defined(__GNUC__)
	class AtomicPointer {
	private:
		void* rep_;
	public:
		AtomicPointer() { }
		explicit AtomicPointer(void* v) : rep_(v) { }
		inline void* Acquire_Load() const {
			void* val;
			__asm__ __volatile__(
				"ldx [%[rep_]], %[val] \n\t"
				"membar #LoadLoad|#LoadStore \n\t"
				: [val] "=r" (val)
				: [rep_] "r" (&rep_)
				: "memory");
			return val;
		}
		inline void Release_Store(void* v) {
			__asm__ __volatile__(
				"membar #LoadStore|#StoreStore \n\t"
				"stx %[v], [%[rep_]] \n\t"
				:
			: [rep_] "r" (&rep_), [v] "r" (v)
				: "memory");
		}
		inline void* NoBarrier_Load() const { return rep_; }
		inline void NoBarrier_Store(void* v) { rep_ = v; }
	};

	// ia64指令集架构acq/rel版本
#elif defined(__ia64) && defined(__GNUC__)
	class AtomicPointer {
	private:
		void* rep_;
	public:
		AtomicPointer() { }
		explicit AtomicPointer(void* v) : rep_(v) { }
		inline void* Acquire_Load() const {
			void* val;
			__asm__ __volatile__(
				"ld8.acq %[val] = [%[rep_]] \n\t"
				: [val] "=r" (val)
				: [rep_] "r" (&rep_)
				: "memory"
			);
			return val;
		}
		inline void Release_Store(void* v) {
			__asm__ __volatile__(
				"st8.rel [%[rep_]] = %[v]  \n\t"
				:
			: [rep_] "r" (&rep_), [v] "r" (v)
				: "memory"
				);
		}
		inline void* NoBarrier_Load() const { return rep_; }
		inline void NoBarrier_Store(void* v) { rep_ = v; }
	};
#else
//#error Please implement AtomicPointer for this platform. // gcc
#endif

#undef LEVELDB_HAVE_MEMORY_BARRIER
#undef ARCH_CPU_X86_FAMILY
#undef ARCH_CPU_ARM_FAMILY
#undef ARCH_CPU_ARM64_FAMILY
#undef ARCH_CPU_PPC_FAMILY

}

#endif 
