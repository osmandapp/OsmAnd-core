project(OsmAndCoreTools)

# Bump this number each time a new source file is committed to repository, source file removed from repository or renamed: 1

file(GLOB_RECURSE sources "*.c*")
file(GLOB_RECURSE headers "*.h*")

set(merged_sources
	${sources}
	${headers}
)
set(merged_include_dirs_public
	"${OSMAND_ROOT}/core/tools"
)

if (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
	# Set warning level 4
	set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} /W4")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

	# Force warning to be an error "deletion of pointer to incomplete type 'type'; no destructor called"
	set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} /we4150")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /we4150")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /we4150")

	# Disable following warnings:
	#  - C4290: 'C++ exception specification ignored except to indicate a function is not __declspec(nothrow)'
	#  - C4201: 'nonstandard extension used : nameless struct/union'
	set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} /wd4290 /wd4201")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4290 /wd4201")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4290 /wd4201")
elseif (CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Wall")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

if (CMAKE_SHARED_LIBS_ALLOWED_ON_TARGET)
	add_library(OsmAndCoreTools_shared SHARED ${merged_sources})
	add_dependencies(OsmAndCoreTools_shared OsmAndCore_shared)
	target_compile_definitions(OsmAndCoreTools_shared
		PRIVATE
			-DOSMAND_CORE_TOOLS_EXPORTS
	)
	target_include_directories(OsmAndCoreTools_shared
		PUBLIC
			${merged_include_dirs_public}
	)
	target_link_libraries(OsmAndCoreTools_shared
		LINK_PUBLIC
			OsmAndCore_shared
	)
endif()

if (CMAKE_STATIC_LIBS_ALLOWED_ON_TARGET)
	add_library(OsmAndCoreTools_static STATIC ${merged_sources})
	add_dependencies(OsmAndCoreTools_static OsmAndCore_static)
	target_compile_definitions(OsmAndCoreTools_static
		PUBLIC
			-DOSMAND_CORE_TOOLS_STATIC
	)
	target_include_directories(OsmAndCoreTools_static
		PUBLIC
			${merged_include_dirs_public}
	)
	target_link_libraries(OsmAndCoreTools_static
		LINK_PUBLIC
			OsmAndCore_static
	)
endif()
