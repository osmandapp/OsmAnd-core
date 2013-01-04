project(osmand)

set(ROOT "${OSMAND_ROOT}/native")
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
	add_definitions(-DSK_BUILD_FOR_MAC)
elseif(CMAKE_TARGET_OS STREQUAL "linux")
	add_definitions(-DSK_BUILD_FOR_UNIX)
endif()
add_definitions(
	-DGOOGLE_PROTOBUF_NO_RTTI
	-DSK_ALLOW_STATIC_GLOBAL_INITIALIZERS=0
	-DSK_RELEASE
	-DSK_CPU_LENDIAN
	-DGR_RELEASE=1
	-DXML_STATIC)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()
add_library(osmand SHARED
    "${ROOT}/src/ElapsedTimer.cpp"
	"${ROOT}/src/common.cpp"
	"${ROOT}/src/mapObjects.cpp"
	"${ROOT}/src/multipolygons.cpp"
	"${ROOT}/src/renderRules.cpp"
	"${ROOT}/src/rendering.cpp"
	"${ROOT}/src/binaryRead.cpp"
	"${ROOT}/src/binaryRoutePlanner.cpp"
	"${ROOT}/src/proto/osmand_index.pb.cc"
	"${ROOT}/src/java_wrap.cpp"
)
target_link_libraries(osmand
	protobuf
	z
	png
	gif
	jpeg
	expat
	freetype2
	harfbuzz
	skia
)
if(CMAKE_TARGET_OS STREQUAL "windows")
	target_link_libraries(osmand
		Winmm
		Usp10
	)
endif()
# http://stackoverflow.com/questions/5693405/specifying-link-flags-for-only-one-static-lib-while-linking-executable