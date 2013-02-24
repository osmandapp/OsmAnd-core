project(OsmAndCoreUtils)
include_directories(AFTER SYSTEM
	"${OSMAND_ROOT}/core/externals/protobuf/upstream.patched/src"
	"${OSMAND_ROOT}/core/include"
	"${OSMAND_ROOT}/core/protos"
	"${OSMAND_ROOT}/core/native/include"
)
add_definitions(
	-DOSMAND_INSPECTOR_EXPORTS
)
file(GLOB_RECURSE includes "${OSMAND_ROOT}/core/include/*.h*")
file(GLOB_RECURSE utils_includes "*.h*")
file(GLOB_RECURSE utils_sources "*.c*")
add_library(OsmAndCoreUtils SHARED
	${includes}
	${utils_includes}
	${utils_sources}
)
target_link_libraries(OsmAndCoreUtils OsmAndCore)
find_package(Qt5Core REQUIRED)
qt5_use_modules(OsmAndCoreUtils Core)
