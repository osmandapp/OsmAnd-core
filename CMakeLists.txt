project(OsmAndCore)

# Bump this number each time a new source file is committed to repository, source file removed from repository or renamed: 96

set(target_specific_sources "")
set(target_specific_public_definitions "")
set(target_specific_public_includes "")
set(target_specific_public_libraries "")
set(target_specific_private_definitions "")
set(target_specific_dependencies "")

# Target specific
if (CMAKE_TARGET_OS STREQUAL "linux")
	find_library(dl_LIBRARY dl)
	find_library(rt_LIBRARY rt)
	set(target_specific_public_libraries ${target_specific_public_libraries}
		${dl_LIBRARY}
		${rt_LIBRARY}
	)
elseif (CMAKE_TARGET_OS STREQUAL "macosx")
	find_library(objc_LIBRARY objc)
	find_library(Cocoa_LIBRARY Cocoa)
	find_library(CoreServices_LIBRARY CoreServices)
	find_library(SystemConfiguration_LIBRARY SystemConfiguration)
	find_library(CFNetwork_LIBRARY CFNetwork)
	find_library(Security_LIBRARY Security)
	find_library(z_LIBRARY z)
	set(target_specific_public_libraries ${target_specific_public_libraries}
		"${Cocoa_LIBRARY}/Cocoa"
		"${CoreServices_LIBRARY}/CoreServices"
		"${SystemConfiguration_LIBRARY}/SystemConfiguration"
		"${CFNetwork_LIBRARY}/CFNetwork"
		"${Security_LIBRARY}/Security"
		${z_LIBRARY}
		${objc_LIBRARY}
	)
elseif (CMAKE_TARGET_OS STREQUAL "android")
	find_library(dl_LIBRARY dl)
	find_library(gnustl_shared_LIBRARY gnustl_shared)
	find_library(log_LIBRARY log)
	find_library(c_LIBRARY c)
	find_library(gcc_LIBRARY gcc)
	find_library(m_LIBRARY m)
	set(target_specific_public_libraries ${target_specific_public_libraries}
		${gnustl_shared_LIBRARY}
		${log_LIBRARY}
		${c_LIBRARY}
		${gcc_LIBRARY}
		${m_LIBRARY}
		${dl_LIBRARY}
	)
	set(target_specific_private_definitions ${target_specific_private_definitions}
		-DOSMAND_GPU_DEBUG=1
	)
endif()

if (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
	# Set warning level 4
	set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} /W4")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

	# Force warning to be an error "deletion of pointer to incomplete type 'type'; no destructor called"
	set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} /we4150")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /we4150")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /we4150")

	# Disable following warnings:
	#  - C4290: 'C++ exception specification ignored except to indicate a function is not __declspec(nothrow)'
	#  - C4201: 'nonstandard extension used : nameless struct/union'
	set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} /wd4290 /wd4201")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4290 /wd4201")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4290 /wd4201")
elseif (CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Wall")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

if (CMAKE_TARGET_OS STREQUAL "linux" OR
	CMAKE_TARGET_OS STREQUAL "macosx" OR
	CMAKE_TARGET_OS STREQUAL "windows" OR
	CMAKE_TARGET_OS STREQUAL "ios" OR
	CMAKE_TARGET_OS STREQUAL "android")
	file(GLOB sources_opengl "src/Map/OpenGL/*.c*")
	file(GLOB headers_opengl "src/Map/OpenGL/*.h*")
	set(target_specific_sources ${target_specific_sources}
		${headers_opengl}
		${sources_opengl}
	)
	set(target_specific_private_definitions ${target_specific_private_definitions}
		-DOSMAND_OPENGL_RENDERERS_SUPPORTED
	)
endif()
if (CMAKE_TARGET_OS STREQUAL "linux" OR
	CMAKE_TARGET_OS STREQUAL "macosx" OR
	CMAKE_TARGET_OS STREQUAL "windows")
	find_package(OpenGL REQUIRED)
	set(target_specific_public_definitions ${target_specific_public_definitions}
		${OPENGL_DEFINITIONS}
	)
	set(target_specific_private_definitions ${target_specific_private_definitions}
		-DOSMAND_OPENGL2PLUS_RENDERER_SUPPORTED
	)
	set(target_specific_public_includes ${target_specific_public_includes}
		${OPENGL_INCLUDE_DIRS}
	)
	set(target_specific_public_libraries ${target_specific_public_libraries}
		${OPENGL_gl_LIBRARY}
		${OPENGL_glu_LIBRARY}
		glew_static
	)
	set(target_specific_dependencies ${target_specific_dependencies}
		glew_static
	)
	file(GLOB sources_opengl2plus "src/Map/OpenGL/OpenGL2plus/*.c*")
	file(GLOB headers_opengl2plus "src/Map/OpenGL/OpenGL2plus/*.h*")
	set(target_specific_sources ${target_specific_sources}
		${headers_opengl2plus}
		${sources_opengl2plus}
	)
endif()
if (CMAKE_TARGET_OS STREQUAL "ios" OR
	CMAKE_TARGET_OS STREQUAL "android")
	set(target_specific_private_definitions ${target_specific_private_definitions}
		-DOSMAND_OPENGLES2_RENDERER_SUPPORTED
	)
	file(GLOB sources_opengles2 "src/Map/OpenGL/OpenGLES2/*.c*")
	file(GLOB headers_opengles2 "src/Map/OpenGL/OpenGLES2/*.h*")
	set(target_specific_sources ${target_specific_sources}
		${headers_opengles2}
		${sources_opengles2}
	)

	if (CMAKE_TARGET_OS STREQUAL "android")
		find_library(EGL_LIBRARY EGL)
		find_library(GLESv2_LIBRARY GLESv2)
		set(target_specific_public_libraries ${target_specific_public_libraries}
			${EGL_LIBRARY}
			${GLESv2_LIBRARY}
		)
	endif()
endif()

file(GLOB_RECURSE includes "include/*.h*")
file(GLOB headers "src/*.h*")
file(GLOB headers_data "src/Data/*.h*")
file(GLOB headers_data_model "src/Data/Model/*.h*")
#file(GLOB headers_routing "src/Routing/*.h*")
file(GLOB headers_map "src/Map/*.h*")
file(GLOB sources "src/*.c*")
file(GLOB sources_data "src/Data/*.c*")
file(GLOB sources_data_model "src/Data/Model/*.c*")
#file(GLOB sources_routing "src/Routing/*.c*")
file(GLOB sources_map "src/Map/*.c*")

set(merged_sources
	${headers}
	${includes}
	${headers_data}
	${headers_data_model}
	#${headers_routing}
	${headers_map}
	${sources}
	${sources_data}
	${sources_data_model}
	#${sources_routing}
	${sources_map}
	${target_specific_sources}
	"protos/OBF.pb.h"
)
set(merged_dependencies
	skia_static
	protobuf_static
	gdal_static
	icu4c_static
	archive_static
	${target_specific_dependencies}
)
set(merged_compile_definitions_public
	-DGLM_SWIZZLE
	${target_specific_public_definitions}
)
set(merged_compile_definitions_private
	${target_specific_private_definitions}
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
	${target_specific_public_includes}
)
set(merged_link_libs_public
	skia_static
	protobuf_static
	gdal_static
	icu4c_static
	archive_static
	glm_headers_only
	boost_static_precompiled
	${target_specific_public_libraries}
)

if (OSMAND_EMBEDDED_QT)
	# When using embedded Qt, we need to use tools from either static or shared
	# version of qt.
	if (CMAKE_SHARED_LIBS_ALLOWED_ON_TARGET)
		find_package(Qt5Core REQUIRED PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib/cmake" NO_DEFAULT_PATH)
	elseif (CMAKE_STATIC_LIBS_ALLOWED_ON_TARGET)
		find_package(Qt5Core REQUIRED PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib/cmake" NO_DEFAULT_PATH)
	endif()
else()
	# If not using embedded QT, find packages it as usual
	find_package(Qt5Core REQUIRED)
	find_package(Qt5Network REQUIRED)
	find_package(Qt5Sql REQUIRED)
endif()

if (CMAKE_SHARED_LIBS_ALLOWED_ON_TARGET)
	add_library(OsmAndCore_shared SHARED
		${merged_sources})
	add_dependencies(OsmAndCore_shared
		${merged_dependencies})
	target_compile_definitions(OsmAndCore_shared
		PRIVATE
			-DOSMAND_CORE_INTERNAL
			-DOSMAND_CORE_EXPORTS
			${merged_compile_definitions_private}
		PUBLIC
			${merged_compile_definitions_public}
	)
	target_include_directories(OsmAndCore_shared
		PRIVATE
			${merged_include_dirs_private}
		PUBLIC
			${merged_include_dirs_public}
	)

	if (OSMAND_EMBEDDED_QT)
		find_library(Qt5Core_SHARED_LIBRARY "Qt5Core" PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib" NO_DEFAULT_PATH)
		find_library(Qt5Cored_SHARED_LIBRARY "Qt5Cored" PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib" NO_DEFAULT_PATH)
		set(Qt5Core_SHARED_LIBRARIES "")
		if (Qt5Core_SHARED_LIBRARY AND Qt5Cored_SHARED_LIBRARY)
			set(Qt5Core_SHARED_LIBRARIES ${Qt5Core_SHARED_LIBRARIES}
				optimized ${Qt5Core_SHARED_LIBRARY}
			)
		else()
			set(Qt5Core_SHARED_LIBRARIES ${Qt5Core_SHARED_LIBRARIES}
				${Qt5Core_SHARED_LIBRARY}
			)
		endif()
		if (Qt5Cored_SHARED_LIBRARY)
			set(Qt5Core_SHARED_LIBRARIES ${Qt5Core_SHARED_LIBRARIES}
				debug ${Qt5Cored_SHARED_LIBRARY}
			)
		endif()

		find_library(Qt5Network_SHARED_LIBRARY "Qt5Network" PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib" NO_DEFAULT_PATH)
		find_library(Qt5Networkd_SHARED_LIBRARY "Qt5Networkd" PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib" NO_DEFAULT_PATH)
		set(Qt5Network_SHARED_LIBRARIES "")
		if (Qt5Network_SHARED_LIBRARY AND Qt5Networkd_SHARED_LIBRARY)
			set(Qt5Network_SHARED_LIBRARIES ${Qt5Network_SHARED_LIBRARIES}
				optimized ${Qt5Network_SHARED_LIBRARY}
			)
		else()
			set(Qt5Network_SHARED_LIBRARIES ${Qt5Network_SHARED_LIBRARIES}
				${Qt5Network_SHARED_LIBRARY}
			)
		endif()
		if (Qt5Networkd_SHARED_LIBRARY)
			set(Qt5Network_SHARED_LIBRARIES ${Qt5Network_SHARED_LIBRARIES}
				debug ${Qt5Networkd_SHARED_LIBRARY}
			)
		endif()

		find_library(Qt5Sql_SHARED_LIBRARY "Qt5Sql" PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib" NO_DEFAULT_PATH)
		find_library(Qt5Sqld_SHARED_LIBRARY "Qt5Sqld" PATHS "${OSMAND_EMBEDDED_QT_SHARED}/lib" NO_DEFAULT_PATH)
		set(Qt5Sql_SHARED_LIBRARIES "")
		if (Qt5Sql_SHARED_LIBRARY AND Qt5Sqld_SHARED_LIBRARY)
			set(Qt5Sql_SHARED_LIBRARIES ${Qt5Sql_SHARED_LIBRARIES}
				optimized ${Qt5Sql_SHARED_LIBRARY}
			)
		else()
			set(Qt5Sql_SHARED_LIBRARIES ${Qt5Sql_SHARED_LIBRARIES}
				${Qt5Sql_SHARED_LIBRARY}
			)
		endif()
		if (Qt5Sqld_SHARED_LIBRARY)
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

if (CMAKE_STATIC_LIBS_ALLOWED_ON_TARGET)
	add_library(OsmAndCore_static STATIC
		${merged_sources})
	add_dependencies(OsmAndCore_static
		${merged_dependencies})
	target_compile_definitions(OsmAndCore_static
		PRIVATE
			-DOSMAND_CORE_INTERNAL		
			${merged_compile_definitions_private}
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

	if (OSMAND_EMBEDDED_QT)
		find_library(Qt5Core_STATIC_LIBRARY "Qt5Core" PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib" NO_DEFAULT_PATH)
		find_library(Qt5Cored_STATIC_LIBRARY "Qt5Cored" PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib" NO_DEFAULT_PATH)
		set(Qt5Core_STATIC_LIBRARIES "")
		if (Qt5Core_STATIC_LIBRARY AND Qt5Cored_STATIC_LIBRARY)
			set(Qt5Core_STATIC_LIBRARIES ${Qt5Core_STATIC_LIBRARIES}
				optimized ${Qt5Core_STATIC_LIBRARY}
			)
		else()
			set(Qt5Core_STATIC_LIBRARIES ${Qt5Core_STATIC_LIBRARIES}
				${Qt5Core_STATIC_LIBRARY}
			)
		endif()
		if (Qt5Cored_STATIC_LIBRARY)
			set(Qt5Core_STATIC_LIBRARIES ${Qt5Core_STATIC_LIBRARIES}
				debug ${Qt5Cored_STATIC_LIBRARY}
			)
		endif()

		find_library(Qt5Network_STATIC_LIBRARY "Qt5Network" PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib" NO_DEFAULT_PATH)
		find_library(Qt5Networkd_STATIC_LIBRARY "Qt5Networkd" PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib" NO_DEFAULT_PATH)
		set(Qt5Network_STATIC_LIBRARIES "")
		if (Qt5Network_STATIC_LIBRARY AND Qt5Networkd_STATIC_LIBRARY)
			set(Qt5Network_STATIC_LIBRARIES ${Qt5Network_STATIC_LIBRARIES}
				optimized ${Qt5Network_STATIC_LIBRARY}
			)
		else()
			set(Qt5Network_STATIC_LIBRARIES ${Qt5Network_STATIC_LIBRARIES}
				${Qt5Network_STATIC_LIBRARY}
			)
		endif()
		if (Qt5Networkd_STATIC_LIBRARY)
			set(Qt5Network_STATIC_LIBRARIES ${Qt5Network_STATIC_LIBRARIES}
				debug ${Qt5Networkd_STATIC_LIBRARY}
			)
		endif()

		find_library(Qt5Sql_STATIC_LIBRARY "Qt5Sql" PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib" NO_DEFAULT_PATH)
		find_library(Qt5Sqld_STATIC_LIBRARY "Qt5Sqld" PATHS "${OSMAND_EMBEDDED_QT_STATIC}/lib" NO_DEFAULT_PATH)
		set(Qt5Sql_STATIC_LIBRARIES "")
		if (Qt5Sql_STATIC_LIBRARY AND Qt5Sqld_STATIC_LIBRARY)
			set(Qt5Sql_STATIC_LIBRARIES ${Qt5Sql_STATIC_LIBRARIES}
				optimized ${Qt5Sql_STATIC_LIBRARY}
			)
		else()
			set(Qt5Sql_STATIC_LIBRARIES ${Qt5Sql_STATIC_LIBRARIES}
				${Qt5Sql_STATIC_LIBRARY}
			)
		endif()
		if (Qt5Sqld_STATIC_LIBRARY)
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
