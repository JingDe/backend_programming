#include"AsyncLogging.h"
#include"LogFile.h"

#include<utility>
#include<cassert>

AsyncLogging::AsyncLogging(const std::string& basename, size_t rollSize, int flushInterval) :
	flushInterval_(flushInterval),
	running_(false),
	basename_(basename),
	rollSize_(rollSize),
	thread_(std::bind(&AsyncLogging::threadFunc, this), "LoggingThread"),
	mutex_(),
	cond_(&mutex_),
	currentBuffer_(new Buffer),
	nextBuffer_(new Buffer),
	buffers_()
{
	currentBuffer_->bzero();
	nextBuffer_->bzero();
	buffers_.reserve(16); //增加vector的容量到16，若分配新内存则迭代器、引用失效
}

AsyncLogging::~AsyncLogging()
{
	running_ = false;
	thread_.stop();
}

void AsyncLogging::start()
{
	running_ = true;
	thread_.start();
}

void AsyncLogging::threadFunc()
{
	assert(running_ = true);

	LogFile output(basename_, rollSize_, false);
	BufferPtr newBuffer1(new Buffer);
	BufferPtr newBuffer2(new Buffer);
	newBuffer1->bzero();
	newBuffer2->bzero();
	BufferVector buffersToWrite; // vector<unique_ptr<Buffer> >
	buffersToWrite.reserve(16);
	while (running_ == true)
	{
		assert(newBuffer1  &&  newBuffer1->length() == 0);
		assert(newBuffer2  &&  newBuffer2->length() == 0);
		assert(buffersToWrite.empty());

		{
			port::MutexLock lock(mutex_);
			if (buffers_.empty())
				cond_.waitForSeconds(flushInterval_); // 等待flushInterval_秒
			buffers_.push_back(currentBuffer_.release()); //currentBuffer_释放拥有的Buffer对象，返回指向该Buffer的指针
			currentBuffer_ = std::move(newBuffer1); // 高效转移newBuffer1的资源到currentBuffer_
			buffersToWrite.swap(buffers_); //交换容器的内容
			if (!nextBuffer_)
				nextBuffer_ = std::move(newBuffer2);
		}

		assert(!buffersToWrite.empty());

		if (buffersToWrite.size() > 25) // 元素个数
		{
			buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end()); // 删除容器的元素
		}

		for (size_t i = 0; i < buffersToWrite.size(); i++)
		{
			output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
		}

		if (buffersToWrite.size() > 2)
		{
			buffersToWrite.resize(2); // 改变容器的大小包含2个元素
		}

		if (!newBuffer1)
		{
			assert(!buffersToWrite.empty());
			newBuffer1 = buffersToWrite.back();
			buffersToWrite.pop_back();
			newBuffer1->reset(); // 修改unique_ptr管理的对象
		}

		if (!newBuffer2)
		{
			assert(!buffersToWrite.empty());
			newBuffer2 = buffersToWrite.back();
			buffersToWrite.pop_back();
			newBuffer2->reset(); // 修改unique_ptr管理的对象
		}

		buffersToWrite.clear(); //删除容器的元素
		output.flush();
	}
	output.flush();
}

void AsyncLogging::append(const char* msg, int len)
{
	port::MutexLock lock(mutex_);
	if (currentBuffer_->available() > len)
	{
		currentBuffer_->append(msg, len);
	}
	else
	{
		buffers_.push_back(currentBuffer_.release());

		if (nextBuffer_)
			currentBuffer_ = std::move(nextBuffer_);
		else
			currentBuffer_.reset(new Buffer);

		currentBuffer_->append(msg, len);
		cond_.Signal(); // 通知后台线程
	}
}