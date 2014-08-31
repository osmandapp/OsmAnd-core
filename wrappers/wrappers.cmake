# add_generate_swig_target: macro that adds a special target that calls generate.sh
macro(add_generate_swig_target TARGET_NAME)
	if (CMAKE_HOST_WIN32 AND NOT CYGWIN)
		add_custom_target(${TARGET_NAME} bash --login "${CMAKE_CURRENT_LIST_DIR}/generate.sh" "${CMAKE_CURRENT_BINARY_DIR}"
			DEPENDS
				"${CMAKE_CURRENT_LIST_DIR}/generate.sh"
				"${ARGN}"
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating wrapper sources in '${CMAKE_CURRENT_BINARY_DIR}' using '${CMAKE_CURRENT_LIST_DIR}/generate.sh'...")
	else()
		add_custom_target(${TARGET_NAME} "${CMAKE_CURRENT_LIST_DIR}/generate.sh" "${CMAKE_CURRENT_BINARY_DIR}"
			DEPENDS
				"${CMAKE_CURRENT_LIST_DIR}/generate.sh"
				"${ARGN}"
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating wrapper sources in '${CMAKE_CURRENT_BINARY_DIR}' using '${CMAKE_CURRENT_LIST_DIR}/generate.sh'...")
	endif()
endmacro()

# Java
if (CMAKE_SHARED_LIBS_ALLOWED_ON_TARGET AND	(
	CMAKE_TARGET_OS STREQUAL "linux" OR
	CMAKE_TARGET_OS STREQUAL "macosx" OR
	CMAKE_TARGET_OS STREQUAL "windows"
	OR CMAKE_TARGET_OS STREQUAL "android"))
	# JNI wrapper is only valid on platforms where Java exists
	add_subdirectory("${OSMAND_ROOT}/core/wrappers/java" "core/wrappers/java")
endif()
