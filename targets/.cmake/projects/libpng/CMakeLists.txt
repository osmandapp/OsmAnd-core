project(png)

set(ROOT "${OSMAND_ROOT}/externals/libpng")
set(UPSTREAM "${ROOT}/upstream.patched")
include_directories(AFTER SYSTEM
	"${UPSTREAM}"
	"${OSMAND_ROOT}/externals/zlib/upstream.patched"
)
add_definitions(-DPNG_CONFIGURE_LIBPNG)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()
add_library(png STATIC
	"${UPSTREAM}/png.c"
	"${UPSTREAM}/pngerror.c"
	"${UPSTREAM}/pngget.c"
	"${UPSTREAM}/pngmem.c"
	"${UPSTREAM}/pngpread.c"
	"${UPSTREAM}/pngread.c"
	"${UPSTREAM}/pngrio.c"
	"${UPSTREAM}/pngrtran.c"
	"${UPSTREAM}/pngrutil.c"
	"${UPSTREAM}/pngset.c"
	"${UPSTREAM}/pngtrans.c"
	"${UPSTREAM}/pngwio.c"
	"${UPSTREAM}/pngwrite.c"
	"${UPSTREAM}/pngwtran.c"
	"${UPSTREAM}/pngwutil.c"
)