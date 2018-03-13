#ifndef ASYNCLOGGING_H_
#define ASYNCLOGGING_H_

#include"FixedBuffer.h"

class CondVar;
class Mutex;
class Thread;

class AsyncLogging {
public:
	AsyncLogging(std::string basename, int flushInterval);

private:
	/*typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
	typedef boost::ptr_vector<Buffer> BufferVector;
	typedef BufferVector::auto_type BufferPtr;*/
	typedef FixedBuffer<kLargeBuffer> Buffer;
	typedef std::unique_ptr<Buffer> BufferPtr; //boost::ptr_vector<T>::auto_type类似std::unique_ptr，具备移动语义，能自动管理对象生命期
	typedef std::vector<BufferPtr> BufferVector;


	//LogFile日志文件名
	std::string basename_;
	size_t rollSize_;
	const int flushInterval_;
	
	Thread thread_; // 后台线程，负责将缓存中的日志数据写到LogFile日志文件中
	Mutex mutex_;
	CondVar cond_;

	bool running_;

	// 双缓冲，收集日志数据
	BufferPtr currentBuffer_;
	BufferPtr nextBuffer_;
	BufferVector buffers_;

	AsyncLogging(const AsyncLogging&);
	void operator=(const AsyncLogging&);
};

#endif
