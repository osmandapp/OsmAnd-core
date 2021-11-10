project(gif)

prepare_upstream()

if (CMAKE_TARGET_OS STREQUAL "ios" AND CMAKE_TARGET_BUILD_TOOL STREQUAL "xcode")
	configure_file("config.h.ios" "${CMAKE_CURRENT_BINARY_DIR}/config.h" COPYONLY)
else()
	save_and_set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_C_FLAGS}")
	include(CheckIncludeFile)
	include(CheckIncludeFiles)
	include(CheckFunctionExists)
	include(CheckSymbolExists)
	include(CheckLibraryExists)
	include(CheckTypeSize)
	include(TestBigEndian)

	check_include_file("dlfcn.h" HAVE_DLFCN_H)
	check_include_file("fcntl.h" HAVE_FCNTL_H)
	check_include_file("inttypes.h" HAVE_INTTYPES_H)
	unset(HAVE_LIBX11)
	check_include_file("limits.h" HAVE_LIMITS_H)
	check_include_file("memory.h" HAVE_MEMORY_H)
	check_include_file("stdarg.h" HAVE_STDARG_H)
	check_include_file("stdint.h" HAVE_STDINT_H)
	check_include_file("stdlib.h" HAVE_STDLIB_H)
	check_include_file("strings.h" HAVE_STRINGS_H)
	check_include_file("string.h" HAVE_STRING_H)
	check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
	check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
	check_include_file("unistd.h" HAVE_UNISTD_H)
	check_include_file("X11/Xlib.h" HAVE_X11_XLIB_H)
	check_include_file("X11/Xutil.h" HAVE_X11_XUTIL_H)
	unset(LT_OBJDIR)
	unset(PACKAGE)
	unset(PACKAGE_BUGREPORT)
	unset(PACKAGE_NAME)
	unset(PACKAGE_STRING)
	unset(PACKAGE_TARNAME)
	unset(PACKAGE_URL)
	unset(PACKAGE_VERSION)
	check_include_files("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)
	unset(VERSION)
	set(X_DISPLAY_MISSING ON)

	restore(CMAKE_REQUIRED_FLAGS)

	configure_file("config.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config.h")
endif()

add_library(gif_static STATIC
	"upstream.patched/lib/dgif_lib.c"
	#"upstream.patched/lib/egif_lib.c"
	"upstream.patched/lib/gif_font.c"
	"upstream.patched/lib/gif_hash.c"
	"upstream.patched/lib/gif_hash.h"
	"upstream.patched/lib/gifalloc.c"
	"upstream.patched/lib/gif_err.c"
	"upstream.patched/lib/gif_lib_private.h"
)

set(target_specific_private_definitions "")
if (CMAKE_TARGET_OS STREQUAL "windows")
	if (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
		set(target_specific_private_definitions ${target_specific_private_definitions}
			-D_CRT_SECURE_NO_WARNINGS
			-D_CRT_NONSTDC_NO_WARNINGS
		)
	endif()

	set(target_specific_private_definitions ${target_specific_private_definitions}
		-DWINDOWS32
	)
endif()

target_compile_definitions(gif_static
	PRIVATE
		-DHAVE_CONFIG_H
		${target_specific_private_definitions}
)

target_include_directories(gif_static
	PRIVATE
		"${CMAKE_CURRENT_BINARY_DIR}"
	PUBLIC
		"upstream.patched/lib"
)
