project(freetype2)

prepare_upstream()

set(target_specific_sources "")
if (CMAKE_TARGET_OS STREQUAL "macosx" OR CMAKE_TARGET_OS STREQUAL "ios" OR CMAKE_TARGET_OS STREQUAL "linux" OR CMAKE_TARGET_OS STREQUAL "android")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/builds/unix/ftsystem.c"
	)
elseif (CMAKE_TARGET_OS STREQUAL "windows")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/builds/windows/ftsystem.c"
	)
else ()
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/base/ftsystem.c"
	)
endif ()

add_library(freetype2_static STATIC
	"upstream.patched/src/autofit/autofit.c"
	"upstream.patched/src/base/ftbase.c"
	"upstream.patched/src/base/ftbbox.c"
	"upstream.patched/src/base/ftbdf.c"
	"upstream.patched/src/base/ftbitmap.c"
	"upstream.patched/src/base/ftcid.c"
	"upstream.patched/src/base/ftfstype.c"
	"upstream.patched/src/base/ftgasp.c"
	"upstream.patched/src/base/ftglyph.c"
	"upstream.patched/src/base/ftgxval.c"
	"upstream.patched/src/base/ftinit.c"
	"upstream.patched/src/base/ftmm.c"
	"upstream.patched/src/base/ftotval.c"
	"upstream.patched/src/base/ftpatent.c"
	"upstream.patched/src/base/ftpfr.c"
	"upstream.patched/src/base/ftstroke.c"
	"upstream.patched/src/base/ftsynth.c"
	"upstream.patched/src/base/fttype1.c"
	"upstream.patched/src/base/ftwinfnt.c"
	"upstream.patched/src/bdf/bdf.c"
	"upstream.patched/src/bzip2/ftbzip2.c"
	"upstream.patched/src/cache/ftcache.c"
	"upstream.patched/src/cff/cff.c"
	"upstream.patched/src/cid/type1cid.c"
	"upstream.patched/src/gzip/ftgzip.c"
	"upstream.patched/src/lzw/ftlzw.c"
	"upstream.patched/src/pcf/pcf.c"
	"upstream.patched/src/pfr/pfr.c"
	"upstream.patched/src/psaux/psaux.c"
	"upstream.patched/src/pshinter/pshinter.c"
	"upstream.patched/src/psnames/psnames.c"
	"upstream.patched/src/raster/raster.c"
	"upstream.patched/src/sdf/sdf.c"
	"upstream.patched/src/sfnt/sfnt.c"
	"upstream.patched/src/smooth/smooth.c"
	"upstream.patched/src/truetype/truetype.c"
	"upstream.patched/src/type1/type1.c"
	"upstream.patched/src/type42/type42.c"
	"upstream.patched/src/winfonts/winfnt.c"
	"upstream.patched/src/base/ftdebug.c"
	${target_specific_sources}
)
add_dependencies(freetype2_static
	z_static
)

check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("fcntl.h" HAVE_FCNTL_H)
configure_file("ftconfig.h.in" "ftconfig-override.h")

target_compile_definitions(freetype2_static
	PRIVATE
		-DFT2_BUILD_LIBRARY
		-DFT_CONFIG_MODULES_H=<ftmodule-override.h>
		-DFT_CONFIG_OPTIONS_H=<ftoption-override.h>
		-DFT_CONFIG_CONFIG_H=<ftconfig-override.h>
)
target_include_directories(freetype2_static
	PRIVATE
		"."
		"${CMAKE_CURRENT_BINARY_DIR}"
	PUBLIC
		"upstream.patched/include"
)
target_link_libraries(freetype2_static
	LINK_PUBLIC
		z_static
)
