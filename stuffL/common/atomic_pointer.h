#ifndef ATOMIC_POINTER_H_
#define ATOMIC_POINTER_H_

// AtomicPointer�ṩ��һ������ָ��Ĵ洢
// 1�����ƽ̨�ṩmemory barrier��ʹ������ԭʼָ�룻
// 2���������<atomic>��ʹ��<atomic>��
		// ����ʹ��memory barrier�汾�����ܸ��á�

#include <stdint.h>

#ifdef LEVELDB_ATOMIC_PRESENT
	#include <atomic>
#endif
#ifdef _MSC_VER
	#include <windows.h> // windows����ʵ�֣���һ��port_win.h
#endif
#ifdef __APPLE__
	#include <libkern/OSAtomic.h>
#endif

// ��ͬCPU
#if defined(_M_X64) || defined(__x86_64__) // _M_X64���������壬���� x64 ����������Ϊ����ֵ100�� ����δ���塣
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

	// ����MemeryBarrier()
#if defined(_MSC_VER) && defined(COMPILER_MSVC) && defined(ARCH_CPU_X86_FAMILY)// Windows on x86
	// windows.h�ṩ��MemroyBarrier�꣬��ֹCPU reorder��д��Ҳ������ֹ������reorder
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
	// linux ARM�ں��ṩһ���߶��Ż��Ļ����ض���memory barrier������
	// �ú�����һ���̶����ڴ�λ�ã�����ӳ�䵽ÿһ���û�������

	// �ڵ��˻�����ʹ��CPU�ض���ָ���ǲ���Ҫ�Ͱ���ġ�
	// benchmark�����ڶ�˻����϶���ĺ�����������ȫ�ɺ��Ե�
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


	// ʵ��AtomicPointer:
	// ����ʹ���ض�ƽ̨��Memory barrier
#if defined(LEVELDB_HAVE_MEMORY_BARRIER)
	class AtomicPointer {
	private:
		void* rep_;

	public:
		AtomicPointer() {}
		explicit AtomicPointer(void* p) : rep_(p) {}
		// ��Ϊinline�������������ñ�����barrier�ģ�������������reorder
		inline void* NoBarrier_Load() const { return rep_; }
		inline void NoBarrier_Store(void* v) { rep_ = v; }
		inline void* Acquire_Load() const {
			void* result = rep_;
			MemoryBarrier();// ��֤ȡ��rep_�󣬲Ž���������д
			return result;
		}
		inline void Release_Store(void *v) {
			MemoryBarrier();
			rep_ = v;
		}
	};
	// ���ʹ��<atomic>�İ汾
#elif defined(LEVELDB_ATOMIC_PRESENT)
	class AtomicPointer {
	private:
		std::atomic<void*> rep_;

	public:
		AtomicPointer() {}
		explicit AtomicPointer(void* v) :rep_(v) {}
		inline void* Acquire_Load() const {
			// load()ԭ�ӵػ�ȡ�ͷ���rep_�ĵ�ǰֵ
			// memory_order_acquire��ʾ��ǰ�߳�֮ǰ���κζ�д����reorder�������̶߳�rep_��д�ڵ�ǰ�߳̿ɼ�
			return rep_.load(std::memory_order_acquire);
		}
		inline void Release_Store(void* v) {
			// store()ԭ�ӵ��滻rep_�ĵ�ǰֵ
			// memory_order_release��ʾstore֮��ǰ�̵߳��κζ�д����reorder��
			// ��ǰ�̵߳�����д������acquire rep_���߳̿ɼ������� carry a dependency into rep_��д������consume rep_���߳̿ɼ�
			rep_.store(v, std::memory_order_release);
		}
		inline void* NoBarrier_Load() const {
			// memory_order_relaxedֻ��֤�˲�����ԭ���ԣ�û��������д�ϵ�ͬ������˳������
			return rep_.load(std::memory_order_relaxed);
		}
		inline void NoBarrier_Store() {
			rep_.store(v, std::memory_order_relaxed);
		}
	};

	// sparc��������memory barrier�汾
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

	// ia64ָ��ܹ�acq/rel�汾
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
