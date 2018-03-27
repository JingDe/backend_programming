#include"skiplist_iterator.h"
#include"common/testharness.h"
#include"common/hash.h"
#include"common/random.h"
#include"common/atomic_pointer.h"
#include"common/arena.h"
#include"port/port.h"
#include"common/MutexLock.h"
#include"common/env.h"

#include<cassert>
#include<set>

typedef uint64_t Key;

struct Comparator {
	int operator()(const int& a, const int& b) const {
		if (a < b) {
			return -1;
		}
		else if (a > b) {
			return +1;
		}
		else {
			return 0;
		}
	}
};

class SkipTest {};

TEST(SkipTest, Empty)
{
	Arena arena;
	Comparator cmp;
	SkipList<Key, Comparator> list(4, cmp, &arena);

	SkipList<Key, Comparator>::Iterator iter(&list);
	ASSERT_TRUE(!iter.Valid());
	iter.SeekToFirst();
	ASSERT_TRUE(!iter.Valid());
	iter.Seek(100);
	ASSERT_TRUE(!iter.Valid());
	iter.SeekToLast();
	ASSERT_TRUE(!iter.Valid());
}


TEST(SkipTest, InsertAndLookup)
{
	Arena arena;
	const int N = 200;
	const int R = 500;
	std::set<Key> keys;
	Comparator cmp;
	SkipList<Key, Comparator> list(4, cmp, &arena);
	// 插入N个元素
	for (int i = 0; i < N; i++)
	{
		int key = rand() % R; // rand()返回值[0, RAND_MAX], RAND_MAX至少32767
		if (keys.insert(key).second) // set.insert()返回值std::pair<iterator,bool>
			list.Insert(key); // key的范围[0, R-1]
	}

	for (int i = 0; i < R; i++)
	{
		if (list.Contains(i))
			ASSERT_EQ(keys.count(i), 1);
		else
			ASSERT_EQ(keys.count(i), 0);
	}

	{
		SkipList<Key, Comparator>::Iterator iter(&list);
		ASSERT_TRUE(!iter.Valid());

		iter.Seek(0); // 第一个元素
		ASSERT_TRUE(iter.Valid());// 跳表的第一个元素等于set的第一个元素
		ASSERT_EQ(*(keys.begin()), iter.key());

		iter.SeekToFirst();
		ASSERT_TRUE(iter.Valid());
		ASSERT_EQ(*(keys.begin()), iter.key());

		iter.SeekToLast();
		ASSERT_TRUE(iter.Valid());
		ASSERT_EQ(*(keys.rbegin()), iter.key());
	}

	for (int i = 0; i < R; i++)
	{
		SkipList<Key, Comparator>::Iterator iter(&list);
		iter.Seek(i); // 查找到等于或大于i的结点

		std::set<Key>::iterator model_iter = keys.lower_bound(i); // 返回迭代器指向元素不小于i
		for (int j = 0; j < 3; j++)
		{
			if (model_iter == keys.end())
			{
				ASSERT_TRUE(!iter.Valid());
				break;
			}
			else
			{
				ASSERT_TRUE(iter.Valid());
				ASSERT_EQ(*model_iter, iter.key());
				++model_iter;
				iter.Next();
			}
		}
	}

	{
		SkipList<Key, Comparator>::Iterator iter(&list);
		iter.SeekToLast();

		for (std::set<Key>::reverse_iterator model_iter = keys.rbegin(); model_iter != keys.rend(); model_iter++)
		{
			ASSERT_TRUE(iter.Valid());
			ASSERT_EQ(*model_iter, iter.key());
			iter.Prev();
		}
		ASSERT_TRUE(!iter.Valid());
	}
}


class ConcurrentTest {
private:
	static const uint32_t K = 4;

	static int key(int key) { return (key >> 40); }
	static int gen(int key) { return (key >> 8) & 0xffffffffu; }
	static int hash(int key) { return key & 0xff; }

	static int HashNumbers(int k, int g) {
		int data[2] = { k, g };
		return Hash(reinterpret_cast<char*>(data), sizeof(data), 0);
	}

	static int MakeKey(int k, int g) {
		assert(sizeof(int) == sizeof(int));
		assert(k <= K);  // We sometimes pass K to seek to the end of the skiplist
		assert(g <= 0xffffffffu);
		return ((k << 40) | (g << 8) | (HashNumbers(k, g) & 0xff));
	}

	static bool IsValidKey(int k) {
		return hash(k) == (HashNumbers(key(k), gen(k)) & 0xff);
	}

	static int RandomTarget(Random* rnd) {
		switch (rnd->Next() % 10) {
		case 0:
			// Seek to beginning
			return MakeKey(0, 0);
		case 1:
			// Seek to end
			return MakeKey(K, 0);
		default:
			// Seek to middle
			return MakeKey(rnd->Next() % K, 0);
		}
	}

	// 用于生成value
	struct State {
		port::AtomicPointer generation[K];
		void Set(int k, intptr_t v)
		{
			generation[k].Release_Store(reinterpret_cast<void*>(v));
		}
		intptr_t Get(int k)
		{
			return reinterpret_cast<intptr_t>(generation[k].Acquire_Load());
		}

		State() {
			for (int k = 0; k < K; k++) {
				Set(k, 0);
			}
		}
	};

	State current_;

	Arena arena_;

	SkipList<Key, Comparator> list_;

public:
	ConcurrentTest() :arena_(), list_(4, Comparator(), &arena_) {}

	// 插入一个key, 需要外部同步
	void WriteStep(Random* rnd)
	{
		const uint32_t k = rnd->Next() % K;
		const intptr_t g = current_.Get(k) + 1;
		const int key = MakeKey(k, g);
		list_.Insert(key);
		current_.Set(k, g);
	}

	void ReadStep(Random* rnd)
	{
		State initial_state;
		for (int k = 0; k < K; k++)
		{
			initial_state.Set(k, current_.Get(k));
		}

		Key pos = RandomTarget(rnd);
		SkipList<Key, Comparator>::Iterator iter(&list_);
		iter.Seek(pos);
		while (true)
		{
			Key current;
			if (!iter.Valid())
				current = MakeKey(K, 0);
			else
			{
				current = iter.key();
				ASSERT_TRUE(IsValidKey(current)) << current;
			}
			ASSERT_LE(pos, current) << " should not go backwards";

			// 验证[pos, current)的结点不存在initial_state
			while (pos < current)
			{
				ASSERT_LE(key(pos), K) << pos;

				// 
				ASSERT_TRUE((gen(pos)==0)  ||  (gen(pos) > static_cast<Key>(initial_state.Get(key(pos)))))
					<< "key: " << key(pos)
					<< "; gen: " << gen(pos)
					<< "; initgen: "
					<< initial_state.Get(key(pos));
			}
		}
	}
};

TEST(SkipTest, ConcurrentWithoutThreads) {
	ConcurrentTest test;
	Random rnd(test::RandomSeed());
	for (int i = 0; i < 10000; i++) {
		test.ReadStep(&rnd);
		test.WriteStep(&rnd);
	}
}

int main()
{
	test::RunAllTests();


	system("pause");
	return 0;
}