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
	typedef std::unique_ptr<Buffer> BufferPtr; //boost::ptr_vector<T>::auto_type����std::unique_ptr���߱��ƶ����壬���Զ��������������
	typedef std::vector<BufferPtr> BufferVector;


	//LogFile��־�ļ���
	std::string basename_;
	size_t rollSize_;
	const int flushInterval_;
	
	Thread thread_; // ��̨�̣߳����𽫻����е���־����д��LogFile��־�ļ���
	Mutex mutex_;
	CondVar cond_;

	bool running_;

	// ˫���壬�ռ���־����
	BufferPtr currentBuffer_;
	BufferPtr nextBuffer_;
	BufferVector buffers_;

	AsyncLogging(const AsyncLogging&);
	void operator=(const AsyncLogging&);
};

#endif
