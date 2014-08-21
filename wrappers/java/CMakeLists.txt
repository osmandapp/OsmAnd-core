project(OsmAndCoreJNI)

find_package(Java REQUIRED)
find_package(JNI REQUIRED)

set(target_specific_private_includes "")
set(target_specific_public_libraries "")

file(GLOB_RECURSE core_includes "${OSMAND_ROOT}/core/include/*.h*")
file(GLOB core_swig "${OSMAND_ROOT}/core/*.swig")
file(GLOB_RECURSE core_swig_subfiles "${OSMAND_ROOT}/core/swig/*")

set(OsmAndCoreJNI_generated_cpp
	"${CMAKE_CURRENT_LIST_DIR}/gen/cpp/swig.cpp"
)
if (CMAKE_HOST_WIN32 AND NOT CYGWIN)
	add_custom_command(OUTPUT ${OsmAndCoreJNI_generated_cpp}
		COMMAND bash ARGS --login "${CMAKE_CURRENT_LIST_DIR}/generate.sh"
		DEPENDS
			"${CMAKE_CURRENT_LIST_DIR}/generate.sh"
			${core_includes}
			${core_swig}
			${core_swig_subfiles}
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		COMMENT "Generating sources...")
else()
	add_custom_command(OUTPUT ${OsmAndCoreJNI_generated_cpp}
		COMMAND "${CMAKE_CURRENT_LIST_DIR}/generate.sh"
		DEPENDS
			"${CMAKE_CURRENT_LIST_DIR}/generate.sh"
			${core_includes}
			${core_swig}
			${core_swig_subfiles}
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		COMMENT "Generating sources...")
endif()

add_library(OsmAndCoreJNI SHARED
	${OsmAndCoreJNI_generated_cpp}
)
target_compile_definitions(OsmAndCoreJNI
	PRIVATE
		-DOSMAND_JNI_EXPORTS
		-DOSMAND_SWIG
)
target_include_directories(OsmAndCoreJNI
	PRIVATE
		${JNI_INCLUDE_DIRS}
		${JAVA_INCLUDE_PATH}
		${target_specific_private_includes}
)

if (CMAKE_STATIC_LIBS_ALLOWED_ON_TARGET)
	add_dependencies(OsmAndCoreJNI
		OsmAndCore_static
		OsmAndCoreTools_static
	)
	target_link_libraries(OsmAndCoreJNI
		LINK_PUBLIC
			OsmAndCore_static
			OsmAndCoreTools_static
			${target_specific_public_libraries}
	)
else()
	add_dependencies(OsmAndCoreJNI
		OsmAndCore_shared
		OsmAndCoreTools_shared
	)
	target_link_libraries(OsmAndCoreJNI
		LINK_PUBLIC
			OsmAndCore_shared
			OsmAndCoreTools_shared
			${target_specific_public_libraries}
	)
endif()

file(GLOB_RECURSE OsmAndCoreJNI_generated_java "${CMAKE_CURRENT_LIST_DIR}/gen/java/*")
include(UseJava)
set(CMAKE_JAVA_TARGET_VERSION 1.6.0)
add_jar(OsmAndCore_java
	${OsmAndCoreJNI_generated_java}
)
add_dependencies(OsmAndCore_java
	OsmAndCoreJNI
)
