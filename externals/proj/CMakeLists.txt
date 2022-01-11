project(proj)

prepare_upstream()

include("proj_config.cmake")

# Target specific
set(target_specific_private_definitions "")
set(target_specific_libraries_public "")
if (CMAKE_TARGET_OS STREQUAL "windows")
	set(target_specific_private_definitions "${target_specific_private_definitions}"
		-DNOMINMAX
	)
else()
	find_library(PTHREAD_LIBRARY pthread)
	if (PTHREAD_LIBRARY)
		set(target_specific_libraries_public ${target_specific_libraries_public}
			${PTHREAD_LIBRARY}
		)
	endif()
endif()

file(GLOB src_core "upstream.patched/src/*.c*")
file(GLOB src_projections "upstream.patched/src/projections/*.cpp")
file(GLOB src_conversions "upstream.patched/src/conversions/*.cpp")
file(GLOB src_transformations "upstream.patched/src/transformations/*.cpp")
file(GLOB src_iso19111 "upstream.patched/src/iso19111/*.cpp")
file(GLOB src_iso19111_operation "upstream.patched/src/iso19111/operation/*.cpp")

add_library(proj_static STATIC
	${src_core}
	${src_projections}
	${src_conversions}
	${src_transformations}
	${src_iso19111}
	${src_iso19111_operation}
)
target_compile_definitions(proj_static
	PRIVATE
		${target_specific_private_definitions}
)
target_include_directories(proj_static
	PUBLIC
		"${CMAKE_CURRENT_BINARY_DIR}"
		"upstream.patched/include"
		"upstream.patched/src"
)
target_link_libraries(proj_static
	LINK_PUBLIC
		sqlite_static
		${target_specific_libraries_public}
)
