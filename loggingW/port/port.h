#ifndef PORT_H_
#define PORT_H_

#if defined(OS_WIN)
#include"port/port_win.h"
#elif defined(POSIX)
#include"port/port_posix.h"
#endif

#endif
