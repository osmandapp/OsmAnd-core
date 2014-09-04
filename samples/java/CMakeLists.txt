# External : z
add_subdirectory("${OSMAND_ROOT}/core/samples/java/externals/jogl" "core/samples/java/externals/jogl")

project(OsmAndCore_JavaSamples)

find_package(Java REQUIRED)
include(UseJava)

add_jar(OsmAndCore_JavaSample1
        SOURCES
			"src/sample1.java"
        INCLUDE_JARS
			"${CMAKE_JAVA_TARGET_OUTPUT_DIR}/OsmAndCore_wrapper.jar"
			${JOGL_JAR_LIBRARIES}
			${JOGL_JARRED_NATIVE_LIBRARIES}
			${GLUEGEN_JAR_LIBRARIES}
			${GLUEGEN_JARRED_NATIVE_LIBRARIES}
        ENTRY_POINT "net/osmand/core/samples/Sample1"
		OUTPUT_DIR
			"${CMAKE_JAVA_TARGET_OUTPUT_DIR}")
add_dependencies(OsmAndCore_JavaSample1
	OsmAndCoreWithJNI
	OsmAndCoreJNI
	OsmAndCore_wrapper_jar
	jogl_dummy
)
