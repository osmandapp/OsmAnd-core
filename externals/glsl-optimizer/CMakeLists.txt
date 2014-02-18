project(glsl_optimizer)

file(GLOB libglcpp_sources "upstream.patched/src/glsl/glcpp/*.c")
file(GLOB_RECURSE mesa_sources "upstream.patched/src/mesa/*.c")
file(GLOB glsl_sources "upstream.patched/src/glsl/*.c*")

file(GLOB glsl_source_remove "upstream.patched/src/glsl/main.cpp")
list(REMOVE_ITEM glsl_sources ${glsl_source_remove})

add_library(glsl-optimizer_static STATIC
	${libglcpp_sources}
	${mesa_sources}
	${glsl_sources}
)

set(target_specific_private_definitions "")
set(target_specific_private_includes "")
if(CMAKE_TARGET_OS STREQUAL "windows")
	if(CMAKE_COMPILER_FAMILY STREQUAL "msvc")
		set(target_specific_private_includes ${target_specific_private_includes}
			"include"
		)

		set(target_specific_private_definitions ${target_specific_private_definitions}
			-D_CRT_SECURE_NO_WARNINGS
			-D__STDC_VERSION__=200000L
			-D_CRT_NONSTDC_NO_WARNINGS
		)

		# Disable "no matching operator delete found; memory will not be freed if initialization throws an exception
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4291")
	endif()
endif()

if(target_specific_private_definitions)
	target_compile_definitions(glsl-optimizer_static
		PRIVATE
			${target_specific_private_definitions}
	)
endif()

target_include_directories(glsl-optimizer_static
	PRIVATE
		"upstream.patched/src/mesa"
		"upstream.patched/src/mapi"
		"upstream.patched/src/glsl"
		"upstream.patched/include"
		${target_specific_private_includes}
	PUBLIC
		"public_include"
)
