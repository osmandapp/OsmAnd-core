project(OsmAndCore)

include_directories(AFTER SYSTEM
	"${OSMAND_ROOT}/externals/skia/upstream.patched/include/core"
	"${OSMAND_ROOT}/externals/skia/upstream.patched/include/images"
	"${OSMAND_ROOT}/externals/skia/upstream.patched/include/utils"
	"${OSMAND_ROOT}/externals/skia/upstream.patched/include/config"
	"${OSMAND_ROOT}/externals/skia/upstream.patched/include/effects"
	"${OSMAND_ROOT}/externals/skia/upstream.patched/include/utils/android"
	"${OSMAND_ROOT}/externals/skia/upstream.patched/src/core"
	"${OSMAND_ROOT}/externals/expat/upstream.patched/lib"
	"${OSMAND_ROOT}/externals/freetype/upstream.patched/include"
	"${OSMAND_ROOT}/externals/giflib/upstream.patched/lib"
	"${OSMAND_ROOT}/externals/jpeg/upstream.patched"
	"${OSMAND_ROOT}/externals/libpng/upstream.patched"
	"${OSMAND_ROOT}/externals/harfbuzz/upstream.patched/src"
	"${OSMAND_ROOT}/externals/protobuf/upstream.patched/src"
	"${OSMAND_ROOT}/native/include"
	"${OSMAND_ROOT}/native/src"
	"$ENV{JAVA_HOME}/include"
)
if(CMAKE_TARGET_OS STREQUAL "windows")
	include_directories(AFTER SYSTEM
		"$ENV{JAVA_HOME}/include/win32"
	)
	add_definitions(-DSK_BUILD_FOR_WIN32)
elseif(CMAKE_TARGET_OS STREQUAL "darwin")
	include_directories(AFTER SYSTEM
		"$ENV{JAVA_HOME}/include/darwin"
	)
	add_definitions(-DSK_BUILD_FOR_MAC)
elseif(CMAKE_TARGET_OS STREQUAL "linux")
	include_directories(AFTER SYSTEM
		"$ENV{JAVA_HOME}/include/linux"
	)
	add_definitions(-DSK_BUILD_FOR_UNIX)
endif()
add_definitions(
	-DGOOGLE_PROTOBUF_NO_RTTI
	-DSK_ALLOW_STATIC_GLOBAL_INITIALIZERS=0
	-DSK_RELEASE
	-DSK_CPU_LENDIAN
	-DGR_RELEASE=1
	-DXML_STATIC
)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()
set(pd_sources "")
if(CMAKE_TARGET_OS STREQUAL "linux")
elseif(CMAKE_TARGET_OS STREQUAL "darwin")
elseif(CMAKE_TARGET_OS STREQUAL "windows")
endif()
add_library(OsmAndCore SHARED
	"src/Logging.cpp"
	"native/src/ElapsedTimer.cpp"
	"native/src/ExplicitSkiaReferences.cpp"
	"native/src/common.cpp"
	"native/src/mapObjects.cpp"
	"native/src/multipolygons.cpp"
	"native/src/renderRules.cpp"
	"native/src/rendering.cpp"
	"native/src/binaryRead.cpp"
	"native/src/binaryRoutePlanner.cpp"
	"native/src/proto/osmand_index.pb.cc"
	"native/src/java_wrap.cpp"
	${pd_sources}
)

target_link_libraries(OsmAndCore LINK_PUBLIC
	skia_osmand
	protobuf_osmand
)
