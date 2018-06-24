
/*
	序列化：
	（1）对象本身可序列化和反序列化：
	定义有serialize(StreamBuffer&)的类，将本对象序列化到buffer参数中
	（2）对象可以memcpy。
	如，消息头
	struct MessageHeader {
            int64_t seq_num;
            uint64_t protocol_id; // 即uniqueId
            uint32_t is_async;
        };
	的序列化，通过调用StreamBuffer.Write()函数，写进StreamBuffer的buffer中
	（3）出错
	
	
	
*/

