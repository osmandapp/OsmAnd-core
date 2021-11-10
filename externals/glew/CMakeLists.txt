project(glew)

prepare_upstream()

file(GLOB_RECURSE glew_headers "upstream.patched/include/GL/*.h*")
add_library(glew_static STATIC
	${glew_headers}
	"upstream.patched/src/glew.c"
)

set(target_specific_public_definitions "")
set(target_specific_private_definitions "")
set(target_specific_public_includes "")
set(target_specific_public_libraries "")

find_package(OpenGL REQUIRED)

if (CMAKE_TARGET_OS STREQUAL "linux" OR CMAKE_TARGET_OS STREQUAL "macosx")
	find_package(X11 REQUIRED)
	set(target_specific_public_includes ${target_specific_public_includes}
		${X11_INCLUDE_DIR}
	)
	find_library(X11_Xrender "Xrender" PATHS ${X11_LIBRARY_DIR})
	set(target_specific_public_libraries ${target_specific_public_libraries}
		${X11_LIBRARIES}
		${X11_Xrender}
	)
endif()

target_compile_definitions(glew_static
	PRIVATE
		-DGLEW_BUILD
		${target_specific_private_definitions}
	PUBLIC
		-DGLEW_STATIC
		${target_specific_public_definitions}
)
target_include_directories(glew_static
	PUBLIC
		"upstream.patched/include"
		${target_specific_public_includes}
)
target_link_libraries(glew_static
	LINK_PUBLIC
		OpenGL::GL
		OpenGL::GLU
		${target_specific_public_libraries}
)
if (CMAKE_TARGET_OS STREQUAL "ios" AND CMAKE_TARGET_BUILD_TOOL STREQUAL "xcode")
    set_target_properties(glew_static
        PROPERTIES
            XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES")
endif()
