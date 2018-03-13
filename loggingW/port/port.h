#ifndef PORT_H_
#define PORT_H_

#if defined(WIN32)
#include"port/port_win32.h"
#elif defined(POSIX)
#include"port/port_posix.h"
#endif

#endif
