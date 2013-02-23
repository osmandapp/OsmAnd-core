project(OsmAndCore)

include_directories(AFTER SYSTEM
	"${OSMAND_ROOT}/core/externals/skia/upstream.patched/include/core"
	"${OSMAND_ROOT}/core/externals/skia/upstream.patched/include/images"
	"${OSMAND_ROOT}/core/externals/skia/upstream.patched/include/utils"
	"${OSMAND_ROOT}/core/externals/skia/upstream.patched/include/config"
	"${OSMAND_ROOT}/core/externals/skia/upstream.patched/include/effects"
	"${OSMAND_ROOT}/core/externals/skia/upstream.patched/include/utils/android"
	"${OSMAND_ROOT}/core/externals/skia/upstream.patched/src/core"
	"${OSMAND_ROOT}/core/externals/expat/upstream.patched/lib"
	"${OSMAND_ROOT}/core/externals/freetype/upstream.patched/include"
	"${OSMAND_ROOT}/core/externals/giflib/upstream.patched/lib"
	"${OSMAND_ROOT}/core/externals/jpeg/upstream.patched"
	"${OSMAND_ROOT}/core/externals/libpng/upstream.patched"
	"${OSMAND_ROOT}/core/externals/harfbuzz/upstream.patched/src"
	"${OSMAND_ROOT}/core/externals/protobuf/upstream.patched/src"
	"${OSMAND_ROOT}/core/include"
	"${OSMAND_ROOT}/core/src"
	"${OSMAND_ROOT}/core/protos"
	"${OSMAND_ROOT}/core/native/include"
	"${OSMAND_ROOT}/core/native/src"
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
	-DOSMAND_CORE_EXPORTS
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

file(GLOB_RECURSE sources "src/*.c*")
file(GLOB_RECURSE includes "include/*.h*")
add_library(OsmAndCore SHARED
	${includes}
	${sources}
	${pd_sources}
	"protos/OBF.pb.h"

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
)

target_link_libraries(OsmAndCore LINK_PUBLIC
	skia_osmand
	protobuf_osmand
)
find_package(Qt5Core REQUIRED)
qt5_use_modules(OsmAndCore Core)
