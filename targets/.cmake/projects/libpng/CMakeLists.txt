project(png_osmand)

set(ROOT "${OSMAND_ROOT}/externals/skia/upstream.patched/third_party/libpng")
set(UPSTREAM "${ROOT}")
include_directories(AFTER SYSTEM
	"${UPSTREAM}"
	"${OSMAND_ROOT}/externals/skia/upstream.patched/third_party/externals/zlib"
)
add_definitions(-DPNG_CONFIGURE_LIBPNG -DZ_PREFIX)

set(target_arch_specific_sources "")

if(CMAKE_TARGET_CPU_ARCH STREQUAL "intel")
        set(target_arch_specific_sources "${target_arch_specific_sources}"
        	"${UPSTREAM}/contrib/intel/filter_sse2_intrinsics.c"
        	"${UPSTREAM}/contrib/intel/intel_init.c"
        )
        add_definitions(-DPNG_INTEL_SSE)
endif()
if(CMAKE_TARGET_CPU_ARCH STREQUAL "arm")
        set(target_arch_specific_sources "${target_arch_specific_sources}"
			"${UPSTREAM}/arm/arm_init.c"
        	"${UPSTREAM}/arm/filter_neon_intrinsics.c"
        )
endif()
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()
add_library(png_osmand STATIC
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

	${target_arch_specific_sources}
)

target_link_libraries(png_osmand LINK_PUBLIC
	z_osmand
)
