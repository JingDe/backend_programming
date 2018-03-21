#ifndef ATOMIC_H_
#define ATOMIC_H_

// gccÔ­×Ó²Ù×÷

template<typename T>
class AtomicIntegerT : noncopyable {
public:
	AtomicIntegerT() :value_(0) {}

	AtomicIntegerT(const AtomicIntegerT& that) :value_(that.get()) {}

	AtomicIntegerT& operator=(const AtomicIntegerT& that)
	{
		getAndSet(that.get());
		return *this;
	}

	T get()
	{
		// in gcc >= 4.7: __atomic_load_n(&value_, __ATOMIC_SEQ_CST)
		return __sync_val_compare_and_swap(&value_, 0, 0);
	}

	T getAndAdd(T x)
	{
		// in gcc >= 4.7: __atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST)
		return __sync_fetch_and_add(&value_, x);
	}

	T addAndGet(T x)
	{
		return getAndAdd(x) + x;
	}

	T incrementAndGet()
	{
		return addAndGet(1);
	}

	T decrementAndGet()
	{
		return addAndGet(-1);
	}

	void add(T x)
	{
		getAndAdd(x);
	}

	void increment()
	{
		incrementAndGet();
	}

	void decrement()
	{
		decrementAndGet();
	}

	T getAndSet(T newValue)
	{
		// in gcc >= 4.7: __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST)
		return __sync_lock_test_and_set(&value_, newValue);
	}

private:
	volatile T value_; 
	/*  within a single thread of execution, volatile accesses cannot be optimized out or 
	reordered with another visible side effect that is sequenced-before or sequenced-after the volatile access. 
	This makes volatile objects suitable for communication with a signal handler, 
	but not with another thread of execution, see std::memory_order */
};

typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
typedef detail::AtomicIntegerT<int64_t> AtomicInt64;

#endif 
