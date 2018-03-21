#include<iostream>

void test()
{
#if defined(OS_WIN) && defined(COMPILER_MSVC) && defined(ARCH_CPU_X86_FAMILY)// Windows on x86
	std::cout << "1\n";

#elif defined(ARCH_CPU_X86_FAMILY) && defined(__GNUC__)// Gcc on x86
	std::cout << "2\n";

#elif defined(ARCH_CPU_X86_FAMILY) && defined(__SUNPRO_CC)// Sun Studio
	std::cout << "3\n";

#elif defined(__APPLE__)// Mac OS
	std::cout << "4\n";

#elif defined(ARCH_CPU_ARM_FAMILY) && defined(__linux__)// ARM Linux
	std::cout << "5\n";

#elif defined(ARCH_CPU_ARM64_FAMILY)// ARM64
	std::cout << "6\n";

#elif defined(ARCH_CPU_PPC_FAMILY) && defined(__GNUC__)// PPC
	std::cout << "7\n";

#elif defined(ARCH_CPU_MIPS_FAMILY) && defined(__GNUC__)// MIPS
	std::cout << "8\n";
#else
	std::cout << "9\n";
#endif
}

void test2()
{
#if defined(LEVELDB_ATOMIC_PRESENT)
	std::cout << "1\n";
#elif defined(__sparcv9) && defined(__GNUC__)
	std::cout << "2\n";
#elif defined(__ia64) && defined(__GNUC__)
	std::cout << "3\n";
#else
	std::cout << "4\n";
#endif
}

void test3()
{
#if defined(_M_X64) // ±àÒëÆ÷¶¨Òå£¡£¡
	std::cout << "0\n"; // #define ARCH_CPU_X86_FAMILY
#elif defined(__x86_64__) 
	std::cout << "1\n";
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
	std::cout << "2\n";
#elif defined(__ARMEL__)
	std::cout << "3\n";
#elif defined(__aarch64__)
	std::cout << "4\n";
#elif defined(__ppc__) || defined(__powerpc__) || defined(__powerpc64__)
	std::cout << "5\n";
#elif defined(__mips__)
	std::cout << "6\n";
#endif
}

int main()
{
#if defined(OS_WIN)
	std::cout << "OS_WIN\n";
#endif

#if defined(COMPILER_MSVC)
	std::cout << "COMPILER_MSVC\n";
#endif 

#if defined(ARCH_CPU_X86_FAMILY)
	std::cout << "ARCH_CPU_X86_FAMILY\n";
#endif 

	test();
	test2();
	test3();

	system("pause");
	return 0;
}