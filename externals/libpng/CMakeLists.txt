project(png)

prepare_upstream()

set(target_specific_sources "")
set(target_specific_public_definitions "")
set(target_specific_private_definitions "")

#TODO: filter_neon.S needs to be patched to support neon
if (CMAKE_TARGET_OS STREQUAL "ios")
	add_definitions(-DPNG_ARM_NEON_OPT=0)
endif()
if (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "arm" OR CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "arm64" OR
	(CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "fat" AND CMAKE_TARGET_OS STREQUAL "ios"))
	set(target_specific_sources ${target_specific_sources}
		"upstream.patched/arm/arm_init.c"
		"upstream.patched/arm/filter_neon_intrinsics.c"
		"upstream.patched/arm/filter_neon.S"
	)
endif()

if (CMAKE_TARGET_OS STREQUAL "windows")
	if (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
		set(target_specific_private_definitions ${target_specific_private_definitions}
			-D_CRT_SECURE_NO_WARNINGS
		)
	endif()
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
		${target_specific_private_definitions}
	PUBLIC
		-DPNG_PREFIX
		${target_specific_public_definitions}
)
target_include_directories(png_static
	PUBLIC
		"."
		"upstream.patched"
)
target_link_libraries(png_static
	LINK_PUBLIC
		z_static
)

if (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "fat" AND CMAKE_TARGET_OS STREQUAL "ios")
	set(ios_iphonesimulator_exclude_list
		"arm_init.c"
		"filter_neon_intrinsics.c"
		"filter_neon.S")
	string(REPLACE ";" " " ios_iphonesimulator_exclude "${ios_iphonesimulator_exclude_list}")
	set_target_properties(png_static
		PROPERTIES
			XCODE_ATTRIBUTE_EXCLUDED_SOURCE_FILE_NAMES[sdk=iphonesimulator*][arch=*] "${ios_iphonesimulator_exclude}"
	)
endif()
