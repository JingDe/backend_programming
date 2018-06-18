#include"skiplist_lite.h"
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

struct CMP {
	/*int compare(const int& a, int b) const
	{
	if (a < b)
	return -1;
	else if (a == b)
	return 0;
	else
	return 1;
	}*/

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

TEST(SkipTest, EMPTY)
{
	Comparator cmp;
	SkipList slist(4, cmp);
	slist.Display();

	slist.Insert(8);
	slist.Insert(4);
	slist.Insert(5);
	slist.Display();

	printf("end empty\n");
}

TEST(SkipTest, InsertAndLookup)
{
	const int N = 200;
	const int R = 500;
	std::set<int> keys;
	Comparator cmp;
	SkipList list(6, cmp);
	for (int i = 0; i < N; i++)
	{
		int key = rand() % R; // rand()����ֵ[0, RAND_MAX], RAND_MAX����32767
		if (keys.insert(key).second) // set.insert()����ֵstd::pair<iterator,bool>
			list.Insert(key); // key�ķ�Χ[0, R-1]
	}

	for (int i = 0; i < R; i++)
	{
		if (list.Contains(i))
			ASSERT_EQ(keys.count(i), 1);
		else
			ASSERT_EQ(keys.count(i), 0);
	}

	list.Display();
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

	SkipList list_;

public:
	ConcurrentTest() :list_(8, Comparator()) {}

	// ����һ��key, ��Ҫ�ⲿͬ��
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

		int pos = RandomTarget(rnd);
		
		list_.Display();
	}
};

const uint32_t ConcurrentTest::K;

// ConcurrentTest �ĵ��̲߳���
TEST(SkipTest, ConcurrentWithoutThreads)
{
	ConcurrentTest test;
	Random rnd(test::RandomSeed());
	for (int i = 0; i < 10000; i++)
	{
		test.ReadStep(&rnd);
		test.WriteStep(&rnd);
	}
}

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
	printf("in ConcurrentReader\n");

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
		printf("Wait Running...\n");
		state.Wait(TestState::RUNNING);
		printf("RUNNING\n");
		for (int i = 0; i < kSize; i++)
			state.t_.WriteStep(&rnd);
		state.quit_flag_.Release_Store(&state); // �����NULL����
		state.Wait(TestState::DONE);
		printf("DONE\n");
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