include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckTypeSize)
include(TestBigEndian)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)
include(CheckCXXSymbolExists)
include(CheckCXXSourceCompiles)

if (CMAKE_TARGET_OS STREQUAL "windows")
	set(CPL_MULTIPROC_WIN32 ON)
else()
	set(CPL_MULTIPROC_PTHREAD ON)
	check_include_file("pthread.h" HAVE_PTHREAD_H)
	check_symbol_exists("PTHREAD_MUTEX_RECURSIVE" "pthread.h" HAVE_PTHREAD_MUTEX_RECURSIVE)
	check_symbol_exists("PTHREAD_MUTEX_ADAPTIVE_NP" "pthread.h" HAVE_PTHREAD_MUTEX_ADAPTIVE_NP)
	check_symbol_exists("pthread_spinlock_t" "pthread.h" HAVE_PTHREAD_SPINLOCK)
endif()

check_function_exists("mremap" HAVE_5ARGS_MREMAP)

check_include_file("assert.h" HAVE_ASSERT_H)

check_function_exists("atoll" HAVE_ATOLL)
check_function_exists("strtoll" HAVE_STRTOLL)
check_function_exists("strtoull" HAVE_STRTOULL)

check_function_exists("getrlimit" HAVE_GETRLIMIT)
check_symbol_exists("RLIMIT_AS" "sys/resource.h" HAVE_RLIMIT_AS)

check_include_file("csf.h" HAVE_CSF_H)

check_include_file("dbmalloc.h" HAVE_DBMALLOC_H)

check_symbol_exists("strtof" "stdlib.h" HAVE_DECL_STRTOF)

check_include_file("direct.h" HAVE_DIRECT_H)

check_include_file("dlfcn.h" HAVE_DLFCN_H)

check_function_exists("vprintf" HAVE_VPRINTF)
if (NOT HAVE_VPRINTF)
	check_function_exists("_doprnt" HAVE_DOPRNT)
endif()

check_include_file("errno.h" HAVE_ERRNO_H)

check_include_file("fcntl.h" HAVE_FCNTL_H)

check_include_file("float.h" HAVE_FLOAT_H)

check_function_exists("getcwd" HAVE_GETCWD)

check_function_exists("iconv" HAVE_ICONV)

set(HAVE_IEEEFP ON)

check_type_size("int8" SIZEOF_INT8)
if (SIZEOF_INT8)
	set(HAVE_INT8 ON)
endif()

check_type_size("int16" SIZEOF_INT16)
if (SIZEOF_INT16)
	set(HAVE_INT16 ON)
endif()

check_type_size("int32" SIZEOF_INT32)
if (SIZEOF_INT32)
	set(HAVE_INT32 ON)
endif()

check_type_size("__uint128_t" SIZEOF_UINT128_T)
if (SIZEOF_UINT128_T)
	set(HAVE_UINT128_T ON)
endif()

check_include_file("inttypes.h" HAVE_INTTYPES_H)

check_include_file("limits.h" HAVE_LIMITS_H)

check_include_file("locale.h" HAVE_LOCALE_H)

check_type_size("long long" SIZEOF_LONG_LONG)
if (SIZEOF_LONG_LONG)
	set(HAVE_LONG_LONG ON)
endif()

check_type_size("uintptr_t" SIZEOF_UINTPTR_T)
if (SIZEOF_UINTPTR_T)
	set(HAVE_UINTPTR_T ON)
endif()

check_include_file("memory.h" HAVE_MEMORY_H)

check_function_exists("snprintf" HAVE_SNPRINTF)

check_include_file("stdint.h" HAVE_STDINT_H)

check_include_file("stdlib.h" HAVE_STDLIB_H)

check_include_file("strings.h" HAVE_STRINGS_H)

check_include_file("string.h" HAVE_STRING_H)

check_function_exists("strtof" HAVE_STRTOF)

check_include_file("sys/stat.h" HAVE_SYS_STAT_H)

check_include_file("sys/types.h" HAVE_SYS_TYPES_H)

check_include_file("unistd.h" HAVE_UNISTD_H)

check_include_file("values.h" HAVE_VALUES_H)

check_function_exists("vprintf" HAVE_VPRINTF)

check_function_exists("vsnprintf" HAVE_VSNPRINTF)

check_function_exists("readlink" HAVE_READLINK)

check_function_exists("posix_spawnp" HAVE_POSIX_SPAWNP)

check_function_exists("posix_memalign" HAVE_POSIX_MEMALIGN)

check_function_exists("vfork" HAVE_VFORK)

check_function_exists("mmap" HAVE_MMAP)

check_function_exists("sigaction" HAVE_SIGACTION)

check_function_exists("statvfs" HAVE_STATVFS)

check_function_exists("statvfs64" HAVE_STATVFS64)

check_function_exists("lstat" HAVE_LSTAT)

if (HAVE_ICONV)
	# Check if iconv() takes const char ** (C)
	check_c_source_compiles("
		#include <iconv.h>
		int main(int argc, char ** argv)
		{ return iconv(0, (const char **)0, 0, (char**)0, 0); }
		" ICONV_NEEDS_C_CONST)
	# Check if iconv() takes char ** (C)
	check_c_source_compiles("
		#include <iconv.h>
		int main(int argc, char ** argv)
		{ return iconv(0, (char **)0, 0, (char**)0, 0); }
		" ICONV_NEEDS_C_NON_CONST)
	if (ICONV_NEEDS_C_CONST AND NOT ICONV_NEEDS_C_NON_CONST)
		set(ICONV_CONST "const")
	elseif (NOT ICONV_NEEDS_C_CONST AND ICONV_NEEDS_C_NON_CONST)
		set(ICONV_CONST "")
	elseif (ICONV_NEEDS_C_CONST AND ICONV_NEEDS_C_NON_CONST)
		# Doesn't matter
		set(ICONV_CONST "")
	else()
		message(FATAL_ERROR "iconv.h: Take a look on iconv() declaration")
	endif()

	# Check if iconv() takes const char ** (C++)
	check_cxx_source_compiles("
		#include <iconv.h>
		int main(int argc, char ** argv)
		{ return iconv(0, (const char **)0, 0, (char**)0, 0); }
		" ICONV_NEEDS_CPP_CONST)
	# Check if iconv() takes char ** (C++)
	check_cxx_source_compiles("
		#include <iconv.h>
		int main(int argc, char ** argv)
		{ return iconv(0, (char **)0, 0, (char**)0, 0); }
		" ICONV_NEEDS_CPP_NON_CONST)
	if (ICONV_NEEDS_CPP_CONST AND NOT ICONV_NEEDS_CPP_NON_CONST)
		set(ICONV_CPP_CONST "const")
	elseif (NOT ICONV_NEEDS_CPP_CONST AND ICONV_NEEDS_CPP_NON_CONST)
		set(ICONV_CPP_CONST "")
	elseif (ICONV_NEEDS_CPP_CONST AND ICONV_NEEDS_CPP_NON_CONST)
		# Doesn't matter
		set(ICONV_CPP_CONST "")
	else()
		message(FATAL_ERROR "iconv.h: Take a look on iconv() declaration")
	endif()
endif()

check_type_size("int" SIZEOF_INT)

check_type_size("long" SIZEOF_LONG)

check_type_size("unsigned long" SIZEOF_UNSIGNED_LONG)

check_type_size("void*" SIZEOF_VOIDP)

check_include_files("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)

check_type_size("off_t" SIZEOF_OFF_T)
check_type_size("ftell(NULL)" SIZEOF_FTELL)

check_function_exists("fseek64" HAVE_FSEEK64)
check_function_exists("fseeko64" HAVE_FSEEKO64)
check_function_exists("fseeko" HAVE_FSEEKO)
if (HAVE_FSEEK64)
	set(VSI_FSEEK64 "fseek64")
elseif (HAVE_FSEEKO64)
	set(VSI_FSEEK64 "fseeko64")
elseif (SIZEOF_OFF_T EQUAL 8 AND HAVE_FSEEKO)
	set(VSI_FSEEK64 "fseeko")
elseif (SIZEOF_FTELL EQUAL 8)
	set(VSI_FSEEK64 "fseek")
endif()

check_function_exists("ftell64" HAVE_FTELL64)
check_function_exists("ftello64" HAVE_FTELLO64)
check_function_exists("ftello" HAVE_FTELLO)
if (HAVE_FTELL64)
	set(VSI_FTELL64 "ftell64")
elseif (HAVE_FTELLO64)
	set(VSI_FTELL64 "ftello64")
elseif (SIZEOF_OFF_T EQUAL 8 AND HAVE_FTELLO)
	set(VSI_FTELL64 "ftello")
elseif (SIZEOF_FTELL EQUAL 8)
	set(VSI_FTELL64 "ftell")
endif()

if (VSI_FSEEK64 AND VSI_FTELL64)
	set(UNIX_STDIO_64 ON)
endif()

check_function_exists("fopen64" HAVE_FOPEN64)
check_function_exists("fopen" HAVE_FOPEN)
if (HAVE_FOPEN64)
	set(VSI_FOPEN64 "fopen64")
elseif (HAVE_FOPEN)
	set(VSI_FOPEN64 "fopen")
endif()

check_function_exists("ftruncate64" HAVE_FTRUNCATE64)
check_function_exists("ftruncate" HAVE_FTRUNCATE)
if (HAVE_FTRUNCATE64)
	set(VSI_FTRUNCATE64 "ftruncate64")
elseif (HAVE_FTRUNCATE)
	set(VSI_FTRUNCATE64 "ftruncate")
endif()

set(VSI_NEED_LARGEFILE64_SOURCE ON)

check_function_exists("stat" HAVE_STAT)
check_function_exists("stat64" HAVE_STAT64)
check_function_exists("_stat64" HAVE__STAT64)
if (HAVE_STAT64)
	set(VSI_STAT64 "stat64")
elseif (HAVE__STAT64)
	set(VSI_STAT64 "_stat64")
elseif (SIZEOF_OFF_T EQUAL 8 AND HAVE_STAT)
	set(VSI_STAT64 "stat")
endif()

set(CMAKE_EXTRA_INCLUDE_FILES "sys/stat.h")
check_type_size("struct stat" SIZEOF_STAT)
check_type_size("struct stat64" SIZEOF_STAT64)
check_type_size("struct _stat64" SIZEOF__STAT64)
check_type_size("struct __stat64" SIZEOF___STAT64)
if (SIZEOF_STAT64)
	set(VSI_STAT64_T "stat64")
elseif (SIZEOF__STAT64)
	set(VSI_STAT64_T "_stat64")
elseif (SIZEOF___STAT64)
	set(VSI_STAT64_T "__stat64")
elseif (SIZEOF_OFF_T EQUAL 8 AND SIZEOF_STAT)
	set(VSI_STAT64_T "stat")
endif()
unset(CMAKE_EXTRA_INCLUDE_FILES)

if ((VSI_STAT64 AND NOT VSI_STAT64_T) OR (NOT VSI_STAT64 AND VSI_STAT64_T))
	unset(VSI_STAT64)
	unset(VSI_STAT64_T)
endif()

check_c_compiler_flag("-fvisibility" USE_GCC_VISIBILITY_FLAG)

check_function_exists("__atomic_is_lock_free" HAVE_GCC_ATOMIC_BUILTINS)

check_function_exists("__builtin_bswap16" HAVE_GCC_BSWAP)

test_big_endian(WORDS_BIGENDIAN)

check_function_exists("getaddrinfo" HAVE_GETADDRINFO)

check_symbol_exists("_SC_PHYS_PAGES" "unistd.h" HAVE_SC_PHYS_PAGES)

check_function_exists("uselocale" HAVE_USELOCALE)
check_symbol_exists("LC_NUMERIC_MASK" "locale.h" HAVE_LC_NUMERIC_MASK)
if (HAVE_USELOCALE AND NOT LC_NUMERIC_MASK)
	set(HAVE_USELOCALE OFF)
endif()

check_cxx_symbol_exists("std::isnan" "cmath" HAVE_STD_IS_NAN)

check_c_compiler_flag("-Wzero-as-null-pointer-constant" HAVE_GCC_WARNING_ZERO_AS_NULL_POINTER_CONSTANT)

configure_file("cpl_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cpl_config.h")
