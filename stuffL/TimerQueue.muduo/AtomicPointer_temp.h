#ifndef _ATOMICPOINTER_TEMP_H_
#define _ATOMICPOINTER_TEMP_H_

inline void MemoryBarrier() {
	__asm__ __volatile__("" : : : "memory");
}


class AtomicPointer {
private:
	void* rep_;

public:
	AtomicPointer() {}
	explicit AtomicPointer(void* p) : rep_(p) {}
	
	inline void* NoBarrier_Load() const { return rep_; }
	inline void NoBarrier_Store(void* v) { rep_ = v; }
	inline void* Acquire_Load() const {
		void* result = rep_;
		MemoryBarrier();
		return result;
	}
	inline void Release_Store(void *v) {
		MemoryBarrier();
		rep_ = v;
	}

	void* incrementAndGet() // ¼Ó1£¬·µ»Ørep_
	{
		MemoryBarrier();
		void* result = reinterpret_cast<void*>(reinterpret_cast<int>(rep_)+1);
		//Release_Store(result);
		MemoryBarrier();
		rep_ = result;

		//Acquire_Load();
		// MemoryBarrier();// ??
		return result;
	}
	
};


#endif