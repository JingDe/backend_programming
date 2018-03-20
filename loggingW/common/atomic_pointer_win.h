#ifndef ATOMIC_POINTER_H_
#define ATOMIC_POINTER_H_

// AtomicPointer�ṩ��һ������ָ��Ĵ洢
// 1�����ƽ̨�ṩmemory barrier��ʹ������ԭʼָ�룻
// 2���������atomic��ʹ��atomic��
		// ����ʹ��memory barrier�汾�����ܸ��á�

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



// ����MemeryBarrier()
#if defined(OS_WIN) && defined(COMPILER_MSVC) && defined(ARCH_CPU_X86_FAMILY)// Windows on x86
	// windows.h�ṩ��MemroyBarrier()����
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


// ʵ��AtomicPointer:
// ����ʹ��Memory barrier
#if defined(LEVELDB_HAVE_MEMORY_BARRIER)
class AtomicPoointer {
private:
	void* rep_;

public:
	AtomicPointer() {}
	explicit AtomicPointer(void* p) : rep_(p) {}
	// ��Ϊinline�������������ñ�����barrier�ģ�������������reorder
	inline void* NoBarrier_Load() const { return rep_;  }
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
#else
//#error Please implement AtomicPointer for this platform. // gcc
#endif


#endif 
