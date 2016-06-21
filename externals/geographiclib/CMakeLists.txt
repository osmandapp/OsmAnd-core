project(geographiclib)

prepare_upstream()

#include("upstream.patched/CMakeLists.txt")

add_library(geographiclib_static STATIC IMPORTED GLOBAL)
set_target_properties(geographiclib_static PROPERTIES IMPORTED_LOCATION ${OSMAND_ROOT}/core/externals/geographiclib/build/src/libGeographic.a) # FIXME

add_library(geographiclib_headers_only INTERFACE)
target_include_directories(geographiclib_headers_only
	INTERFACE
                "upstream.patched/include"
)
