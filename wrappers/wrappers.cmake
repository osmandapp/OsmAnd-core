# Java
if(CMAKE_SHARED_LIBS_ALLOWED_ON_TARGET AND (CMAKE_TARGET_OS STREQUAL "linux" OR CMAKE_TARGET_OS STREQUAL "darwin" OR CMAKE_TARGET_OS STREQUAL "windows" OR CMAKE_TARGET_OS STREQUAL "android"))
	# JNI wrapper is only valid on platforms where Java exists
	add_subdirectory("${OSMAND_ROOT}/core/wrappers/java" "core/wrappers/java")
endif()
