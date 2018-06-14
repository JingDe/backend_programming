
LogStream类：字符串流，提供针对不同数据类型的operator<<接口，转换成字符串
	使用：
	LogStream ls;
	ls<<3<<"hello"<<1.21<<false;
	//cout<<ls.buffer()<<endl;
	
	Fmt类：格式化字符串，构造函数中对格式化字符串和需要格式化的数组进行格式化，得到格式化之后的字符串
	使用：
	Fmt fmt("%d", 3);
	cout<<fmt.data()<<endl;
	
	!!!
	LogStream ls;
	Fmt fmt("%d", 3);
	!!!
	ls.append(fmt.data(), fmt.length());    
	-->>	  ls<<Fmt("%d", 3);
	
	
	
Logger类：写一条日志的动作，调用输出函数和刷新函数输出到目的地
	从构造函数开始构造一个LogStream对象，在析构函数里调用全局输出函数、全局flush函数
	在LogStream类的基础之上，添加了输出字符串流的动作
	定义了方便客户使用的LOG_DEBUG等宏
	
	使用：
	Logger::setOutput(defaultoutput);
	Logger::setFlush(defaultFlush);
	cout<<Logger(__FILE__, __LINE__, Logger::LOG_DEBUG, __FUNC__).stream();
	!!!
	Logger(__FILE__, __LINE__, Logger::DEBUG, __FUNC__).stream()<<"error "<<3<<"!=0"<<false;
	-->>   LOG_DEBUG<<"error "<<3<<"!=0 "<<false;
	
	
	
LogFile类：日志文件，在AppendFile基础之上，提供append接口和flush接口，同时提供了线程安全性，同步append操作和flush操作等
	当文件过大，重新创建；（每次append检查）
	当文件过老，重新创建；（append了多次后检查）
	
	当文件append了多次后，刷新；
	
	
	日志文件名格式：XXX、时间、主机名、进程id
	
	使用：
	LogFile lgfile("test_LogFile", 100 * 1000, true, 3, 1024);
	// 文件基本名称，文件rollsize, 线程安全，刷新间隔，每append多少次检查刷新和重新创建
	Logger::setOutput(/*append到lgfile的函数*/);
	Logger::setFlush(/*flush lgfile的函数*/);
	LOG_DEBUG<<"debug info";

	
AsynLogging类：	Logger类和LogFile类的中间层，前端Logger输出日志数据到AsyncLogging类的buffer，
	AsyncLogging类的后台线程输出buffer数据到LogFile类对象。
	调用LogFile的接口append和flush，不需要处理重新创建日志文件等细节。
	前端append对buffer操作加锁，后端后台线程输出buffer数据到LogFile不加锁。

	使用：
	AsyncLogging asynlog("test_AsyncLogging", kRollSize);
	log.start();
	
	Logger::setOutput(/*调用asynclog的append接口*/);
	LOG_DEBUG<<"debug info";
	
	log.stop();
	
	
	
	
单线程使用：
	Logger类提供的宏，输出到标准输出
	Logger类提供的宏，输出到不加锁的LogFile对象
	
多线程使用：
	Logger类提供的宏，输出到加锁的LogFile对象
	Logger类提供的宏（间接调用AsynLogging类提供的append接口)，输出到AsyncLogging类的加锁buffer

	Logger类与AsynLogging类的相似和不同：
	<1> Logger类只提供一个buffer（LogStream对象），
		前端调用LOG_DEBUG等宏（Logger设置类函数output和flush到加锁的LogFile），
		后端在析构函数中输出到目标位置
	<2> AsynLogging类提供双缓冲，
		前端调用LOG_DEBUG等宏（Logger设置类函数output和flush到AsynLogging的append接口，append到双buffer中），
		后端创建后台线程将buffer数据输出到不加锁的LogFile
		
		
	