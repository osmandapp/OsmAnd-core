project(expat_osmand)

include_directories(AFTER SYSTEM
	"."
)
add_definitions(-DHAVE_EXPAT_CONFIG_H)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

add_library(expat_osmand STATIC
	"upstream.patched/lib/xmlparse.c"
	"upstream.patched/lib/xmlrole.c"
	"upstream.patched/lib/xmltok.c"
	"upstream.patched/lib/xmltok_impl.c"
	"upstream.patched/lib/xmltok_ns.c"
)
