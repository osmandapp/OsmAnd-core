project(glew)

add_library(glew_static STATIC
	"upstream.patched/include/GL/glew.h"
	"upstream.patched/src/glew.c"
)

find_package(OpenGL REQUIRED)

target_compile_definitions(glew_static
	PRIVATE
		-DGLEW_BUILD
	PUBLIC
		-DGLEW_STATIC
		${OPENGL_DEFINITIONS}
)
target_include_directories(glew_static
	PUBLIC
		${OPENGL_INCLUDE_DIRS}
		"upstream.patched/include"
)
target_link_libraries(glew_static
	LINK_PUBLIC
		${OPENGL_LIBRARIES}
)