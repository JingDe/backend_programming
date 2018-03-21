#include"cache.h"
#include"common/coding.h"
#include"common/testharness.h"

#include<iostream>
#include<cassert>
#include<vector>

static std::string EncodeKey(int k) {
	std::string result;
	PutFixed32(&result, k);
	return result;
}
static int DecodeKey(const Slice& k) {
	assert(k.size() == 4);
	return DecodeFixed32(k.data());
}

static void* EncodeValue(uintptr_t v) { return reinterpret_cast<void*>(v); }
static int DecodeValue(void* v) { return reinterpret_cast<uintptr_t>(v); }

class CacheTest {
public:
	static CacheTest* current_;

	static void Deleter(const Slice& key, void* v)
	{
		current_->deleted_keys_.push_back(DecodeKey(key));
		current_->deleted_values_.push_back(DecodeValue(v));
	}

	static const int kCacheSize = 1000;
	std::vector<int> deleted_keys_;
	std::vector<int> deleted_values_;
	Cache* cache_;

	CacheTest() :cache_(NewLRUCache(kCacheSize))
	{
		current_ = this;
	}

	~CacheTest() {
		delete cache_;
	}

	int Lookup(int key)
	{
		Cache::Handle* handle = cache_->Lookup(EncodeKey(key));
		const int r = (handle == NULL) ? -1 : DecodeValue(cache_->Value(handle));
		if (handle != NULL)
			cache_->Release(handle);
		return r;
	}

	void Insert(int key, int value, int charge = 1)
	{
		cache_->Release(cache_->Insert(EncodeKey(key), EncodeValue(value), charge, &CacheTest::Deleter));
	}

	Cache::Handle* InsertAndReturnHandle(int key, int value, int charge = 1) {
		return cache_->Insert(EncodeKey(key), EncodeValue(value), charge,
			&CacheTest::Deleter);
	}

	void Erase(int key) {
		cache_->Erase(EncodeKey(key));
	}
};

CacheTest* CacheTest::current_;

void test_HitAndMiss()
{
	CacheTest::current_ = new CacheTest;

	assert(-1 == CacheTest::current_->Lookup(100)); // current_为NULL

	CacheTest::current_->Insert(100, 101);
	assert(101== CacheTest::current_->Lookup(100));
	assert(-1== CacheTest::current_->Lookup(200));
	assert(-1== CacheTest::current_->Lookup(300));

	CacheTest::current_->Insert(200, 201);
	assert(101== CacheTest::current_->Lookup(100));
	assert(201== CacheTest::current_->Lookup(200));
	assert(-1== CacheTest::current_->Lookup(300));

	CacheTest::current_->Insert(100,102);
	assert(102== CacheTest::current_->Lookup(100));
	assert(201== CacheTest::current_->Lookup(200));
	assert(-1== CacheTest::current_->Lookup(300));

	assert(1== CacheTest::current_->deleted_keys_.size());
	assert(100== CacheTest::current_->deleted_keys_[0]);
	assert(101== CacheTest::current_->deleted_values_[0]);

	delete CacheTest::current_;

	printf("passed HitAndMiss\n");
}

void test_Erase()
{
	CacheTest::current_ = new CacheTest;

	CacheTest::current_->Erase(200);
	assert(0 == CacheTest::current_->deleted_keys_.size());

	CacheTest::current_->Insert(100, 101);
	CacheTest::current_->Insert(200, 201);
	CacheTest::current_->Erase(100);
	assert(-1 == CacheTest::current_->Lookup(100));
	assert(201== CacheTest::current_->Lookup(200));

	assert(1 == CacheTest::current_->deleted_keys_.size());
	assert(100 == CacheTest::current_->deleted_keys_[0]);
	assert(101 == CacheTest::current_->deleted_values_[0]);

	CacheTest::current_->Erase(100);
	assert(-1 == CacheTest::current_->Lookup(100));
	assert(201 == CacheTest::current_->Lookup(200));
	assert(1 == CacheTest::current_->deleted_keys_.size());

	delete CacheTest::current_;

	printf("passed Erase\n");
}


TEST(CacheTest, EntriesArePinned) {
	Insert(100, 101); // 先Insert，(100, 101)引用计数为2，插入in_use_; 后release，(100, 101)引用计数为1，移到lru_
	Cache::Handle* h1 = cache_->Lookup(EncodeKey(100)); // (100, 101)引用计数为2，移到in_use_
	ASSERT_EQ(101, DecodeValue(cache_->Value(h1)));

	Insert(100, 102); // (100, 101) in_cache_为false，引用计数为1
					// (100, 102) 先Insert，(100, 101)引用计数为2，插入in_use_
					// (100, 102) 后release，(100, 101)引用计数为1，移到lru_
	Cache::Handle* h2 = cache_->Lookup(EncodeKey(100)); // (100, 102)引用计数为2，移到in_use_
	ASSERT_EQ(102, DecodeValue(cache_->Value(h2)));
	ASSERT_EQ(0, deleted_keys_.size());

	cache_->Release(h1); // (100, 101)引用计数减1，立即delete
	ASSERT_EQ(1, deleted_keys_.size());
	ASSERT_EQ(100, deleted_keys_[0]);
	ASSERT_EQ(101, deleted_values_[0]);

	Erase(100); // (100, 102)引用计数为1，移到lru_
	ASSERT_EQ(-1, Lookup(100));
	ASSERT_EQ(1, deleted_keys_.size());

	cache_->Release(h2); // (100, 102)引用计数减1，立即delete
	ASSERT_EQ(2, deleted_keys_.size());
	ASSERT_EQ(100, deleted_keys_[1]);
	ASSERT_EQ(102, deleted_values_[1]);
}

TEST(CacheTest, EvictionPolicy) {
	Insert(100, 101); // 引用计数为1，移到lru_
	Insert(200, 201);
	Insert(300, 301);
	Cache::Handle* h = cache_->Lookup(EncodeKey(300)); // 300移到in_use_

	for (int i = 0; i < kCacheSize + 100; i++) {
		Insert(1000 + i, 2000 + i); // 引用计数为1，移到lru_
		ASSERT_EQ(2000 + i, Lookup(1000 + i)); // 先移到in_use_开头最新位置，再移到lru_开头最新位置
		ASSERT_EQ(101, Lookup(100));
	}
	ASSERT_EQ(101, Lookup(100));
	ASSERT_EQ(-1, Lookup(200));
	ASSERT_EQ(301, Lookup(300));
	cache_->Release(h);
}

// cache只删除lru_中的老数据
TEST(CacheTest, UseExceedsCacheSize) {
	// Overfill the cache, keeping handles on all inserted entries.
	std::vector<Cache::Handle*> h;
	for (int i = 0; i < kCacheSize + 100; i++) {
		h.push_back(InsertAndReturnHandle(1000 + i, 2000 + i));
	}

	// Check that all the entries can be found in the cache.
	for (int i = 0; i < h.size(); i++) {
		ASSERT_EQ(2000 + i, Lookup(1000 + i));
	}

	for (int i = 0; i < h.size(); i++) {
		cache_->Release(h[i]);
	}
}


TEST(CacheTest, HeavyEntries) {
	// Add a bunch of light and heavy entries and then count the combined
	// size of items still in the cache, which must be approximately the
	// same as the total capacity.
	const int kLight = 1;
	const int kHeavy = 10;
	int added = 0;
	int index = 0;
	while (added < 2 * kCacheSize) {
		const int weight = (index & 1) ? kLight : kHeavy;
		Insert(index, 1000 + index, weight);
		added += weight;
		index++;
	}

	int cached_weight = 0;
	for (int i = 0; i < index; i++) {
		const int weight = (i & 1 ? kLight : kHeavy);
		int r = Lookup(i);
		if (r >= 0) {
			cached_weight += weight;
			ASSERT_EQ(1000 + i, r);
		}
	}
	ASSERT_LE(cached_weight, kCacheSize + kCacheSize / 10);
}

TEST(CacheTest, NewId) {
	uint64_t a = cache_->NewId();
	uint64_t b = cache_->NewId();
	ASSERT_NE(a, b);
}

TEST(CacheTest, Prune) {
	Insert(1, 100);
	Insert(2, 200);

	Cache::Handle* handle = cache_->Lookup(EncodeKey(1));
	ASSERT_TRUE(handle);
	cache_->Prune(); // 删除所有lru_
	cache_->Release(handle);

	ASSERT_EQ(100, Lookup(1));
	ASSERT_EQ(-1, Lookup(2));
}

TEST(CacheTest, ZeroSizeCache) {
	delete cache_;
	cache_ = NewLRUCache(0);

	Insert(1, 100);
	ASSERT_EQ(-1, Lookup(1));
}

int main()
{
	test_HitAndMiss();

	test_Erase();

	test::RunAllTests();

	system("pause");
	return 0;
}