project(gif_osmand)

add_definitions(-DHAVE_CONFIG_H)
if(CMAKE_TARGET_OS STREQUAL "windows")
	include_directories(AFTER SYSTEM
		"include"
	)
endif()
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()
add_library(gif_osmand STATIC
	"upstream.patched/lib/dgif_lib.c"
	"upstream.patched/lib/gifalloc.c"
	"upstream.patched/lib/gif_err.c"
)
