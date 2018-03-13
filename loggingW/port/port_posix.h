#ifndef PORT_POSIX_H_
#define PORT_POSIX_H_


#include<pthread.h>



//		#if defined(__APPLE__)
//		#include <machine/endian.h>
//			#if defined(__DARWIN_LITTLE_ENDIAN) && defined(__DARWIN_BYTE_ORDER)
//			#define PLATFORM_IS_LITTLE_ENDIAN \
//					(__DARWIN_BYTE_ORDER == __DARWIN_LITTLE_ENDIAN)
//			#endif
//		#elif defined(OS_SOLARIS)
//		#include <sys/isa_defs.h>
//			#ifdef _LITTLE_ENDIAN
//			#define PLATFORM_IS_LITTLE_ENDIAN true
//			#else
//			#define PLATFORM_IS_LITTLE_ENDIAN false
//			#endif
//		#elif defined(OS_FREEBSD) || defined(OS_OPENBSD) ||\
//			  defined(OS_NETBSD) || defined(OS_DRAGONFLYBSD)
//		#include <sys/types.h>
//		#include <sys/endian.h>
//		#define PLATFORM_IS_LITTLE_ENDIAN (_BYTE_ORDER == _LITTLE_ENDIAN)
//		#elif defined(OS_HPUX)
//		#define PLATFORM_IS_LITTLE_ENDIAN false
//		#elif defined(OS_ANDROID)
//		// Due to a bug in the NDK x86 <sys/endian.h> definition,
//		// _BYTE_ORDER must be used instead of __BYTE_ORDER on Android.
//		// See http://code.google.com/p/android/issues/detail?id=39824
//		#include <endian.h>
//		#define PLATFORM_IS_LITTLE_ENDIAN  (_BYTE_ORDER == _LITTLE_ENDIAN)
//		#else
//		#include <endian.h>
//		#endif
//
//#include <pthread.h>
//		#if defined(HAVE_CRC32C)
//		#include <crc32c/crc32c.h>
//		#endif  // defined(HAVE_CRC32C)
//		#ifdef HAVE_SNAPPY
//		#include <snappy.h>
//		#endif  // defined(HAVE_SNAPPY)
//	#include <stdint.h>
//	#include <string>
//	#include "port/atomic_pointer.h"
//	#include "port/thread_annotations.h"
//
//#ifndef PLATFORM_IS_LITTLE_ENDIAN
//#define PLATFORM_IS_LITTLE_ENDIAN (__BYTE_ORDER == __LITTLE_ENDIAN)
//#endif
//
//	#if defined(__APPLE__) || defined(OS_FREEBSD) ||\
//		defined(OS_OPENBSD) || defined(OS_DRAGONFLYBSD)
//	// Use fsync() on platforms without fdatasync()
//	#define fdatasync fsync
//	#endif
//
//#if defined(OS_ANDROID) && __ANDROID_API__ < 9
//// fdatasync() was only introduced in API level 9 on Android. Use fsync()
//// when targetting older platforms.
//#define fdatasync fsync
//#endif

namespace port {
	//extern pid_t gettid();

	extern int getThreadID();
	
	class Mutex {
	public:
		Mutex();
		~Mutex();

		void Lock();
		void UnLock();
		void AssertHeld();

	private:
		pthread_mutex_t mutex_;

		Mutex(const Mutex&);
		void operator=(const Mutext&);
	};

}

#endif