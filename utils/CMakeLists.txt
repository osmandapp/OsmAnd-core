project(OsmAndCoreUtils)
include_directories(AFTER SYSTEM
	"${OSMAND_ROOT}/core/externals/skia/upstream.patched/include/core"
	"${OSMAND_ROOT}/core/externals/skia/upstream.patched/include/images"
	"${OSMAND_ROOT}/core/externals/skia/upstream.patched/include/utils"
	"${OSMAND_ROOT}/core/externals/skia/upstream.patched/include/config"
	"${OSMAND_ROOT}/core/externals/skia/upstream.patched/include/effects"
	"${OSMAND_ROOT}/core/externals/skia/upstream.patched/src/core"
	"${OSMAND_ROOT}/core/externals/protobuf/upstream.patched/src"
	"${OSMAND_ROOT}/core/include"
	"${OSMAND_ROOT}/core/protos"
	"${OSMAND_ROOT}/core/native/include"
)
add_definitions(
	-DGOOGLE_PROTOBUF_NO_RTTI
	-DSK_ALLOW_STATIC_GLOBAL_INITIALIZERS=0
	-DSK_RELEASE
	-DSK_CPU_LENDIAN
	-DGR_RELEASE=1
	-DXML_STATIC
	-DOSMAND_CORE_UTILS_EXPORTS
)
file(GLOB_RECURSE includes "*.h*")
file(GLOB_RECURSE sources "*.c*")
file(GLOB_RECURSE headers "*.h*")
add_library(OsmAndCoreUtils SHARED
	${includes}
	${sources}
	${headers}
)
add_dependencies(OsmAndCoreUtils OsmAndCore)
target_link_libraries(OsmAndCoreUtils OsmAndCore)
find_package(Qt5Core REQUIRED)
qt5_use_modules(OsmAndCoreUtils Core)
