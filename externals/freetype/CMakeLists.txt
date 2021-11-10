project(freetype2)

prepare_upstream()

add_library(freetype2_static STATIC
	"upstream.patched/src/base/ftbase.c"
	"upstream.patched/src/base/ftbbox.c"
	"upstream.patched/src/base/ftbitmap.c"
	"upstream.patched/src/base/ftfstype.c"
	"upstream.patched/src/base/ftgasp.c"
	"upstream.patched/src/base/ftglyph.c"
	"upstream.patched/src/base/ftinit.c"
	"upstream.patched/src/base/ftlcdfil.c"
	"upstream.patched/src/base/ftstroke.c"
	"upstream.patched/src/base/ftsystem.c"
	"upstream.patched/src/base/fttype1.c"
	"upstream.patched/src/base/ftxf86.c"
	"upstream.patched/src/autofit/autofit.c"
	"upstream.patched/src/bdf/bdf.c"
	"upstream.patched/src/cff/cff.c"
	"upstream.patched/src/pshinter/pshinter.c"
	"upstream.patched/src/psnames/psnames.c"
	"upstream.patched/src/raster/raster.c"
	"upstream.patched/src/sfnt/sfnt.c"
	"upstream.patched/src/smooth/smooth.c"
	"upstream.patched/src/truetype/truetype.c"
)

target_compile_definitions(freetype2_static
	PRIVATE
		-DFT2_BUILD_LIBRARY
		-DFT_CONFIG_MODULES_H=<ftmodule-override.h>
)
target_include_directories(freetype2_static
	PRIVATE
		"."
	PUBLIC
		"upstream.patched/include"
)
