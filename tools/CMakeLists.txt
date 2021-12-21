project(OsmAndCoreTools)

# Bump this number each time a new source file is committed to repository, source file removed from repository or renamed: 5

set(target_specific_sources "")
set(target_specific_public_definitions "")
set(target_specific_public_includes "")
set(target_specific_public_libraries "")
set(target_specific_private_definitions "")
set(target_specific_dependencies "")

file(GLOB_RECURSE sources "src/*.c*")
file(GLOB_RECURSE headers "include/*.h*")

set(merged_sources
	${sources}
	${headers}
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

if (CMAKE_TARGET_OS STREQUAL "linux" OR
	CMAKE_TARGET_OS STREQUAL "macosx" OR
	CMAKE_TARGET_OS STREQUAL "windows")
	find_package(OpenGL REQUIRED)
	set(target_specific_public_libraries ${target_specific_public_libraries}
		OpenGL::GL
		OpenGL::GLU
		glew_static
	)
	set(target_specific_dependencies ${target_specific_dependencies}
		OpenGL::GL
		OpenGL::GLU
		glew_static
	)
endif()

if (CMAKE_SHARED_LIBS_ALLOWED_ON_TARGET)
	add_library(OsmAndCoreTools_shared SHARED ${merged_sources})
	add_dependencies(OsmAndCoreTools_shared
		OsmAndCore_shared
		${target_specific_dependencies})
	target_compile_definitions(OsmAndCoreTools_shared
		PRIVATE
			-DOSMAND_CORE_TOOLS_EXPORTS
			${target_specific_public_definitions}
	)
	target_include_directories(OsmAndCoreTools_shared
		PRIVATE
			"${OSMAND_ROOT}/core/tools/include/OsmAndCoreTools"
		PUBLIC
			"${OSMAND_ROOT}/core/tools/include"
			${target_specific_public_includes}
	)
	target_link_libraries(OsmAndCoreTools_shared
		LINK_PUBLIC
			OsmAndCore_shared
			${target_specific_public_libraries}
	)

	if (TARGET OsmAndCore_shared_standalone)
		add_library(OsmAndCoreTools_shared_standalone SHARED ${merged_sources})
		add_dependencies(OsmAndCoreTools_shared_standalone
			OsmAndCore_shared_standalone
			${target_specific_dependencies})
		target_compile_definitions(OsmAndCoreTools_shared_standalone
			PRIVATE
				-DOSMAND_CORE_TOOLS_EXPORTS
				${target_specific_public_definitions}
		)
		target_include_directories(OsmAndCoreTools_shared_standalone
			PRIVATE
				"${OSMAND_ROOT}/core/tools/include/OsmAndCoreTools"
			PUBLIC
				"${OSMAND_ROOT}/core/tools/include"
				${target_specific_public_includes}
		)
		target_link_libraries(OsmAndCoreTools_shared_standalone
			LINK_PUBLIC
				OsmAndCore_shared_standalone
				${target_specific_public_libraries}
		)
	endif()
endif()

if (CMAKE_STATIC_LIBS_ALLOWED_ON_TARGET)
	add_library(OsmAndCoreTools_static STATIC ${merged_sources})
	add_dependencies(OsmAndCoreTools_static
		OsmAndCore_static
		${target_specific_dependencies})
	target_compile_definitions(OsmAndCoreTools_static
		PUBLIC
			-DOSMAND_CORE_TOOLS_STATIC
			${target_specific_public_definitions}
	)
	target_include_directories(OsmAndCoreTools_static
		PRIVATE
			"${OSMAND_ROOT}/core/tools/include/OsmAndCoreTools"
		PUBLIC
			"${OSMAND_ROOT}/core/tools/include"
			${target_specific_public_includes}
	)
	target_link_libraries(OsmAndCoreTools_static
		LINK_PUBLIC
			OsmAndCore_static
			${target_specific_public_libraries}
	)

	if (TARGET OsmAndCore_static_standalone)
		add_library(OsmAndCoreTools_static_standalone STATIC ${merged_sources})
		add_dependencies(OsmAndCoreTools_static_standalone
			OsmAndCore_static_standalone
			${target_specific_dependencies})
		target_compile_definitions(OsmAndCoreTools_static_standalone
			PUBLIC
				-DOSMAND_CORE_TOOLS_STATIC
				${target_specific_public_definitions}
		)
		target_include_directories(OsmAndCoreTools_static_standalone
			PRIVATE
				"${OSMAND_ROOT}/core/tools/include/OsmAndCoreTools"
			PUBLIC
				"${OSMAND_ROOT}/core/tools/include"
				${target_specific_public_includes}
		)
		target_link_libraries(OsmAndCoreTools_static_standalone
			LINK_PUBLIC
				OsmAndCore_static_standalone
				${target_specific_public_libraries}
		)
	endif()
endif()
