project(OsmAndCore_JavaWrapper)

# Custom target to generate files with SWIG
file(GLOB core_swig "${OSMAND_ROOT}/core/*.swig")
file(GLOB_RECURSE core_swig_subfiles "${OSMAND_ROOT}/core/swig/*")

get_filename_component(CMAKE_ABS_CURRENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} ABSOLUTE)

add_generate_swig_target(OsmAndCore_swig_java
	${core_swig}
	${core_swig_subfiles})
set(swig_generated_cpp
	"${CMAKE_ABS_CURRENT_BINARY_DIR}/gen/cpp/swig.cpp"
)
set_source_files_properties(${swig_generated_cpp} GENERATED)
set(swig_generated_java
	"@${CMAKE_ABS_CURRENT_BINARY_DIR}/gen/java.list"
)
set_source_files_properties(${swig_generated_java} GENERATED)

# JAR with all generated Java classes:
add_custom_target(OsmAndCore_wrapper_jar
	"${Java_JAR_EXECUTABLE}"
		-cf "OsmAndCore_wrapper.jar"
		-C "${CMAKE_ABS_CURRENT_BINARY_DIR}/gen/java"
		.
	WORKING_DIRECTORY "${CMAKE_JAVA_TARGET_OUTPUT_DIR}"
	COMMENT "Packing 'OsmAndCore_wrapper.jar'...")
add_dependencies(OsmAndCore_wrapper_jar
	OsmAndCore_swig_java
)

# Native library:
set(target_specific_private_includes "")
set(target_specific_public_libraries "")

# OsmAndCore JNI-only library
add_library(OsmAndCoreJNI SHARED
	${swig_generated_cpp}
)
target_compile_definitions(OsmAndCoreJNI
	PRIVATE
		-DOSMAND_JNI_EXPORTS
		-DOSMAND_SWIG
)
if (CMAKE_TARGET_OS STREQUAL "android")
	target_include_directories(OsmAndCoreJNI
		PRIVATE
			"${OSMAND_ROOT}/core/swig"
			${target_specific_private_includes}
	)
else()
	target_include_directories(OsmAndCoreJNI
		PRIVATE
			${JNI_INCLUDE_DIRS}
			${JAVA_INCLUDE_PATH}
			"${OSMAND_ROOT}/core/swig"
			${target_specific_private_includes}
	)
endif()
add_dependencies(OsmAndCoreJNI
	OsmAndCore_shared
	OsmAndCore_swig_java
)
target_link_libraries(OsmAndCoreJNI
	LINK_PUBLIC
		OsmAndCore_shared
		${target_specific_public_libraries}
)

if (TARGET OsmAndCore_shared_standalone)
	add_library(OsmAndCoreJNI_standalone SHARED
		${swig_generated_cpp}
	)
	target_compile_definitions(OsmAndCoreJNI_standalone
		PRIVATE
			-DOSMAND_JNI_EXPORTS
			-DOSMAND_SWIG
	)
	if (CMAKE_TARGET_OS STREQUAL "android")
		target_include_directories(OsmAndCoreJNI_standalone
			PRIVATE
				"${OSMAND_ROOT}/core/swig"
				${target_specific_private_includes}
		)
	else()
		target_include_directories(OsmAndCoreJNI_standalone
			PRIVATE
				${JNI_INCLUDE_DIRS}
				${JAVA_INCLUDE_PATH}
				"${OSMAND_ROOT}/core/swig"
				${target_specific_private_includes}
		)
	endif()
	add_dependencies(OsmAndCoreJNI_standalone
		OsmAndCore_shared_standalone
		OsmAndCore_swig_java
	)
	target_link_libraries(OsmAndCoreJNI_standalone
		LINK_PUBLIC
			OsmAndCore_shared_standalone
			${target_specific_public_libraries}
	)
endif()

# OsmAndCore+JNI library
add_library(OsmAndCoreWithJNI SHARED
	${swig_generated_cpp}
)
target_compile_definitions(OsmAndCoreWithJNI
	PRIVATE
		-DOSMAND_JNI_EXPORTS
		-DOSMAND_SWIG
)
if (CMAKE_TARGET_OS STREQUAL "android")
	target_include_directories(OsmAndCoreWithJNI
		PRIVATE
			"${OSMAND_ROOT}/core/swig"
			${target_specific_private_includes}
	)
else()
	target_include_directories(OsmAndCoreWithJNI
		PRIVATE
			"${OSMAND_ROOT}/core/swig"
			${JNI_INCLUDE_DIRS}
			${JAVA_INCLUDE_PATH}
			${target_specific_private_includes}
	)
endif()
add_dependencies(OsmAndCoreWithJNI
	OsmAndCore_static
	OsmAndCore_swig_java
)
target_link_libraries(OsmAndCoreWithJNI
	LINK_PUBLIC
		OsmAndCore_static
		${target_specific_public_libraries}
)

if (TARGET OsmAndCore_static_standalone)
	# OsmAndCore+JNI (standalone) library
	add_library(OsmAndCoreWithJNI_standalone SHARED
		${swig_generated_cpp}
	)
	target_compile_definitions(OsmAndCoreWithJNI_standalone
		PRIVATE
			-DOSMAND_JNI_EXPORTS
			-DOSMAND_SWIG
	)
	if (CMAKE_TARGET_OS STREQUAL "android")
		target_include_directories(OsmAndCoreWithJNI_standalone
			PRIVATE
				"${OSMAND_ROOT}/core/swig"
				${target_specific_private_includes}
		)
	else()
		target_include_directories(OsmAndCoreWithJNI_standalone
			PRIVATE
				"${OSMAND_ROOT}/core/swig"
				${JNI_INCLUDE_DIRS}
				${JAVA_INCLUDE_PATH}
				${target_specific_private_includes}
		)
	endif()
	add_dependencies(OsmAndCoreWithJNI_standalone
		OsmAndCore_static_standalone
		OsmAndCore_swig_java
	)
	target_link_libraries(OsmAndCoreWithJNI_standalone
		LINK_PUBLIC
			OsmAndCore_static_standalone
			${target_specific_public_libraries}
	)
endif()
