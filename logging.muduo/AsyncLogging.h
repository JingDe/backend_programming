#ifndef ASYNCLOGGING_H_
#define ASYNCLOGGING_H_

#include"common/FixedBuffer.h"
#include"port/port.h"
#include"thread.muduo/Thread.h"

#include<vector>
#include<memory>
#include<algorithm>


class AsyncLogging {
public:
	AsyncLogging(const std::string& basename, size_t rollSize, int flushInterval=3);
	~AsyncLogging();

	void start();
	void stop();
	void threadFunc();

	void append(const char* msg, int len);

private:
	/*typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
	typedef boost::ptr_vector<Buffer> BufferVector;
	typedef BufferVector::auto_type BufferPtr;*/
	typedef FixedBuffer<kLargeBuffer> Buffer;
	typedef std::unique_ptr<Buffer> BufferPtr; //boost::ptr_vector<T>::auto_type����std::unique_ptr���߱��ƶ����壬���Զ��������������
	typedef std::vector<BufferPtr> BufferVector;


	//LogFile��־�ļ���
	std::string basename_;
	size_t rollSize_;
	const int flushInterval_;
	
	Thread thread_; // ��̨�̣߳����𽫻����е���־����д��LogFile��־�ļ���
	port::Mutex mutex_; // ͬ����buffer�ķ���
	port::CondVar cond_;

	bool running_;

	// ˫���壬�ռ���־����
	BufferPtr currentBuffer_; // unique_ptr<Buffer>
	BufferPtr nextBuffer_;
	BufferVector buffers_;

	AsyncLogging(const AsyncLogging&);
	void operator=(const AsyncLogging&);
};

#endif
