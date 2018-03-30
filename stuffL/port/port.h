#ifndef PORT_H_
#define PORT_H_

#if defined(_MSC_VER) // _MSC_BUILD  _MSC_FULL_VER
#include"port/port_win.h"
#elif defined(__GNUG__) // GNU C++±‡“Î∆˜
#include"port/port_posix.h"
#endif

#endif
