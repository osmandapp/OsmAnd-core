project(gif)

add_library(gif_static STATIC
	"upstream.patched/lib/dgif_lib.c"
	"upstream.patched/lib/gifalloc.c"
	"upstream.patched/lib/gif_err.c"
	"upstream.patched/lib/gif_lib.h"
	"upstream.patched/lib/gif_lib_private.h"
	"upstream.patched/lib/gif_hash.h"
)

target_compile_definitions(gif_static
	PRIVATE
		-DHAVE_CONFIG_H
)
if(CMAKE_TARGET_OS STREQUAL "windows")
	target_include_directories(gif_static
		PUBLIC
			"include"
	)
endif()
target_include_directories(gif_static
	PUBLIC
		"upstream.patched/lib"
)
