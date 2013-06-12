project(OsmAndCore)

set(pd_includes_private "")
if(CMAKE_TARGET_OS STREQUAL "windows")
	set(pd_includes_private ${pd_includes_private}
		"$ENV{JAVA_HOME}/include/win32"
	)
elseif(CMAKE_TARGET_OS STREQUAL "darwin")
	set(pd_includes_private ${pd_includes_private}
		"$ENV{JAVA_HOME}/include/darwin"
	)
elseif(CMAKE_TARGET_OS STREQUAL "linux")
	set(pd_includes_private ${pd_includes_private}
		"$ENV{JAVA_HOME}/include/linux"
	)
endif()

set(pd_sources "")
if(CMAKE_TARGET_OS STREQUAL "linux")
elseif(CMAKE_TARGET_OS STREQUAL "darwin")
elseif(CMAKE_TARGET_OS STREQUAL "windows")
endif()

file(GLOB_RECURSE sources "src/*.c*")
file(GLOB_RECURSE headers "src/*.h*")
file(GLOB_RECURSE includes "include/*.h*")
file(GLOB_RECURSE sources_client "client/*.c*")
file(GLOB_RECURSE headers_client "client/*.h*")
add_library(OsmAndCore SHARED
	${includes}
	${headers}
    ${sources}
    ${sources_client}
    ${headers_client}
	${pd_sources}
	"protos/OBF.pb.h"

	"native/src/ElapsedTimer.cpp"
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

add_dependencies(OsmAndCore
	skia_static
	protobuf_static
)

target_compile_definitions(OsmAndCore
	PRIVATE
		-DOSMAND_CORE_EXPORTS
)
target_include_directories(OsmAndCore
	PRIVATE
		"${OSMAND_ROOT}/core/native/include"
		"${OSMAND_ROOT}/core/native/src"
		"$ENV{JAVA_HOME}/include"
		${pd_includes_private}
	PUBLIC
		"${OSMAND_ROOT}/core/include"
		"${OSMAND_ROOT}/core/src"
		"${OSMAND_ROOT}/core/client"
		"${OSMAND_ROOT}/core/protos"
)
target_link_libraries(OsmAndCore
	LINK_PUBLIC
		skia_static
		protobuf_static
)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
qt5_use_modules(OsmAndCore Core Network)
