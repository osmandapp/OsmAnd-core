project(z_osmand)

add_definitions(-D_LARGEFILE64_SOURCE=1 -DHAVE_SYS_TYPES_H -DHAVE_STDINT_H -DHAVE_STDDEF_H -DZ_PREFIX)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()
add_library(z_osmand STATIC
	"upstream.patched/adler32.c"
	"upstream.patched/compress.c"
	"upstream.patched/crc32.c"
	"upstream.patched/deflate.c"
	"upstream.patched/gzclose.c"
	"upstream.patched/gzlib.c"
	"upstream.patched/gzread.c"
	"upstream.patched/gzwrite.c"
	"upstream.patched/infback.c"
	"upstream.patched/inffast.c"
	"upstream.patched/inflate.c"
	"upstream.patched/inftrees.c"
	"upstream.patched/trees.c"
	"upstream.patched/uncompr.c"
	"upstream.patched/zutil.c"
)
