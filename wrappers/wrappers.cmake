# add_generate_swig_target: macro that adds a special target that calls generate.sh
macro(add_generate_swig_target TARGET_NAME)
	if (CMAKE_HOST_WIN32 AND NOT CYGWIN)
		add_custom_target(${TARGET_NAME} bash --login "${CMAKE_CURRENT_LIST_DIR}/generate.sh" "${CMAKE_CURRENT_BINARY_DIR}"
			DEPENDS
				"${CMAKE_CURRENT_LIST_DIR}/generate.sh"
				${ARGN}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating wrapper sources in '${CMAKE_CURRENT_BINARY_DIR}' using '${CMAKE_CURRENT_LIST_DIR}/generate.sh'...")
	else()
		add_custom_target(${TARGET_NAME} "${CMAKE_CURRENT_LIST_DIR}/generate.sh" "${CMAKE_CURRENT_BINARY_DIR}"
			DEPENDS
				"${CMAKE_CURRENT_LIST_DIR}/generate.sh"
				${ARGN}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating wrapper sources in '${CMAKE_CURRENT_BINARY_DIR}' using '${CMAKE_CURRENT_LIST_DIR}/generate.sh'...")
	endif()
endmacro()

# Java
set(OSMAND_JAVA_AVAILABLE OFF)
if (CMAKE_TARGET_OS STREQUAL "android")
	set(OSMAND_JAVA_AVAILABLE ON)
else()
	find_package(Java)
	find_package(JNI)
	
	if (Java_FOUND AND JNI_FOUND)
		set(OSMAND_JAVA_AVAILABLE ON)
	endif ()
endif()
if (OSMAND_JAVA_AVAILABLE)
	set(CMAKE_JAVA_TARGET_OUTPUT_DIR "${OSMAND_OUTPUT_ROOT}")
	add_subdirectory("${OSMAND_ROOT}/core/wrappers/java" "core/wrappers/java")
	if (NOT CMAKE_TARGET_OS STREQUAL "android")
		add_subdirectory("${OSMAND_ROOT}/core/samples/java" "core/samples/java")
	endif()
endif()
