project(png)

include_directories(AFTER SYSTEM
	"upstream.patched"
	"${OSMAND_ROOT}/core/externals/zlib/upstream.patched"
)
add_definitions(-DPNG_CONFIGURE_LIBPNG -DZ_PREFIX)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()
add_library(png_osmand STATIC
	"upstream.patched/png.c"
	"upstream.patched/pngerror.c"
	"upstream.patched/pngget.c"
	"upstream.patched/pngmem.c"
	"upstream.patched/pngpread.c"
	"upstream.patched/pngread.c"
	"upstream.patched/pngrio.c"
	"upstream.patched/pngrtran.c"
	"upstream.patched/pngrutil.c"
	"upstream.patched/pngset.c"
	"upstream.patched/pngtrans.c"
	"upstream.patched/pngwio.c"
	"upstream.patched/pngwrite.c"
	"upstream.patched/pngwtran.c"
	"upstream.patched/pngwutil.c"
)

target_link_libraries(png_osmand LINK_PUBLIC
	z_osmand
)
