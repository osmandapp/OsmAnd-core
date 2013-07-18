project(OsmAndCore)

set(target_specific_sources "")
set(target_specific_public_definitions "")
set(target_specific_public_includes "")
set(target_specific_public_libraries "")

# Target specific
if(CMAKE_TARGET_OS STREQUAL "linux")
	set(target_specific_public_libraries ${target_specific_public_libraries}
		dl
	)
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
file(GLOB headers_map "src/Map/*.h*")
file(GLOB sources "src/*.c*")
file(GLOB sources_data "src/Data/*.c*")
file(GLOB sources_data_model "src/Data/Model/*.c*")
file(GLOB sources_routing "src/Routing/*.c*")
file(GLOB sources_map "src/Map/*.c*")
file(GLOB_RECURSE headers_client "client/*.h*")
file(GLOB_RECURSE sources_client  "client/*.c*")

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Sql REQUIRED)

set(merged_sources
	${headers}
	${includes}
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
set(merged_dependencies
	skia_static
	protobuf_static
	gdal_static
	${target_specific_public_libraries}
)
set(merged_compile_definitions_public
	${target_specific_public_definitions}
)
set(merged_include_dirs_private
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
)
set(merged_include_dirs_public
	"${OSMAND_ROOT}/core/include"
	"${OSMAND_ROOT}/core/client"
	"${OSMAND_ROOT}/core/protos"
	${target_specific_public_includes}
	${Qt5Core_INCLUDE_DIRS}
	${Qt5Network_INCLUDE_DIRS}
	${Qt5Sql_INCLUDE_DIRS}
)
set(merged_link_libs_public
	skia_static
	protobuf_static
	gdal_static
	${target_specific_public_libraries}
)
set(merged_qt5_modules
	Core
	Network
	Sql
)

if(CMAKE_SHARED_LIBS_ALLOWED_ON_TARGET)
	add_library(OsmAndCore_shared SHARED ${merged_sources})
	set_target_properties(OsmAndCore_shared PROPERTIES AUTOMOC TRUE)
	add_dependencies(OsmAndCore_shared ${merged_dependencies})
	target_compile_definitions(OsmAndCore_shared
		PRIVATE
			-DOSMAND_CORE_EXPORTS
		PUBLIC
			${merged_compile_definitions_public}
	)
	target_include_directories(OsmAndCore_shared
		PRIVATE
			${merged_include_dirs_private}
		PUBLIC
			${merged_include_dirs_public}
	)
	qt5_use_modules(OsmAndCore_shared
		LINK_PUBLIC
			${merged_qt5_modules}
	)
	target_link_libraries(OsmAndCore_shared
		LINK_PUBLIC
			${merged_link_libs_public}
	)
endif()

if(CMAKE_STATIC_LIBS_ALLOWED_ON_TARGET)
	add_library(OsmAndCore_static STATIC ${merged_sources})
	set_target_properties(OsmAndCore_static PROPERTIES AUTOMOC TRUE)
	add_dependencies(OsmAndCore_static ${merged_dependencies})
	target_compile_definitions(OsmAndCore_static
		PUBLIC
			-DOSMAND_CORE_STATIC
			${merged_compile_definitions_public}
	)
	target_include_directories(OsmAndCore_static
		PRIVATE
			${merged_include_dirs_private}
		PUBLIC
			${merged_include_dirs_public}
	)
	qt5_use_modules(OsmAndCore_static
		LINK_PUBLIC
			${merged_qt5_modules}
	)
	target_link_libraries(OsmAndCore_static
		LINK_PUBLIC
			${merged_link_libs_public}
	)
endif()
