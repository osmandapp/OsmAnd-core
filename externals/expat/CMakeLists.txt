project(expat)

prepare_upstream()

if (CMAKE_TARGET_OS STREQUAL "ios" AND CMAKE_TARGET_BUILD_TOOL STREQUAL "xcode")
	configure_file("expat_config.h.ios" "${CMAKE_CURRENT_BINARY_DIR}/expat_config.h" COPYONLY)
else()
	save_and_set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_C_FLAGS}")

	# Modified content from 'ConfigureChecks.cmake':
	include(CheckIncludeFile)
	include(CheckIncludeFiles)
	include(CheckFunctionExists)
	include(CheckSymbolExists)
	include(TestBigEndian)

	check_include_file("dlfcn.h" HAVE_DLFCN_H)
	check_include_file("fcntl.h" HAVE_FCNTL_H)
	check_include_file("inttypes.h" HAVE_INTTYPES_H)
	check_include_file("memory.h" HAVE_MEMORY_H)
	check_include_file("stdint.h" HAVE_STDINT_H)
	check_include_file("stdlib.h" HAVE_STDLIB_H)
	check_include_file("strings.h" HAVE_STRINGS_H)
	check_include_file("string.h" HAVE_STRING_H)
	check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
	check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
	check_include_file("unistd.h" HAVE_UNISTD_H)

	check_function_exists("getpagesize" HAVE_GETPAGESIZE)
	check_function_exists("bcopy" HAVE_BCOPY)
	check_symbol_exists("memmove" "string.h" HAVE_MEMMOVE)
	check_function_exists("mmap" HAVE_MMAP)

	#/* Define to 1 if you have the ANSI C header files. */
	check_include_files("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)

	test_big_endian(WORDS_BIGENDIAN)
	#/* 1234 = LIL_ENDIAN, 4321 = BIGENDIAN */
	if (WORDS_BIGENDIAN)
		set(BYTEORDER 4321)
	else(WORDS_BIGENDIAN)
		set(BYTEORDER 1234)
	endif(WORDS_BIGENDIAN)

	if (HAVE_SYS_TYPES_H)
		check_symbol_exists("off_t" "sys/types.h" OFF_T)
		check_symbol_exists("size_t" "sys/types.h" SIZE_T)
	else(HAVE_SYS_TYPES_H)
		set(OFF_T "long")
		set(SIZE_T "unsigned")
	endif(HAVE_SYS_TYPES_H)
	# End of modified content from 'ConfigureChecks.cmake'

	restore(CMAKE_REQUIRED_FLAGS)

	configure_file("expat_config.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/expat_config.h")
endif()

add_library(expat_static STATIC
	"upstream.patched/lib/xmlparse.c"
	"upstream.patched/lib/xmlrole.c"
	"upstream.patched/lib/xmltok.c"
	"upstream.patched/lib/xmltok_impl.c"
	"upstream.patched/lib/xmltok_ns.c"
)

target_compile_definitions(expat_static
	PRIVATE
		-DHAVE_EXPAT_CONFIG_H
	PUBLIC
		-DXML_STATIC
)
target_include_directories(expat_static
	PRIVATE
		"${CMAKE_CURRENT_BINARY_DIR}"
	PUBLIC
		"upstream.patched/lib"
)
