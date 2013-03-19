project(OsmAndCoreUtils)
include_directories(AFTER SYSTEM
	"${OSMAND_ROOT}/core/externals/protobuf/upstream.patched/src"
	"${OSMAND_ROOT}/core/include"
	"${OSMAND_ROOT}/core/protos"
	"${OSMAND_ROOT}/core/native/include"
)
add_definitions(
	-DOSMAND_CORE_UTILS_EXPORTS
)
file(GLOB_RECURSE includes "*.h*")
file(GLOB_RECURSE sources "*.c*")
add_library(OsmAndCoreUtils SHARED
	${includes}
	${sources}
)
target_link_libraries(OsmAndCoreUtils OsmAndCore)
find_package(Qt5Core REQUIRED)
qt5_use_modules(OsmAndCoreUtils Core)
