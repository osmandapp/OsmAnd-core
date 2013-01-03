project(gif)

set(ROOT "${OSMAND_ROOT}/externals/giflib")
set(UPSTREAM "${ROOT}/upstream.patched")
add_definitions(-DHAVE_CONFIG_H)
if(CMAKE_TARGET_OS STREQUAL "windows")
	include_directories(AFTER SYSTEM
		"${ROOT}/include"
	)
endif()
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()
add_library(gif STATIC
    "${UPSTREAM}/lib/dgif_lib.c"
    "${UPSTREAM}/lib/gifalloc.c"
    "${UPSTREAM}/lib/gif_err.c"
)