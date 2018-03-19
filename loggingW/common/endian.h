#ifndef ENDIAN_H_
#define ENDIAN_H_

/*
#undef PLATFORM_IS_LITTLE_ENDIAN

#if defined(__APPLE__)
	#include <machine/endian.h>
	#if defined(__DARWIN_LITTLE_ENDIAN) && defined(__DARWIN_BYTE_ORDER)
	#define PLATFORM_IS_LITTLE_ENDIAN 	(__DARWIN_BYTE_ORDER == __DARWIN_LITTLE_ENDIAN)
	#endif
#elif defined(OS_SOLARIS)
	#include <sys/isa_defs.h>
	#ifdef _LITTLE_ENDIAN
	#define PLATFORM_IS_LITTLE_ENDIAN true
	#else
	#define PLATFORM_IS_LITTLE_ENDIAN false
	#endif
#elif defined(OS_FREEBSD) || defined(OS_OPENBSD) ||  defined(OS_NETBSD) || defined(OS_DRAGONFLYBSD)
	#include <sys/types.h>
	#include <sys/endian.h>
	#define PLATFORM_IS_LITTLE_ENDIAN (_BYTE_ORDER == _LITTLE_ENDIAN)
#elif defined(OS_HPUX)
	#define PLATFORM_IS_LITTLE_ENDIAN false
#elif defined(OS_ANDROID)
	// Due to a bug in the NDK x86 <sys/endian.h> definition,
	// _BYTE_ORDER must be used instead of __BYTE_ORDER on Android.
	// See http://code.google.com/p/android/issues/detail?id=39824
	#include <endian.h>
	#define PLATFORM_IS_LITTLE_ENDIAN  (_BYTE_ORDER == _LITTLE_ENDIAN)
#elif defined(GCC)
	#include <endian.h>
	#define PLATFORM_IS_LITTLE_ENDIAN (__BYTE_ORDER == __LITTLE_ENDIAN)
#endif
*/


extern bool is_big_endian();

#endif
