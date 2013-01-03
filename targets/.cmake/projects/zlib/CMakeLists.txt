project(z)

set(ROOT "${OSMAND_ROOT}/externals/zlib")
set(UPSTREAM "${ROOT}/upstream.patched")
add_definitions(-D_LARGEFILE64_SOURCE=1 -DHAVE_SYS_TYPES_H -DHAVE_STDINT_H -DHAVE_STDDEF_H -DZ_PREFIX)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()
add_library(z STATIC
    "${UPSTREAM}/adler32.c"
    "${UPSTREAM}/compress.c"
    "${UPSTREAM}/crc32.c"
    "${UPSTREAM}/deflate.c"
    "${UPSTREAM}/gzclose.c"
    "${UPSTREAM}/gzlib.c"
    "${UPSTREAM}/gzread.c"
    "${UPSTREAM}/gzwrite.c"
    "${UPSTREAM}/infback.c"
    "${UPSTREAM}/inffast.c"
    "${UPSTREAM}/inflate.c"
    "${UPSTREAM}/inftrees.c"
    "${UPSTREAM}/trees.c"
    "${UPSTREAM}/uncompr.c"
    "${UPSTREAM}/zutil.c"
)