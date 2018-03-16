#ifndef CACHE_LEVELDB_H_
#define CACHE_LEVELDB_H_

#include"common/Slice.h"

class Cache {
public:
	Cache() {}

	virtual ~Cache();

	// 指向缓存中的一个条目
	struct Handle {}; // LRUHandle

	virtual Handle* Insert(const Slice& key, void* value, size_t charge, void(*deleter)(const Slice& key, void* value)) = 0;

	virtual Handle* Lookup(const Slice& key) = 0;

	virtual void Release(Handle* handle) = 0;

	virtual void* Value(Handle* handle) = 0;

	virtual void Erase(const Slice& key) = 0;

	virtual void Prune() {} // 删除非active的条目

	virtual size_t TotalCharge() const = 0;

	virtual uint64_t NewId() = 0; // 

private:
	void LRU_Remove(Handle* e);
	void LRU_Append(Handle* e);
	void Unref(Handle* e);

	struct Rep;
	Rep* rep_;

	Cache(const Cache&);
	void operator=(const Cache&);
};

#endif