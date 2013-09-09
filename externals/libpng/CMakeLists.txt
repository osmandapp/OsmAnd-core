project(png)

set(target_specific_sources "")
if((CMAKE_TARGET_CPU_ARCH STREQUAL "arm") OR (CMAKE_TARGET_OS STREQUAL "ios" AND CMAKE_TARGET_CPU_ARCH STREQUAL "device"))
	enable_language(ASM)
	set(target_specific_sources ${target_specific_sources}
		"upstream.patched/arm/arm_init.c"
		"upstream.patched/arm/filter_neon.S"
	)
endif()

add_library(png_static STATIC
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
	${target_specific_sources}
)

add_dependencies(png_static
	z_static
)

target_compile_definitions(png_static
	PRIVATE
		-DPNG_CONFIGURE_LIBPNG
)
target_include_directories(png_static
	PUBLIC
		"upstream.patched"
)
target_link_libraries(png_static
	LINK_PUBLIC
		z_static
)
