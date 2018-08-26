project(z_osmand)

set(ROOT "${OSMAND_ROOT}/externals/skia/upstream.patched/third_party/externals/zlib")
set(UPSTREAM "${ROOT}")
add_definitions(-D_LARGEFILE64_SOURCE=1 -DHAVE_SYS_TYPES_H -DHAVE_STDINT_H -DHAVE_STDDEF_H -DZ_PREFIX)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()
set(target_arch_specific_sources "")
if(CMAKE_TARGET_CPU_ARCH STREQUAL "intel")
        set(target_arch_specific_sources "${target_arch_specific_sources}"
	      	"${UPSTREAM}/crc_folding.c"
    	  	"${UPSTREAM}/fill_window_sse.c"
    	  	"${UPSTREAM}/x86.c"
        )
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.2 -mpclmul")
else()
        set(target_arch_specific_sources "${target_arch_specific_sources}"
	      	"${UPSTREAM}/simd_stub.c"
        )
endif()

add_library(z_osmand STATIC
	"${UPSTREAM}/adler32.c"
    "${UPSTREAM}/compress.c"
    "${UPSTREAM}/crc32.c"
    "${UPSTREAM}/crc32.h"
    "${UPSTREAM}/deflate.c"
    "${UPSTREAM}/deflate.h"
    "${UPSTREAM}/gzclose.c"
    "${UPSTREAM}/gzguts.h"
    "${UPSTREAM}/gzlib.c"
    "${UPSTREAM}/gzread.c"
    "${UPSTREAM}/gzwrite.c"
    "${UPSTREAM}/infback.c"
    "${UPSTREAM}/inffast.c"
    "${UPSTREAM}/inffast.h"
    "${UPSTREAM}/inffixed.h"
    "${UPSTREAM}/inflate.c"
    "${UPSTREAM}/inflate.h"
    "${UPSTREAM}/inftrees.c"
    "${UPSTREAM}/inftrees.h"
    #"${UPSTREAM}/mozzconf.h"
    "${UPSTREAM}/trees.c"
    "${UPSTREAM}/trees.h"
    "${UPSTREAM}/uncompr.c"
    "${UPSTREAM}/x86.h"
    "${UPSTREAM}/zconf.h"
    "${UPSTREAM}/zlib.h"
    "${UPSTREAM}/zutil.c"
    "${UPSTREAM}/zutil.h"

    ${target_arch_specific_sources}

	#"${UPSTREAM}/adler32.c"
#	"${UPSTREAM}/compress.c"
#	"${UPSTREAM}/crc32.c"
#	"${UPSTREAM}/deflate.c"
#	"${UPSTREAM}/gzclose.c"
#	"${UPSTREAM}/gzlib.c"
#	"${UPSTREAM}/gzread.c"
#	"${UPSTREAM}/gzwrite.c"
#	"${UPSTREAM}/infback.c"
#	"${UPSTREAM}/inffast.c"
#	"${UPSTREAM}/inflate.c"
#	"${UPSTREAM}/inftrees.c"
#	"${UPSTREAM}/trees.c"
#	"${UPSTREAM}/uncompr.c"
#	"${UPSTREAM}/zutil.c"
)
