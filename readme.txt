
学习和实践，各种开源项目的组件
例如Nginx的数据结构、常用算法、内存池等等等，例如leveldb、sqlite等等

fgets // 读到eof、读到256-1个字符、读到\n时，读取终止
        // 最后添加一个\0, 读到\n时，最后为\n\0，
fread // 读size个1字节，不添加\0，需要调用者根据返回值添加0
        // 返回读到的对象个数

TODO：

# Nginx的epoll机制
# Nginx的Windows平台select事件模块
# nginx的信号
# Nginx的内存池

# Nginx的文件缓存模块
# Nginx的slab共享内存
# Nginx的定时器

# 比较Nginx的多进程与muduo的多线程，实现上细节的不同点


# rpc框架
https://github.com/hjk41/tinyrpc



#makefile



# protobuf
这是个平台无关的库，装个VS2017，把它编译运行起来，单步跟踪、goto definition什么的都很方便。

protobuf 大概分成两部分：compiler 和 runtime 。

compiler 的前端是手写的递归下降 parser ，如果你学过编译原理，很容易读懂。这个编译器的后端是各个目标语言的代码生成器，可以选你熟悉的来读。前后端通过 descriptor 联系起来，非常清晰，也便于扩展。

runtime 主要功能是序列化和反序列化。每个目标语言各有一套，可以根据需要来读，一般要结合生成的代码一起读。


# sqlite


# leveldb




# chromium


# log(LOG_DEBUG, "", ...)形式的日志


# extern C总结
