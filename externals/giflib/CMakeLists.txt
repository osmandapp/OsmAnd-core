project(gif)

add_library(gif_static STATIC
	"upstream.patched/lib/dgif_lib.c"
	"upstream.patched/lib/gifalloc.c"
	"upstream.patched/lib/gif_err.c"
	"upstream.patched/lib/gif_lib.h"
	"upstream.patched/lib/gif_lib_private.h"
	"upstream.patched/lib/gif_hash.h"
)

set(target_specific_private_definitions "")
if(CMAKE_TARGET_OS STREQUAL "windows")
	if(MSVC)
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
