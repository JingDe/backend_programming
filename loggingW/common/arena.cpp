#include"arena.h"

#include<vector>
#include<cassert>

static const int kBlockSize = 4096;


Arena::Arena()
	: memory_usage_(0), alloc_bytes_remaining_(0), alloc_ptr_(0),blocks_(std::vector<char*>())
{

}

Arena::~Arena()
{
	for (std::vector<char*>::iterator it = blocks_.begin(); it != blocks_.end(); it++)
		delete [] *it;
}

char* Arena::Allocate(size_t bytes)
{
	assert(bytes > 0);

	if (alloc_bytes_remaining_ >= bytes)
	{
		char *res = alloc_ptr_;
		alloc_ptr_ += bytes;
		alloc_bytes_remaining_ -= bytes;
		return res;
	}
	else
		return AllocateFallback(bytes);
}

char* Arena::AllocateAligned(size_t bytes)
{	
	char* result;
	int align = sizeof(void*);
	assert( (align & (align - 1)) == 0);

	size_t current_mod = reinterpret_cast<uintptr_t>(alloc_ptr_) & (align-1); // % align
	size_t slop = (current_mod == 0 ? 0 : align - current_mod);
	size_t needed = bytes + slop;
	if (needed <= alloc_bytes_remaining_)
	{
		result = alloc_ptr_ + slop;
		alloc_ptr_ += needed;
		alloc_bytes_remaining_ -= needed;
	}
	else
		result = AllocateFallback(bytes);
	assert((reinterpret_cast<uintptr_t>(result) & (align - 1)) == 0);
	return result;
}

char* Arena::AllocateFallback(size_t bytes)
{
	if (bytes > kBlockSize / 4)
	{
		char* result = AllocateNewBlock(bytes);
		return result;
	}

	alloc_ptr_ = AllocateNewBlock(kBlockSize);
	alloc_bytes_remaining_ = kBlockSize;

	char* result = alloc_ptr_;
	alloc_ptr_ += bytes;
	alloc_bytes_remaining_ -= bytes;
	return result;
}

char* Arena::AllocateNewBlock(size_t bytes)
{
	char* result = new char[bytes];
	size_t new_usage = MemoryUsage() + bytes + sizeof(char*);
	memory_usage_.Release_Store(reinterpret_cast<void*>(new_usage)); //NoBarrier_Store??
	blocks_.push_back(result);
	return result;
}