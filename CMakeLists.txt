project(OsmAndCore)

set(target_specific_sources "")
set(target_specific_public_definitions "")
set(target_specific_public_includes "")
set(target_specific_public_libraries "")

# Target specific
if(CMAKE_TARGET_OS STREQUAL "linux")
	set(target_specific_public_libraries ${target_specific_public_libraries}
		dl
		rt
	)
endif()

# 3D rendering: OpenGL, OpenGLES2, Direct3D
if(CMAKE_TARGET_OS STREQUAL "linux" OR CMAKE_TARGET_OS STREQUAL "darwin" OR CMAKE_TARGET_OS STREQUAL "windows" OR CMAKE_TARGET_OS STREQUAL "ios" OR CMAKE_TARGET_OS STREQUAL "android")
	file(GLOB_RECURSE sources_opengl_common "src/Map/OpenGL_Common/*.c*")
	file(GLOB_RECURSE headers_opengl_common "src/Map/OpenGL_Common/*.h*")
	set(target_specific_public_definitions ${target_specific_public_definitions}
		-DGLM_SWIZZLE
	)
	set(target_specific_public_includes ${target_specific_public_includes}
		"${OSMAND_ROOT}/core/externals/glm/upstream.patched"
	)
	set(target_specific_public_libraries ${target_specific_public_libraries}
		glsl-optimizer_static
	)
	set(target_specific_sources ${target_specific_sources}
		${headers_opengl_common}
		${sources_opengl_common}
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
		${OPENGL_gl_LIBRARY}
		${OPENGL_glu_LIBRARY}
		glew_static
	)
	file(GLOB_RECURSE sources_opengl "src/Map/OpenGL/*.c*")
	file(GLOB_RECURSE headers_opengl "src/Map/OpenGL/*.h*")
	set(target_specific_sources ${target_specific_sources}
		${headers_opengl}
		${sources_opengl}
	)
endif()
if(CMAKE_TARGET_OS STREQUAL "ios" OR CMAKE_TARGET_OS STREQUAL "android")
	set(target_specific_public_definitions ${target_specific_public_definitions}
		-DOSMAND_OPENGLES2_RENDERER_SUPPORTED
	)
	file(GLOB_RECURSE sources_opengles "src/Map/OpenGLES2/*.c*")
	file(GLOB_RECURSE headers_opengles "src/Map/OpenGLES2/*.h*")
	set(target_specific_sources ${target_specific_sources}
		${headers_opengles}
		${sources_opengles}
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

set(generated_sources
	"${CMAKE_CURRENT_LIST_DIR}/gen/EmbeddedResources_bundle.cpp"
)
set_source_files_properties(${generated_sources} PROPERTIES GENERATED TRUE)
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
	${generated_sources}
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
	"${OSMAND_ROOT}/core/protos"
)
set(merged_include_dirs_public
	"${OSMAND_ROOT}/core/include"
	"${OSMAND_ROOT}/core/client"
	${target_specific_public_includes}
)
set(merged_link_libs_public
	skia_static
	protobuf_static
	gdal_static
	${target_specific_public_libraries}
)

if(OSMAND_EMBEDDED_QT)
	# When using embedded Qt, we need to use tools from either static or shared
	# version of qt.
	find_package(Qt5Core PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib/cmake" NO_DEFAULT_PATH)
	if(NOT Qt5Core_FOUND)
		find_package(Qt5Core REQUIRED PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib/cmake" NO_DEFAULT_PATH)
	endif()
else()
	# If not using embedded QT, find packages it as usual
	find_package(Qt5Core REQUIRED)
	find_package(Qt5Network REQUIRED)
	find_package(Qt5Sql REQUIRED)
endif()

set(OsmAndCore_resources
	"${OSMAND_ROOT}/resources/rendering_styles/default.render.xml"
	"${OSMAND_ROOT}/resources/routing/routing.xml"
)
if(CMAKE_HOST_WIN32 AND NOT CYGWIN)
	add_custom_command(OUTPUT "${CMAKE_CURRENT_LIST_DIR}/gen/EmbeddedResources_bundle.cpp"
		COMMAND bash ARGS --login "${CMAKE_CURRENT_LIST_DIR}/embed-resources.sh"
		DEPENDS ${OsmAndCore_resources}
		COMMENT "Embedding resources...")
else()
	add_custom_command(OUTPUT "${CMAKE_CURRENT_LIST_DIR}/gen/EmbeddedResources_bundle.cpp"
		COMMAND "${CMAKE_CURRENT_LIST_DIR}/embed-resources.py"
		DEPENDS ${OsmAndCore_resources}
		COMMENT "Embedding resources...")
endif()
add_custom_target(OsmAndCore_embed_resources
	ALL
	DEPENDS
		"${CMAKE_CURRENT_LIST_DIR}/gen/EmbeddedResources_bundle.cpp"
)
set(merged_dependencies ${merged_dependencies}
	OsmAndCore_embed_resources
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

	if(OSMAND_EMBEDDED_QT)
		find_library(Qt5Core_SHARED_LIBRARY "Qt5Core" PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib" NO_DEFAULT_PATH)
		find_library(Qt5Cored_SHARED_LIBRARY "Qt5Cored" PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib" NO_DEFAULT_PATH)
		set(Qt5Core_SHARED_LIBRARIES "")
		if(Qt5Core_SHARED_LIBRARY)
			set(Qt5Core_SHARED_LIBRARIES ${Qt5Core_SHARED_LIBRARIES}
				optimized ${Qt5Core_SHARED_LIBRARY}
			)
		endif()
		if(Qt5Cored_SHARED_LIBRARY)
			set(Qt5Core_SHARED_LIBRARIES ${Qt5Core_SHARED_LIBRARIES}
				debug ${Qt5Cored_SHARED_LIBRARY}
			)
		endif()

		find_library(Qt5Network_SHARED_LIBRARY "Qt5Network" PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib" NO_DEFAULT_PATH)
		find_library(Qt5Networkd_SHARED_LIBRARY "Qt5Networkd" PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib" NO_DEFAULT_PATH)
		set(Qt5Network_SHARED_LIBRARIES "")
		if(Qt5Network_SHARED_LIBRARY)
			set(Qt5Network_SHARED_LIBRARIES ${Qt5Network_SHARED_LIBRARIES}
				optimized ${Qt5Network_SHARED_LIBRARY}
			)
		endif()
		if(Qt5Networkd_SHARED_LIBRARY)
			set(Qt5Network_SHARED_LIBRARIES ${Qt5Network_SHARED_LIBRARIES}
				debug ${Qt5Networkd_SHARED_LIBRARY}
			)
		endif()

		find_library(Qt5Sql_SHARED_LIBRARY "Qt5Sql" PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib" NO_DEFAULT_PATH)
		find_library(Qt5Sqld_SHARED_LIBRARY "Qt5Sqld" PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib" NO_DEFAULT_PATH)
		set(Qt5Sql_SHARED_LIBRARIES "")
		if(Qt5Sql_SHARED_LIBRARY)
			set(Qt5Sql_SHARED_LIBRARIES ${Qt5Sql_SHARED_LIBRARIES}
				optimized ${Qt5Sql_SHARED_LIBRARY}
			)
		endif()
		if(Qt5Sqld_SHARED_LIBRARY)
			set(Qt5Sql_SHARED_LIBRARIES ${Qt5Sql_SHARED_LIBRARIES}
				debug ${Qt5Sqld_SHARED_LIBRARY}
			)
		endif()

		target_include_directories(OsmAndCore_shared
			PUBLIC
				"${OSMAND_EMBEDDED_QT_SHARED}/include"
				"${OSMAND_EMBEDDED_QT_SHARED}/include/QtCore"
				"${OSMAND_EMBEDDED_QT_SHARED}/include/QtNetwork"
				"${OSMAND_EMBEDDED_QT_SHARED}/include/QtSql"
		)
		target_link_libraries(OsmAndCore_shared
			LINK_PUBLIC
				${Qt5Sql_SHARED_LIBRARIES}
				${Qt5Network_SHARED_LIBRARIES}
				${Qt5Core_SHARED_LIBRARIES}
		)
	else()
		target_link_libraries(OsmAndCore_shared
			LINK_PUBLIC
				Qt5::Sql
				Qt5::Network
				Qt5::Core
		)
	endif()

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

	if(OSMAND_EMBEDDED_QT)
		find_library(Qt5Core_STATIC_LIBRARY "Qt5Core" PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib" NO_DEFAULT_PATH)
		find_library(Qt5Cored_STATIC_LIBRARY "Qt5Cored" PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib" NO_DEFAULT_PATH)
		set(Qt5Core_STATIC_LIBRARIES "")
		if(Qt5Core_STATIC_LIBRARY)
			set(Qt5Core_STATIC_LIBRARIES ${Qt5Core_STATIC_LIBRARIES}
				optimized ${Qt5Core_STATIC_LIBRARY}
			)
		endif()
		if(Qt5Cored_STATIC_LIBRARY)
			set(Qt5Core_STATIC_LIBRARIES ${Qt5Core_STATIC_LIBRARIES}
				debug ${Qt5Cored_STATIC_LIBRARY}
			)
		endif()

		find_library(Qt5Network_STATIC_LIBRARY "Qt5Network" PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib" NO_DEFAULT_PATH)
		find_library(Qt5Networkd_STATIC_LIBRARY "Qt5Networkd" PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib" NO_DEFAULT_PATH)
		set(Qt5Network_STATIC_LIBRARIES "")
		if(Qt5Network_STATIC_LIBRARY)
			set(Qt5Network_STATIC_LIBRARIES ${Qt5Network_STATIC_LIBRARIES}
				optimized ${Qt5Network_STATIC_LIBRARY}
			)
		endif()
		if(Qt5Networkd_STATIC_LIBRARY)
			set(Qt5Network_STATIC_LIBRARIES ${Qt5Network_STATIC_LIBRARIES}
				debug ${Qt5Networkd_STATIC_LIBRARY}
			)
		endif()

		find_library(Qt5Sql_STATIC_LIBRARY "Qt5Sql" PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib" NO_DEFAULT_PATH)
		find_library(Qt5Sqld_STATIC_LIBRARY "Qt5Sqld" PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib" NO_DEFAULT_PATH)
		set(Qt5Sql_STATIC_LIBRARIES "")
		if(Qt5Sql_STATIC_LIBRARY)
			set(Qt5Sql_STATIC_LIBRARIES ${Qt5Sql_STATIC_LIBRARIES}
				optimized ${Qt5Sql_STATIC_LIBRARY}
			)
		endif()
		if(Qt5Sqld_STATIC_LIBRARY)
			set(Qt5Sql_STATIC_LIBRARIES ${Qt5Sql_STATIC_LIBRARIES}
				debug ${Qt5Sqld_STATIC_LIBRARY}
			)
		endif()

		target_include_directories(OsmAndCore_static
			PUBLIC
				"${OSMAND_EMBEDDED_QT_STATIC}/include"
				"${OSMAND_EMBEDDED_QT_STATIC}/include/QtCore"
				"${OSMAND_EMBEDDED_QT_STATIC}/include/QtNetwork"
				"${OSMAND_EMBEDDED_QT_STATIC}/include/QtSql"
		)
		target_link_libraries(OsmAndCore_static
			LINK_PUBLIC
				${Qt5Sql_STATIC_LIBRARIES}
				${Qt5Network_STATIC_LIBRARIES}
				${Qt5Core_STATIC_LIBRARIES}
		)
	else()
		target_link_libraries(OsmAndCore_static
			LINK_PUBLIC
				Qt5::Sql
				Qt5::Network
				Qt5::Core
		)
	endif()

	target_link_libraries(OsmAndCore_static
		LINK_PUBLIC
			${merged_link_libs_public}
	)
endif()
