find_package(Qt5Core REQUIRED)

project(OsmAndInspector)
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
file(GLOB_RECURSE inspector "inspector.*")
add_library(OsmAndInspector SHARED
	${includes}
	${inspector}
)
target_link_libraries(OsmAndInspector OsmAndCore)
qt5_use_modules(OsmAndInspector Core)