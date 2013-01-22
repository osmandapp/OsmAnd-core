project(expat_osmand)

set(ROOT "${OSMAND_ROOT}/externals/expat")
set(UPSTREAM "${ROOT}/upstream.patched")

include_directories(AFTER SYSTEM
	"${ROOT}"
)
add_definitions(-DHAVE_EXPAT_CONFIG_H)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

add_library(expat_osmand STATIC
	"${UPSTREAM}/lib/xmlparse.c"
	"${UPSTREAM}/lib/xmlrole.c"
	"${UPSTREAM}/lib/xmltok.c"
	"${UPSTREAM}/lib/xmltok_impl.c"
	"${UPSTREAM}/lib/xmltok_ns.c"
)
