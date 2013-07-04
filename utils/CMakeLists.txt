project(OsmAndCoreUtils)

file(GLOB_RECURSE includes "*.h*")
file(GLOB_RECURSE sources "*.c*")
file(GLOB_RECURSE headers "*.h*")
add_library(OsmAndCoreUtils SHARED
	${includes}
	${sources}
	${headers}
)
add_dependencies(OsmAndCoreUtils
	OsmAndCore
)
target_compile_definitions(OsmAndCoreUtils
	PRIVATE
		-DOSMAND_CORE_UTILS_EXPORTS
)
target_include_directories(OsmAndCoreUtils
	PUBLIC
		"${OSMAND_ROOT}/core/utils"
)
target_link_libraries(OsmAndCoreUtils
	LINK_PUBLIC
		OsmAndCore
)
