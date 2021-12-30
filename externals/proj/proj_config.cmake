include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckFunctionExists)

if (CMAKE_TARGET_OS STREQUAL "windows")
	set(MUTEX_win32 ON)
else()
	set(MUTEX_pthread ON)
	check_symbol_exists("PTHREAD_MUTEX_RECURSIVE" "pthread.h" HAVE_PTHREAD_MUTEX_RECURSIVE)
endif()

check_function_exists("localeconv" HAVE_LOCALECONV)

check_function_exists("strerror" HAVE_STRERROR)

configure_file("proj_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/proj_config.h")
