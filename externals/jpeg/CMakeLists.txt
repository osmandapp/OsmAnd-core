project(jpeg)

prepare_upstream()

add_library(jpeg_static STATIC
	"upstream.patched/jcapimin.c"
	"upstream.patched/jcapistd.c"
	"upstream.patched/jccoefct.c"
	"upstream.patched/jccolor.c"
	"upstream.patched/jcdctmgr.c"
	"upstream.patched/jchuff.c"
	"upstream.patched/jcicc.c"
	"upstream.patched/jcinit.c"
	"upstream.patched/jcmainct.c"
	"upstream.patched/jcmarker.c"
	"upstream.patched/jcmaster.c"
	"upstream.patched/jcomapi.c"
	"upstream.patched/jcparam.c"
	"upstream.patched/jcphuff.c"
	"upstream.patched/jcprepct.c"
	"upstream.patched/jcsample.c"
	"upstream.patched/jctrans.c"
	"upstream.patched/jdapimin.c"
	"upstream.patched/jdapistd.c"
	"upstream.patched/jdatadst.c"
	"upstream.patched/jdatasrc.c"
	"upstream.patched/jdcoefct.c"
	"upstream.patched/jdcolor.c"
	"upstream.patched/jddctmgr.c"
	"upstream.patched/jdhuff.c"
	"upstream.patched/jdicc.c"
	"upstream.patched/jdinput.c"
	"upstream.patched/jdmainct.c"
	"upstream.patched/jdmarker.c"
	"upstream.patched/jdmaster.c"
	"upstream.patched/jdmerge.c"
	"upstream.patched/jdphuff.c"
	"upstream.patched/jdpostct.c"
	"upstream.patched/jdsample.c"
	"upstream.patched/jdtrans.c"
	"upstream.patched/jerror.c"
	"upstream.patched/jfdctflt.c"
	"upstream.patched/jfdctfst.c"
	"upstream.patched/jfdctint.c"
	"upstream.patched/jidctflt.c"
	"upstream.patched/jidctfst.c"
	"upstream.patched/jidctint.c"
	"upstream.patched/jidctred.c"
	"upstream.patched/jquant1.c"
	"upstream.patched/jquant2.c"
	"upstream.patched/jutils.c"
	"upstream.patched/jmemmgr.c"
	"upstream.patched/jmemnobs.c"

	"upstream.patched/jsimd_none.c"

	"upstream.patched/jaricom.c"
	"upstream.patched/jcarith.c"
	"upstream.patched/jdarith.c"
)

set(target_specific_private_definitions "")
if (CMAKE_TARGET_OS STREQUAL "windows")
	if (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
		set(target_specific_private_definitions ${target_specific_private_definitions}
			-D_CRT_SECURE_NO_WARNINGS
		)
	endif()
endif()

target_compile_definitions(jpeg_static
	PRIVATE
		-DNO_GETENV
		${target_specific_private_definitions}
)
target_include_directories(jpeg_static
	PUBLIC
		"${CMAKE_CURRENT_BINARY_DIR}"
		"upstream.patched"
)

set(LIBJPEG_TURBO_VERSION_NUMBER 212)
set(JPEG_LIB_VERSION 60)
set(BITS_IN_JSAMPLE 8)
set(C_ARITH_CODING_SUPPORTED 1)
set(D_ARITH_CODING_SUPPORTED 1)
set(MEM_SRCDST_SUPPORTED 1)

include(CheckCSourceCompiles)
include(CheckIncludeFiles)
include(CheckTypeSize)
include(CheckSymbolExists)

check_type_size("size_t" SIZE_T)
check_type_size("unsigned long" UNSIGNED_LONG)
check_c_source_compiles("int main(int argc, char **argv) { unsigned long a = argc;  return __builtin_ctzl(a); }"
	HAVE_BUILTIN_CTZL)
check_include_files("intrin.h" HAVE_INTRIN_H)
check_include_files(locale.h HAVE_LOCALE_H)
check_include_files(stddef.h HAVE_STDDEF_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(sys/types.h NEED_SYS_TYPES_H)
check_symbol_exists(memset string.h HAVE_MEMSET)
check_symbol_exists(memcpy string.h HAVE_MEMCPY)
if(NOT HAVE_MEMSET AND NOT HAVE_MEMCPY)
	set(NEED_BSD_STRINGS 1)
endif()
check_type_size("unsigned char" UNSIGNED_CHAR)
check_type_size("unsigned short" UNSIGNED_SHORT)

configure_file("upstream.patched/jconfigint.h.in" jconfigint.h)
configure_file("upstream.patched/jconfig.h.in" jconfig.h)
