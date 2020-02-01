#include <stdio.h>
#include "glog/logging.h"

int main()
{
   //初始化参数
   FLAGS_logtostderr = false;  //true:标准输出,false:文件输出
   FLAGS_alsologtostderr = true;  //除了日志文件之外是否需要标准输出
   FLAGS_colorlogtostderr = false;  //标准输出带颜色
   FLAGS_logbufsecs = 0;   //设置可以缓冲日志的最大秒数，0指实时输出
   FLAGS_max_log_size = 10;  //日志文件大小(单位：MB)
   FLAGS_stop_logging_if_full_disk = true; //磁盘满时是否记录到磁盘
    google::InitGoogleLogging("mqttserver");
    google::SetLogDestination(google::GLOG_INFO, "./test");
    LOG(INFO) << "this is log";
    LOG(WARNING) << "this is warnning";
    LOG(ERROR) << "this is error";
    google::ShutdownGoogleLogging();
}



