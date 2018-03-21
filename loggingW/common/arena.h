#ifndef ARENA_H_
#define ARENA_H_

#include"common/atomic_pointer.h"

//#define ATOMICPOINTER_WINAPI//??
//#include"port/port_win.h"
//#undef ATOMICPOINTER_WINAPI

#include<vector>

class Arena {
public:
	Arena();
	~Arena();

	char* Allocate(size_t bytes);

	char* AllocateAligned(size_t bytes);

	size_t MemoryUsage() const {  // 总的内存使用量，包括成员变量blocks_
		return reinterpret_cast<uintptr_t>(memory_usage_.NoBarrier_Load());
	}

private:
	char* AllocateFallback(size_t bytes);
	char* AllocateNewBlock(size_t bytes);

	std::vector<char*> blocks_;

	char* alloc_ptr_;
	size_t alloc_bytes_remaining_;

	//int blocks_memory_;
	port::AtomicPointer memory_usage_;

	Arena(const Arena&);
	void operator=(const Arena&);
};

#endif
