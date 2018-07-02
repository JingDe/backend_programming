
定时器设计：
一个定时器包含：到期时间、回调函数

定时器的使用方式：
（1）设置epoll_wait的timeout，timeout时处理超时定时器
（2）使用timerfd通知epoll_wait，统一事件源（同IO、信号事件）
前者通过epoll_wait返回，通知超时，处理超时定时器；
后者通过timerfd_settime设置时间，时间到达后，timerfd可读

定时器的组织方式：升序链表、时间轮、时间堆/二叉堆、二叉搜索树set/map
需要的接口：getMinExpired(), handleExpired(time_t), #resetTimerfd()
考虑同步访问？






时间轮，以固定的频率调用tick。在走到某个时刻的时候，需要遍历所有链表。
时间轮适用于固定周期的定时事件。

时间堆，以最近超时的时间，调用tick。
时间堆适用于


