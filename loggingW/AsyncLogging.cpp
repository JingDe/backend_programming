#include"AsyncLogging.h"

#include<utility>

AsyncLogging::AsyncLogging(const std::string& basename, size_t rollSize, int flushInterval) :
	flushInterval_(flushInterval),
	running_(false),
	basename_(basename),
	rollSize_(rollSize),
	thread_(std::bind(&AsyncLogging::threadFunc, this), "LoggingThread"),
	mutex_(),
	cond_(mutex_),
	currentBuffer_(new Buffer),
	nextBuffer_(new Buffer),
	buffers_()
{
	currentBuffer_->bzero();
	nextBuffer_->bzero();
	buffers_.reserve(16); //����vector�����������������ڴ��������������ʧЧ
}


void AsyncLogging::threadFunc()
{
	assert(running_ = true);

	LogFile output(basename_, rollSize_, false);
	BufferPtr newBuffer1(new Buffer);
	BufferPtr newBuffer2(new Buffer);
	newBuffer1->bzero();
	newBuffer2->bzero();
	BufferVector buffersToWrite;
	buffersToWrite.reserve(16);
	while (running_ == true)
	{
		assert(newBuffer1  &&  newBuffer1->length() == 0);
		assert(newBuffer2  &&  newBuffer2->length() == 0);
		assert(buffersToWrite.empty());

		{
			port::MutexLock lock(mutex_);
			if (buffers_.empty())
				cond_.waitForSeconds(flushInterval_);
			buffers_.push_back(currentBuffer_.release()); //currentBuffer_�ͷ�ӵ�е�Buffer���󣬷���ָ���Buffer��ָ��
			currentBuffer_ = std::move(newBuffer1); // ��Чת��newBuffer1����Դ��currentBuffer_
			buffersToWrite.swap(buffer_); //
		}
	}
}