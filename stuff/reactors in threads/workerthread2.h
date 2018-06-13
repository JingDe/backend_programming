
// 原来的工作线程只处理一个连接，现在的工作线程处理多个连接
// 需要考虑每个不同连接处于不同的阶段（针对epoll返回的不同事件处理），需要记录这些信息
// muduo里使用channel代替，channel将一个fd与不同的事件回调函数绑定

// channel对一个connfd，有4个固定的handleRead/handleWrite/handleClose/handleError函数，
// 将数据读到inputBuffer，从outputBuffer发送。
// 真正对数据如何处理msgCallback，属于业务逻辑部分。
// 由客户main()传入到TcpServer，TcpServer再传入到连接对象或者处理连接的工作线程对象
