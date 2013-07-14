project(OsmAndCore)

set(target_specific_sources "")
set(target_specific_public_definitions "")
set(target_specific_public_includes "")
set(target_specific_public_libraries "")
if(CMAKE_TARGET_OS STREQUAL "linux")
elseif(CMAKE_TARGET_OS STREQUAL "darwin")
elseif(CMAKE_TARGET_OS STREQUAL "windows")
endif()

# 3D rendering: OpenGL, OpenGLES, Direct3D
if(CMAKE_TARGET_OS STREQUAL "linux" OR CMAKE_TARGET_OS STREQUAL "darwin" OR CMAKE_TARGET_OS STREQUAL "windows" OR CMAKE_TARGET_OS STREQUAL "ios" OR CMAKE_TARGET_OS STREQUAL "android")
	file(GLOB_RECURSE sources_opengl_base "src/Map/OpenGL_Base/*.c*")
	file(GLOB_RECURSE headers_opengl_base "src/Map/OpenGL_Base/*.h*")
	set(target_specific_public_definitions ${target_specific_public_definitions}
		-DGLM_SWIZZLE
	)
	set(target_specific_public_includes ${target_specific_public_includes}
		"${OSMAND_ROOT}/core/externals/glm/upstream.patched"
	)
	set(target_specific_sources ${target_specific_sources}
		${headers_opengl_base}
		${sources_opengl_base}
	)
endif()
if(CMAKE_TARGET_OS STREQUAL "linux" OR CMAKE_TARGET_OS STREQUAL "darwin" OR CMAKE_TARGET_OS STREQUAL "windows")
	find_package(OpenGL REQUIRED)
	set(target_specific_public_definitions ${target_specific_public_definitions}
		-DOSMAND_OPENGL_RENDERER_SUPPORTED
		${OPENGL_DEFINITIONS}
	)
	set(target_specific_public_includes ${target_specific_public_includes}
		${OPENGL_INCLUDE_DIRS}
	)
	set(target_specific_public_libraries ${target_specific_public_libraries}
		${OPENGL_LIBRARIES}
		glew_static
	)
	file(GLOB_RECURSE sources_opengl "src/Map/OpenGL/*.c*")
	file(GLOB_RECURSE headers_opengl "src/Map/OpenGL/*.h*")
	set(target_specific_sources ${target_specific_sources}
		${headers_opengl}
		${sources_opengl}
	)
endif()

file(GLOB_RECURSE includes "include/*.h*")
file(GLOB headers "src/*.h*")
file(GLOB headers_data "src/Data/*.h*")
file(GLOB headers_data_model "src/Data/Model/*.h*")
file(GLOB headers_routing "src/Routing/*.h*")
file(GLOB headers_map "src/Routing/*.h*")
file(GLOB sources "src/*.c*")
file(GLOB sources_data "src/Data/*.c*")
file(GLOB sources_data_model "src/Data/Model/*.c*")
file(GLOB sources_routing "src/Routing/*.c*")
file(GLOB sources_map "src/Routing/*.c*")
file(GLOB_RECURSE headers_client "client/*.h*")
file(GLOB_RECURSE sources_client  "client/*.c*")
add_library(OsmAndCore SHARED
	${includes}
	${headers}
	${headers_data}
	${headers_data_model}
	${headers_routing}
	${headers_map}
	${sources}
	${sources_data}
	${sources_data_model}
	${sources_routing}
	${sources_map}
	${headers_client}
	${sources_client}
	${target_specific_sources}
	"protos/OBF.pb.h"
)
set_target_properties(OsmAndCore PROPERTIES AUTOMOC TRUE)

add_dependencies(OsmAndCore
	skia_static
	protobuf_static
	gdal_static
	${target_specific_public_libraries}
)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Sql REQUIRED)

target_compile_definitions(OsmAndCore
	PRIVATE
		-DOSMAND_CORE_EXPORTS
	PUBLIC
		${target_specific_public_definitions}
)
target_include_directories(OsmAndCore
	PRIVATE
		"${OSMAND_ROOT}/core/include"
		"${OSMAND_ROOT}/core/include/OsmAndCore"
		"${OSMAND_ROOT}/core/include/OsmAndCore/Data"
		"${OSMAND_ROOT}/core/include/OsmAndCore/Data/Model"
		"${OSMAND_ROOT}/core/include/OsmAndCore/Routing"
		"${OSMAND_ROOT}/core/include/OsmAndCore/Map"
		"${OSMAND_ROOT}/core/src"
		"${OSMAND_ROOT}/core/src/Data"
		"${OSMAND_ROOT}/core/src/Data/Model"
		"${OSMAND_ROOT}/core/src/Routing"
		"${OSMAND_ROOT}/core/src/Map"
	PUBLIC
		"${OSMAND_ROOT}/core/include"
		"${OSMAND_ROOT}/core/src"
		"${OSMAND_ROOT}/core/client"
		"${OSMAND_ROOT}/core/protos"
		${target_specific_public_includes}
		${Qt5Core_INCLUDE_DIRS}
		${Qt5Network_INCLUDE_DIRS}
		${Qt5Sql_INCLUDE_DIRS}
)
target_link_libraries(OsmAndCore
	LINK_PUBLIC
		skia_static
		protobuf_static
		gdal_static
		${target_specific_public_libraries}
)
qt5_use_modules(OsmAndCore
	LINK_PUBLIC
		Core
		Network
		Sql
)
