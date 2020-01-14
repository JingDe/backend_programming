-------------------------------------------
/*
*
*	本文档记录log4cxx动态库的相关信息
*
*/
-------------------------------------------


-----------------------------------------------------------------
Version（1.0）
1.介绍
Log4cxx是开放源代码项目Apache Logging Service的子项目之一，是Java社区著名的log4j的c+移植版，用于为C+程序提供日志功能，以便开发者对目标程序进行调试和审计，有关log4cxx的更多信息可以从Apache Loggin Service的网站http://logging.apache.org获得。
 
2.获取软件包
log4cxx需要两个辅助库，可以从官方网站获得合适的版本：https://apr.apache.org/download.cgi
我们拿最新的软件包：
 apr-1.5.0.tar.gz
 apr-util-1.5.3.tar.gz
接下来获取log4cxx，下载链接：https://logging.apache.org/log4cxx/download.html
我们拿最新的软件包：
 apache-log4cxx-0.10.0.tar.gz
 
3.编译安装
tar zxvf  apr-1.5.0.tar.gz
首先安装apr-1.5.0，切换cd apr-1.5.0，配置./configure --prefix=/usr/local/apr，接着make， make install
--------------------
编译安装过程中libuuid.a遇到点问题，error如下：
/usr/bin/ld: /usr/lib/gcc/x86_64-redhat-linux/4.4.6/../../../../lib64/libuuid.a(gen_uuid.o): relocation R_X86_64_32 against `.rodata.str1.1' can not be used when making a shared object; recompile with -fPIC
/usr/lib/gcc/x86_64-redhat-linux/4.4.6/../../../../lib64/libuuid.a: could not read symbols: Bad value
collect2: ld returned 1 exit status
make[1]: *** [libapr-1.la] Error 1
make[1]: Leaving directory `/opt/log4cxx/apr-1.5.0'
make: *** [all-recursive] Error 1
--------------------
问题的解决过程：
wget http://downloads.sourceforge.net/e2fsprogs/e2fsprogs-1.41.14.tar.gz
tar xvzf  e2fsprogs-1.41.14.tar.gz
 
export CFLAGS="-fPIC"
./configure --prefix=/usr/local/e2fsprogs
make
make install
make install-libs
cp /usr/local/e2fsprogs/lib/libuuid.a /usr/lib64/
--------------------
 
tar zxvf apr-util-1.5.3.tar.gz
接着安装apr-util-1.5.3，切换至cd ../apr-util-1.5.3， ./configure --prefix=/usr/local/apr-util --with-apr=/usr/local/apr，接着make，make install
 
tar zxvf apache-log4cxx-0.10.0.tar.gz
最后安装apache-log4cxx-0.10.0，注意我们需要日志支持中文显示，切换至cd ../apache-log4cxx-0.10.0，配置./configure --prefix=/usr/local/log4cxx --with-apr=/usr/local/apr --with-apr-util=/usr/local/apr-util --with-charset=utf-8 --with-logchar=utf-8
注意配置前需进行以下操作：
1）vim src/main/cpp/inputstreamreader.cpp
增加#include <string.h>；
否则会出现inputstreamreader.cpp:66: error: 'memmove' was not declared in this scope
make[3]: *** [inputstreamreader.lo] 错误 1
2）vim src/main/cpp/socketoutputstream.cpp
增加#include <string.h>；
否则会出现socketoutputstream.cpp:52: error: 'memcpy' was not declared in this scope
3）vim src/examples/cpp/console.cpp
增加#include <string.h>，#include <stdio.h>；
否则会出现
console.cpp: In function ‘int main(int, char**)’:
console.cpp:58: 错误：‘puts’在此作用域中尚未声明
 
 4.测试
//TODO 


-----------------------------------------------------------------
Version(1.1)
将日志打出的线程名由0x125FF58CB2改为十进制的 12456.
此修改在 apache-log4cxx-0.10.0/src/main/cpp/loggingevent.cpp中，
修改如下：
增加 
#include <sys/syscall.h>
#include <stdlib.h>
#include <stdio.h>
#define gettid() syscall(__NR_gettid)
重写
const LogString LoggingEvent::getCurrentThreadName()
{
    long threadId = gettid();
    char result[20];
    snprintf (result, sizeof (result), "%ld", threadId);
    return result;
}
在不同的操作系统 make之后，liblog4cxx.a          liblog4cxx.la         liblog4cxx.lai        liblog4cxx.so         liblog4cxx.so.10      liblog4cxx.so.10.0.0。
替换到
cib下 lib/    （32位）
lib64_4.0/  （owl4.0）
lib64_4.2/  （owl4.2）
