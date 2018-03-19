#ifndef ARENA_H_
#define ARENA_H_

#include<vector>

class Arena {
public:
	Arena();
	~Arena();

	char* Allocate(size_t bytes);

	char* AllocateAligned(size_t bytes);

	int MemoryUsage() const {  // �ܵ��ڴ�ʹ������������Ա����blocks_
		return blocks_memory_+blocks_.capacity()*sizeof(char*);  
	}

private:
	char* AllocateFallback(size_t bytes);
	char* AllocateNewBlock(size_t bytes);

	std::vector<char*> blocks_;

	char* alloc_ptr_;
	size_t alloc_bytes_remaining_;

	int blocks_memory_;

	Arena(const Arena&);
	void operator=(const Arena&);
};

#endif
