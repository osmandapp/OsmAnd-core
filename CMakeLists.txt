project(OsmAndCore)

# Bump this number each time a new source file is committed to repository, source file removed from repository or renamed: 141

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
	find_library(CoreGraphics_LIBRARY CoreGraphics)
	find_library(CoreText_LIBRARY CoreText)
	find_library(CoreFoundation_LIBRARY CoreFoundation)
	find_library(CoreServices_LIBRARY CoreServices)
	find_library(SystemConfiguration_LIBRARY SystemConfiguration)
	find_library(CFNetwork_LIBRARY CFNetwork)
	find_library(Security_LIBRARY Security)
	find_library(z_LIBRARY z)
	set(library_suffix_tbd "")

	if (CMAKE_TARGET_BUILD_TOOL STREQUAL "xcode")
		set(library_suffix_tbd ".tbd")
	endif()
	message("LIBRARY SUFFIX ${library_suffix_tbd}")
	set(target_specific_public_libraries ${target_specific_public_libraries}
		"${Cocoa_LIBRARY}/Cocoa${library_suffix_tbd}"
		"${CoreServices_LIBRARY}/CoreServices${library_suffix_tbd}"
		"${CoreText_LIBRARY}/CoreText${library_suffix_tbd}"
		"${CoreGraphics_LIBRARY}/CoreGraphics${library_suffix_tbd}"
		"${CoreFoundation_LIBRARY}/CoreFoundation${library_suffix_tbd}"
		"${SystemConfiguration_LIBRARY}/SystemConfiguration${library_suffix_tbd}"
		"${CFNetwork_LIBRARY}/CFNetwork${library_suffix_tbd}"
		"${Security_LIBRARY}/Security${library_suffix_tbd}"
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
elseif (CMAKE_TARGET_OS STREQUAL "windows")
	if (CMAKE_HOST_WIN32)
		find_library(ws2_32_LIBRARY ws2_32)
	else()
		set(ws2_32_LIBRARY ws2_32)
	endif()
	set(target_specific_public_libraries ${target_specific_public_libraries}
		${ws2_32_LIBRARY}
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

file(GLOB includes
	"include/proper/*.h*"
	"include/*.h*"
	"include/OsmAndCore/*.h*"
	"include/OsmAndCore/Concurrent/*.h*"
	"include/OsmAndCore/Data/*.h*"
	"include/OsmAndCore/Map/*.h*"
	#"include/OsmAndCore/Routing/*.h*"
	"include/OsmAndCore/Search/*.h*")
file(GLOB headers
	"src/*.h*"
	"src/Concurrent/*.h*"
	"src/Data/*.h*"
	"src/Map/*.h*"
	#"src/Routing/*.h*"
	"src/Search/*.h*")
file(GLOB sources
	"src/*.c*"
	"src/Concurrent/*.c*"
	"src/Data/*.c*"
	"src/Map/*.c*"
	#"src/Routing/*.c*"
	"src/Search/*.c*")

set(merged_sources
	${includes}
	${headers}
	${sources}
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
	z_static
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
	"${OSMAND_ROOT}/core/include/OsmAndCore/Concurrent"
	"${OSMAND_ROOT}/core/include/OsmAndCore/Data"
	"${OSMAND_ROOT}/core/include/OsmAndCore/Routing"
	"${OSMAND_ROOT}/core/include/OsmAndCore/Map"
	"${OSMAND_ROOT}/core/include/OsmAndCore/Search"
	"${OSMAND_ROOT}/core/src"
	"${OSMAND_ROOT}/core/src/Data"
	"${OSMAND_ROOT}/core/src/Routing"
	"${OSMAND_ROOT}/core/src/Map"
	"${OSMAND_ROOT}/core/src/Search"
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
	GeographicLib_STATIC
	${target_specific_public_libraries}
)

if (OSMAND_OWN_SHIPPED_QT)
	# When using embedded Qt, we need to use tools from either static or shared
	# version of qt.
	if (CMAKE_SHARED_LIBS_ALLOWED_ON_TARGET)
		find_package(Qt5Core REQUIRED PATHS "${OSMAND_OWN_SHIPPED_QT_SHARED}/lib/cmake" NO_DEFAULT_PATH)
	elseif (CMAKE_STATIC_LIBS_ALLOWED_ON_TARGET)
		find_package(Qt5Core REQUIRED PATHS "${OSMAND_OWN_SHIPPED_QT_STATIC}/lib/cmake" NO_DEFAULT_PATH)
	endif()
else()
	# If not using embedded QT, find packages it as usual
	find_package(Qt5Core REQUIRED)
	find_package(Qt5Network REQUIRED)
	find_package(Qt5Sql REQUIRED)
endif()

if (CMAKE_SHARED_LIBS_ALLOWED_ON_TARGET)
	# 'OsmAndCore_shared' - linking to Qt shared libraries (or any if not own-shipped), OsmAndCore shared
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
	if (OSMAND_OWN_SHIPPED_QT)
		target_include_directories(OsmAndCore_shared
			PUBLIC
				"${OSMAND_OWN_SHIPPED_QT_SHARED}/include"
				"${OSMAND_OWN_SHIPPED_QT_SHARED}/include/QtCore"
				"${OSMAND_OWN_SHIPPED_QT_SHARED}/include/QtNetwork"
				"${OSMAND_OWN_SHIPPED_QT_SHARED}/include/QtSql"
		)
		target_link_libraries(OsmAndCore_shared
			LINK_PUBLIC
				${Qt5Sql_SHARED_LIBRARIES}
				${Qt5Network_SHARED_LIBRARIES}
				${Qt5Core_SHARED_LIBRARIES}
				${qtpcre_SHARED_LIBRARIES}
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

if (CMAKE_STATIC_LIBS_ALLOWED_ON_TARGET AND
	CMAKE_SHARED_LIBS_ALLOWED_ON_TARGET)
	# 'OsmAndCore_static' - linking to Qt shared libraries (or any if not own-shipped), OsmAndCore static
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
	if (OSMAND_OWN_SHIPPED_QT)
		target_include_directories(OsmAndCore_static
			PUBLIC
				"${OSMAND_OWN_SHIPPED_QT_SHARED}/include"
				"${OSMAND_OWN_SHIPPED_QT_SHARED}/include/QtCore"
				"${OSMAND_OWN_SHIPPED_QT_SHARED}/include/QtNetwork"
				"${OSMAND_OWN_SHIPPED_QT_SHARED}/include/QtSql"
		)
		target_link_libraries(OsmAndCore_static
			LINK_PUBLIC
				${Qt5Sql_SHARED_LIBRARIES}
				${Qt5Network_SHARED_LIBRARIES}
				${Qt5Core_SHARED_LIBRARIES}
				${qtpcre_SHARED_LIBRARIES}
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

if (CMAKE_STATIC_LIBS_ALLOWED_ON_TARGET AND
	OSMAND_OWN_SHIPPED_QT)

	if (CMAKE_SHARED_LIBS_ALLOWED_ON_TARGET)
		# 'OsmAndCore_shared_standalone' - linking to Qt static libraries, OsmAndCore shared
		add_library(OsmAndCore_shared_standalone SHARED
			${merged_sources})
		add_dependencies(OsmAndCore_shared_standalone
			${merged_dependencies})
		target_compile_definitions(OsmAndCore_shared_standalone
			PRIVATE
				-DOSMAND_CORE_INTERNAL
				-DOSMAND_CORE_EXPORTS
				${merged_compile_definitions_private}
			PUBLIC
				${merged_compile_definitions_public}
		)
		target_include_directories(OsmAndCore_shared_standalone
			PRIVATE
				${merged_include_dirs_private}
			PUBLIC
				${merged_include_dirs_public}
		)
		target_include_directories(OsmAndCore_shared_standalone
			PUBLIC
				"${OSMAND_OWN_SHIPPED_QT_STATIC}/include"
				"${OSMAND_OWN_SHIPPED_QT_STATIC}/include/QtCore"
				"${OSMAND_OWN_SHIPPED_QT_STATIC}/include/QtNetwork"
				"${OSMAND_OWN_SHIPPED_QT_STATIC}/include/QtSql"
		)
		target_link_libraries(OsmAndCore_shared_standalone
			LINK_PUBLIC
				${Qt5Sql_STATIC_LIBRARIES}
				${Qt5Network_STATIC_LIBRARIES}
				${Qt5Core_STATIC_LIBRARIES}
				${qtpcre_STATIC_LIBRARIES}
				${merged_link_libs_public}
		)
	endif()

	# 'OsmAndCore_static_standalone' - linking to Qt static libraries, OsmAndCore static
	add_library(OsmAndCore_static_standalone STATIC
		${merged_sources})
	add_dependencies(OsmAndCore_static_standalone
		${merged_dependencies})
	target_compile_definitions(OsmAndCore_static_standalone
		PRIVATE
			-DOSMAND_CORE_INTERNAL		
			${merged_compile_definitions_private}
		PUBLIC
			-DOSMAND_CORE_STATIC
			${merged_compile_definitions_public}
	)
	target_include_directories(OsmAndCore_static_standalone
		PRIVATE
			${merged_include_dirs_private}
		PUBLIC
			${merged_include_dirs_public}
	)
	target_include_directories(OsmAndCore_static_standalone
		PUBLIC
			"${OSMAND_OWN_SHIPPED_QT_STATIC}/include"
			"${OSMAND_OWN_SHIPPED_QT_STATIC}/include/QtCore"
			"${OSMAND_OWN_SHIPPED_QT_STATIC}/include/QtNetwork"
			"${OSMAND_OWN_SHIPPED_QT_STATIC}/include/QtSql"
	)
	target_link_libraries(OsmAndCore_static_standalone
		LINK_PUBLIC
			${Qt5Sql_STATIC_LIBRARIES}
			${Qt5Network_STATIC_LIBRARIES}
			${Qt5Core_STATIC_LIBRARIES}
			${qtpcre_STATIC_LIBRARIES}
			${merged_link_libs_public}
	)
endif()
