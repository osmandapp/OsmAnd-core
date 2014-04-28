project(OsmAndCoreUtils)

# Bump this number each time a new source file is committed to repository, source file removed from repository or renamed: 1

file(GLOB_RECURSE sources "*.c*")
file(GLOB_RECURSE headers "*.h*")

set(merged_sources
	${sources}
	${headers}
)
set(merged_include_dirs_public
	"${OSMAND_ROOT}/core/utils"
)

if (CMAKE_SHARED_LIBS_ALLOWED_ON_TARGET)
	add_library(OsmAndCoreUtils_shared SHARED ${merged_sources})
	add_dependencies(OsmAndCoreUtils_shared OsmAndCore_shared)
	target_compile_definitions(OsmAndCoreUtils_shared
		PRIVATE
			-DOSMAND_CORE_UTILS_EXPORTS
	)
	target_include_directories(OsmAndCoreUtils_shared
		PUBLIC
			${merged_include_dirs_public}
	)
	target_link_libraries(OsmAndCoreUtils_shared
		LINK_PUBLIC
			OsmAndCore_shared
	)
endif ()

if (CMAKE_STATIC_LIBS_ALLOWED_ON_TARGET)
	add_library(OsmAndCoreUtils_static STATIC ${merged_sources})
	add_dependencies(OsmAndCoreUtils_static OsmAndCore_static)
	target_compile_definitions(OsmAndCoreUtils_static
		PUBLIC
			-DOSMAND_CORE_UTILS_STATIC
	)
	target_include_directories(OsmAndCoreUtils_static
		PUBLIC
			${merged_include_dirs_public}
	)
	target_link_libraries(OsmAndCoreUtils_static
		LINK_PUBLIC
			OsmAndCore_static
	)
endif ()
