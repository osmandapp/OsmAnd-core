if (CMAKE_TARGET_OS STREQUAL "linux" OR
	CMAKE_TARGET_OS STREQUAL "macosx" OR
	CMAKE_TARGET_OS STREQUAL "windows")
	add_subdirectory("${OSMAND_ROOT}/core/tools" "core/tools")
endif()
