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
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc")
	set(USE_WHOLE_LIBRARIES "-Wl,-whole-archive")
	set(USE_WHOLE_LIBRARY "")
	set(DONT_USE_WHOLE_LIBRARIES "-Wl,-no-whole-archive")
else()
	set(USE_WHOLE_LIBRARIES "")
	set(USE_WHOLE_LIBRARY "")
	set(DONT_USE_WHOLE_LIBRARIES "")
endif()

if((CMAKE_TARGET_OS STREQUAL "linux") OR (CMAKE_TARGET_OS STREQUAL "darwin"))
	target_link_libraries(png LINK_PRIVATE
	${USE_WHOLE_LIBRARIES}
		${USE_WHOLE_LIBRARY} z
	${DONT_USE_WHOLE_LIBRARIES}
	)
endif()