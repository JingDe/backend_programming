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
#include<iostream>

typedef uint64_t Key;

//struct Comparator {
//	int operator()(const Key& a, const Key& b) const {
//		if (a < b) {
//			return -1;
//		}
//		else if (a > b) {
//			return +1;
//		}
//		else {
//			return 0;
//		}
//	}
//};
struct Comparator {
	int operator()(const Key& a, const Key& b) const {
		
		std::cout << a << std::endl; // WHY:�����Ͳ����쳣

		if (a < b) { // a�޷������ڴ��쳣��
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

//TEST(SkipTest, Empty)
//{
//	Arena arena;
//	Comparator cmp;
//	SkipList<Key, Comparator> list(4, cmp, &arena);
//
//	SkipList<Key, Comparator>::Iterator iter(&list);
//	ASSERT_TRUE(!iter.Valid());
//	iter.SeekToFirst();
//	ASSERT_TRUE(!iter.Valid());
//	iter.Seek(100);
//	ASSERT_TRUE(!iter.Valid());
//	iter.SeekToLast();
//	ASSERT_TRUE(!iter.Valid());
//}


//TEST(SkipTest, InsertAndLookup)
//{
//	Arena arena;
//	const int N = 200;
//	const int R = 500;
//	std::set<Key> keys;
//	Comparator cmp;
//	SkipList<Key, Comparator> list(4, cmp, &arena);
//	// ����N��Ԫ��
//	for (int i = 0; i < N; i++)
//	{
//		Key key = rand() % R; // rand()����ֵ[0, RAND_MAX], RAND_MAX����32767
//		if (keys.insert(key).second) // set.insert()����ֵstd::pair<iterator,bool>
//			list.Insert(key); // key�ķ�Χ[0, R-1]
//	}
//
//	for (int i = 0; i < R; i++)
//	{
//		if (list.Contains(i))
//			ASSERT_EQ(keys.count(i), 1);
//		else
//			ASSERT_EQ(keys.count(i), 0);
//	}
//
//	{
//		SkipList<Key, Comparator>::Iterator iter(&list);
//		ASSERT_TRUE(!iter.Valid());
//
//		iter.Seek(0); // ��һ��Ԫ��
//		ASSERT_TRUE(iter.Valid());// ����ĵ�һ��Ԫ�ص���set�ĵ�һ��Ԫ��
//		ASSERT_EQ(*(keys.begin()), iter.key());
//
//		iter.SeekToFirst();
//		ASSERT_TRUE(iter.Valid());
//		ASSERT_EQ(*(keys.begin()), iter.key());
//
//		iter.SeekToLast();
//		ASSERT_TRUE(iter.Valid());
//		ASSERT_EQ(*(keys.rbegin()), iter.key());
//	}
//
//	for (int i = 0; i < R; i++)
//	{
//		SkipList<Key, Comparator>::Iterator iter(&list);
//		iter.Seek(i); // ���ҵ����ڻ����i�Ľ��
//
//		std::set<Key>::iterator model_iter = keys.lower_bound(i); // ���ص�����ָ��Ԫ�ز�С��i
//		for (int j = 0; j < 3; j++)
//		{
//			if (model_iter == keys.end())
//			{
//				ASSERT_TRUE(!iter.Valid());
//				break;
//			}
//			else
//			{
//				ASSERT_TRUE(iter.Valid());
//				ASSERT_EQ(*model_iter, iter.key());
//				++model_iter;
//				iter.Next();
//			}
//		}
//	}
//
//	{
//		SkipList<Key, Comparator>::Iterator iter(&list);
//		iter.SeekToLast();
//
//		for (std::set<Key>::reverse_iterator model_iter = keys.rbegin(); model_iter != keys.rend(); model_iter++)
//		{
//			ASSERT_TRUE(iter.Valid());
//			ASSERT_EQ(*model_iter, iter.key());
//			iter.Prev();
//		}
//		ASSERT_TRUE(!iter.Valid());
//	}
//}

// ����:һ��writer�Ͷ��readerʱ��ֻ��reader�ĵ���������ʱͬ������reader�����ܹ���������������ʱ�������������
// ���ڲ��벢�����������ܹ۲쵽����������֮�󴴽�����ֵ�����Ǿ��Բ��ܶ�ʧ����������ʱ�����ݡ�
class ConcurrentTest {
private:
	static const uint32_t K = 4;

	// We generate multi-part keys:
	//     <key,gen,hash>
	// where:
	//     key is in range [0..K-1]
	//     gen is a generation number for key
	//     hash is hash(key,gen)
	//
	static uint64_t key(Key key) { return (key >> 40); }
	static uint64_t gen(Key key) { return (key >> 8) & 0xffffffffu; }
	static uint64_t hash(Key key) { return key & 0xff; }

	static uint64_t HashNumbers(uint64_t k, uint64_t g) {
		uint64_t data[2] = { k, g };
		return Hash(reinterpret_cast<char*>(data), sizeof(data), 0);
	}

	static Key MakeKey(uint64_t k, uint64_t g) {
		assert(sizeof(Key) == sizeof(uint64_t));
		assert(k <= K);  // We sometimes pass K to seek to the end of the skiplist
		assert(g <= 0xffffffffu);
		return ((k << 40) | (g << 8) | (HashNumbers(k, g) & 0xff));
	}

	static bool IsValidKey(Key k) {
		return hash(k) == (HashNumbers(key(k), gen(k)) & 0xff);
	}

	static Key RandomTarget(Random* rnd) {
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

	// ��������value
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

	// The insertion code picks a random key, sets gen to be 1 + the last
	// generation number inserted for that key, and sets hash to Hash(key,gen).
	// ����һ��key, ��Ҫ�ⲿͬ��
	void WriteStep(Random* rnd)
	{
		const uint32_t k = rnd->Next() % K;
		const intptr_t g = current_.Get(k) + 1;
		const Key key = MakeKey(k, g);
		list_.Insert(key);
		current_.Set(k, g);
	}

	// At the beginning of a read, we snapshot the last inserted
	// generation number for each key.  We then iterate, including random
	// calls to Next() and Seek().  For every key we encounter, we
	// check that it is either expected given the initial snapshot or has
	// been concurrently added since the iterator started.
	void ReadStep(Random* rnd)
	{
		// �����ʼ״̬
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
				ASSERT_TRUE(IsValidKey(current)) << current; // exit??
			}
			ASSERT_LE(pos, current) << " should not go backwards";

			// ��֤[pos, current)�Ľ�㲻����initial_state
			while (pos < current)
			{
				ASSERT_LE(key(pos), K) << pos;

				// 
				ASSERT_TRUE((gen(pos)==0)  ||  (gen(pos) > static_cast<Key>(initial_state.Get(key(pos)))))
					<< "key: " << key(pos)
					<< "; gen: " << gen(pos)
					<< "; initgen: "
					<< initial_state.Get(key(pos));

				// ����Чkey�ռ�ǰ������һ��key
				if (key(pos) < key(current))
					pos = MakeKey(key(pos) + 1, 0);
				else
					pos = MakeKey(key(pos), gen(pos) + 1);
			}

			if (!iter.Valid())
				break;

			if (rnd->Next() % 2)
			{
				iter.Next();
				pos = MakeKey(key(pos), gen(pos) + 1);
			}
			else
			{
				Key new_target = RandomTarget(rnd);
				if (new_target > pos)
				{
					pos = new_target;
					iter.Seek(new_target);
				}
			}
		}
	}
};

const uint32_t ConcurrentTest::K;


//TEST(SkipTest, ConcurrentWithoutThreads) {
//	ConcurrentTest test;
//	Random rnd(test::RandomSeed());
//	int N = 100; // 10000;
//	for (int i = 0; i < N; i++) {
//		test.ReadStep(&rnd);
//		test.WriteStep(&rnd);
//	}
//}


class TestState {
public:
	ConcurrentTest t_;
	int seed_;
	port::AtomicPointer quit_flag_;

	enum ReaderState {
		STARTING,
		RUNNING,
		DONE
	};

	explicit TestState(int s)
		:seed_(s), quit_flag_(NULL), state_(STARTING), state_cv_(&mu_) {}

	void Wait(ReaderState s)
	{
		MutexLock lock(mu_);
		while (state_ != s) {
			state_cv_.Wait();
		}
	}

	void Change(ReaderState s) {
		MutexLock lock(mu_);
		state_ = s;
		state_cv_.Signal();
	}

private:
	port::Mutex mu_;
	ReaderState state_;
	port::CondVar state_cv_;
};

static void ConcurrentReader(void* arg) {
	//printf("in ConcurrentReader\n");

	TestState* state = reinterpret_cast<TestState*>(arg);
	Random rnd(state->seed_);
	int64_t reads = 0;
	state->Change(TestState::RUNNING);
	while (!state->quit_flag_.Acquire_Load())
	{
		state->t_.ReadStep(&rnd); // 
		++reads;
	}
	state->Change(TestState::DONE);
}

static void RunConcurrent(int run)
{
	const int seed = test::RandomSeed() + (run * 100);
	Random rnd(seed);
	const int N = 1000;
	const int kSize = 1000;
	for (int i = 0; i < N; i++)
	{
		if ((i % 100) == 0)
			fprintf(stderr, "Run %d of %d\n", i, N);
		TestState state(seed + 1);
		Env::Default()->Schedule(ConcurrentReader, &state);
		//printf("Wait Running...\n");
		state.Wait(TestState::RUNNING);
		//printf("RUNNING\n");
		for (int i = 0; i < kSize; i++)
			state.t_.WriteStep(&rnd);
		state.quit_flag_.Release_Store(&state); // �����NULL����
		state.Wait(TestState::DONE);
		//printf("DONE\n");
	}
}

TEST(SkipTest, Concurrent1) { RunConcurrent(1); }
TEST(SkipTest, Concurrent2) { RunConcurrent(2); }
TEST(SkipTest, Concurrent3) { RunConcurrent(3); }
TEST(SkipTest, Concurrent4) { RunConcurrent(4); }
TEST(SkipTest, Concurrent5) { RunConcurrent(5); }

int main()
{
	test::RunAllTests();


	system("pause");
	return 0;
}