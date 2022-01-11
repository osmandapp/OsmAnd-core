project(harfbuzz)

prepare_upstream()

set(common_sources
	"upstream.patched/src/hb-aat-layout-ankr-table.hh"
	"upstream.patched/src/hb-aat-layout-bsln-table.hh"
	"upstream.patched/src/hb-aat-layout-common.hh"
	"upstream.patched/src/hb-aat-layout-feat-table.hh"
	"upstream.patched/src/hb-aat-layout-just-table.hh"
	"upstream.patched/src/hb-aat-layout-kerx-table.hh"
	"upstream.patched/src/hb-aat-layout-morx-table.hh"
	"upstream.patched/src/hb-aat-layout-opbd-table.hh"
	"upstream.patched/src/hb-aat-layout-trak-table.hh"
	"upstream.patched/src/hb-aat-layout.cc"
	"upstream.patched/src/hb-aat-layout.hh"
	"upstream.patched/src/hb-aat-ltag-table.hh"
	"upstream.patched/src/hb-aat-map.cc"
	"upstream.patched/src/hb-aat-map.hh"
	"upstream.patched/src/hb-algs.hh"
	"upstream.patched/src/hb-array.hh"
	"upstream.patched/src/hb-atomic.hh"
	"upstream.patched/src/hb-bimap.hh"
	"upstream.patched/src/hb-bit-page.hh"
	"upstream.patched/src/hb-blob.cc"
	"upstream.patched/src/hb-blob.hh"
	"upstream.patched/src/hb-buffer-serialize.cc"
	"upstream.patched/src/hb-buffer.cc"
	"upstream.patched/src/hb-buffer.hh"
	"upstream.patched/src/hb-cache.hh"
	"upstream.patched/src/hb-cff-interp-common.hh"
	"upstream.patched/src/hb-cff-interp-cs-common.hh"
	"upstream.patched/src/hb-cff-interp-dict-common.hh"
	"upstream.patched/src/hb-cff1-interp-cs.hh"
	"upstream.patched/src/hb-cff2-interp-cs.hh"
	"upstream.patched/src/hb-common.cc"
	"upstream.patched/src/hb-config.hh"
	"upstream.patched/src/hb-debug.hh"
	"upstream.patched/src/hb-dispatch.hh"
	"upstream.patched/src/hb-draw.cc"
	"upstream.patched/src/hb-draw.hh"
	"upstream.patched/src/hb-face.cc"
	"upstream.patched/src/hb-face.hh"
	"upstream.patched/src/hb-fallback-shape.cc"
	"upstream.patched/src/hb-font.cc"
	"upstream.patched/src/hb-font.hh"
	"upstream.patched/src/hb-iter.hh"
	"upstream.patched/src/hb-kern.hh"
	"upstream.patched/src/hb-machinery.hh"
	"upstream.patched/src/hb-map.cc"
	"upstream.patched/src/hb-map.hh"
	"upstream.patched/src/hb-meta.hh"
	"upstream.patched/src/hb-ms-feature-ranges.cc"
	"upstream.patched/src/hb-ms-feature-ranges.hh"
	"upstream.patched/src/hb-mutex.hh"
	"upstream.patched/src/hb-null.hh"
	"upstream.patched/src/hb-number.cc"
	"upstream.patched/src/hb-number.hh"
	"upstream.patched/src/hb-object.hh"
	"upstream.patched/src/hb-open-file.hh"
	"upstream.patched/src/hb-open-type.hh"
	"upstream.patched/src/hb-ot-cff-common.hh"
	"upstream.patched/src/hb-ot-cff1-std-str.hh"
	"upstream.patched/src/hb-ot-cff1-table.cc"
	"upstream.patched/src/hb-ot-cff1-table.hh"
	"upstream.patched/src/hb-ot-cff2-table.cc"
	"upstream.patched/src/hb-ot-cff2-table.hh"
	"upstream.patched/src/hb-ot-cmap-table.hh"
	"upstream.patched/src/hb-ot-color-cbdt-table.hh"
	"upstream.patched/src/hb-ot-color-colr-table.hh"
	"upstream.patched/src/hb-ot-color-cpal-table.hh"
	"upstream.patched/src/hb-ot-color-sbix-table.hh"
	"upstream.patched/src/hb-ot-color-svg-table.hh"
	"upstream.patched/src/hb-ot-color.cc"
	"upstream.patched/src/hb-ot-face-table-list.hh"
	"upstream.patched/src/hb-ot-face.cc"
	"upstream.patched/src/hb-ot-face.hh"
	"upstream.patched/src/hb-ot-font.cc"
	"upstream.patched/src/hb-ot-gasp-table.hh"
	"upstream.patched/src/hb-ot-glyf-table.hh"
	"upstream.patched/src/hb-ot-hdmx-table.hh"
	"upstream.patched/src/hb-ot-head-table.hh"
	"upstream.patched/src/hb-ot-hhea-table.hh"
	"upstream.patched/src/hb-ot-hmtx-table.hh"
	"upstream.patched/src/hb-ot-kern-table.hh"
	"upstream.patched/src/hb-ot-layout-base-table.hh"
	"upstream.patched/src/hb-ot-layout-common.hh"
	"upstream.patched/src/hb-ot-layout-gdef-table.hh"
	"upstream.patched/src/hb-ot-layout-gpos-table.hh"
	"upstream.patched/src/hb-ot-layout-gsub-table.hh"
	"upstream.patched/src/hb-ot-layout-gsubgpos.hh"
	"upstream.patched/src/hb-ot-layout-jstf-table.hh"
	"upstream.patched/src/hb-ot-layout.cc"
	"upstream.patched/src/hb-ot-layout.hh"
	"upstream.patched/src/hb-ot-map.cc"
	"upstream.patched/src/hb-ot-map.hh"
	"upstream.patched/src/hb-ot-math-table.hh"
	"upstream.patched/src/hb-ot-math.cc"
	"upstream.patched/src/hb-ot-maxp-table.hh"
	"upstream.patched/src/hb-ot-meta-table.hh"
	"upstream.patched/src/hb-ot-meta.cc"
	"upstream.patched/src/hb-ot-metrics.cc"
	"upstream.patched/src/hb-ot-metrics.hh"
	"upstream.patched/src/hb-ot-name-language-static.hh"
	"upstream.patched/src/hb-ot-name-language.hh"
	"upstream.patched/src/hb-ot-name-table.hh"
	"upstream.patched/src/hb-ot-name.cc"
	"upstream.patched/src/hb-ot-os2-table.hh"
	"upstream.patched/src/hb-ot-os2-unicode-ranges.hh"
	"upstream.patched/src/hb-ot-post-macroman.hh"
	"upstream.patched/src/hb-ot-post-table.hh"
	"upstream.patched/src/hb-ot-shape-complex-arabic-fallback.hh"
	"upstream.patched/src/hb-ot-shape-complex-arabic-joining-list.hh"
	"upstream.patched/src/hb-ot-shape-complex-arabic-table.hh"
	"upstream.patched/src/hb-ot-shape-complex-arabic-win1256.hh"
	"upstream.patched/src/hb-ot-shape-complex-arabic.cc"
	"upstream.patched/src/hb-ot-shape-complex-arabic.hh"
	"upstream.patched/src/hb-ot-shape-complex-default.cc"
	"upstream.patched/src/hb-ot-shape-complex-hangul.cc"
	"upstream.patched/src/hb-ot-shape-complex-hebrew.cc"
	"upstream.patched/src/hb-ot-shape-complex-indic-table.cc"
	"upstream.patched/src/hb-ot-shape-complex-indic.cc"
	"upstream.patched/src/hb-ot-shape-complex-indic.hh"
	"upstream.patched/src/hb-ot-shape-complex-khmer.cc"
	"upstream.patched/src/hb-ot-shape-complex-khmer.hh"
	"upstream.patched/src/hb-ot-shape-complex-myanmar.cc"
	"upstream.patched/src/hb-ot-shape-complex-myanmar.hh"
	"upstream.patched/src/hb-ot-shape-complex-syllabic.cc"
	"upstream.patched/src/hb-ot-shape-complex-syllabic.hh"
	"upstream.patched/src/hb-ot-shape-complex-thai.cc"
	"upstream.patched/src/hb-ot-shape-complex-use-table.hh"
	"upstream.patched/src/hb-ot-shape-complex-use.cc"
	"upstream.patched/src/hb-ot-shape-complex-vowel-constraints.cc"
	"upstream.patched/src/hb-ot-shape-complex-vowel-constraints.hh"
	"upstream.patched/src/hb-ot-shape-complex.hh"
	"upstream.patched/src/hb-ot-shape-fallback.cc"
	"upstream.patched/src/hb-ot-shape-fallback.hh"
	"upstream.patched/src/hb-ot-shape-normalize.cc"
	"upstream.patched/src/hb-ot-shape-normalize.hh"
	"upstream.patched/src/hb-ot-shape.cc"
	"upstream.patched/src/hb-ot-shape.hh"
	"upstream.patched/src/hb-ot-stat-table.hh"
	"upstream.patched/src/hb-ot-tag-table.hh"
	"upstream.patched/src/hb-ot-tag.cc"
	"upstream.patched/src/hb-ot-var-avar-table.hh"
	"upstream.patched/src/hb-ot-var-fvar-table.hh"
	"upstream.patched/src/hb-ot-var-gvar-table.hh"
	"upstream.patched/src/hb-ot-var-hvar-table.hh"
	"upstream.patched/src/hb-ot-var-mvar-table.hh"
	"upstream.patched/src/hb-ot-var.cc"
	"upstream.patched/src/hb-ot-vorg-table.hh"
	"upstream.patched/src/hb-pool.hh"
	"upstream.patched/src/hb-sanitize.hh"
	"upstream.patched/src/hb-serialize.hh"
	"upstream.patched/src/hb-set-digest.hh"
	"upstream.patched/src/hb-set.cc"
	"upstream.patched/src/hb-set.hh"
	"upstream.patched/src/hb-shape-plan.cc"
	"upstream.patched/src/hb-shape-plan.hh"
	"upstream.patched/src/hb-shape.cc"
	"upstream.patched/src/hb-shaper-impl.hh"
	"upstream.patched/src/hb-shaper-list.hh"
	"upstream.patched/src/hb-shaper.cc"
	"upstream.patched/src/hb-shaper.hh"
	"upstream.patched/src/hb-static.cc"
	"upstream.patched/src/hb-string-array.hh"
	"upstream.patched/src/hb-style.cc"
	"upstream.patched/src/hb-ucd-table.hh"
	"upstream.patched/src/hb-ucd.cc"
	"upstream.patched/src/hb-unicode-emoji-table.hh"
	"upstream.patched/src/hb-unicode.cc"
	"upstream.patched/src/hb-unicode.hh"
	"upstream.patched/src/hb-utf.hh"
	"upstream.patched/src/hb-vector.hh"
	"upstream.patched/src/hb.hh"

	"upstream.patched/src/hb-buffer-deserialize-json.hh"
	"upstream.patched/src/hb-buffer-deserialize-text.hh"
	"upstream.patched/src/hb-number-parser.hh"
	"upstream.patched/src/hb-ot-shape-complex-indic-machine.hh"
	"upstream.patched/src/hb-ot-shape-complex-khmer-machine.hh"
	"upstream.patched/src/hb-ot-shape-complex-myanmar-machine.hh"
	"upstream.patched/src/hb-ot-shape-complex-use-machine.hh"

	# ICU
	"upstream.patched/src/hb-icu.cc"
)

set(target_specific_sources "")
set(target_specific_definitions_public "")
set(target_specific_definitions_private "")
set(target_specific_libraries_public "")
if (CMAKE_TARGET_OS STREQUAL "windows")

	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/hb-gdi.cc"
	)
	set(target_specific_definitions_private ${target_specific_definitions_private}
		-DHAVE_GDI=1
	)
	find_library(gdi32_LIBRARY gdi32)
	target_specific_libraries_public(
		${gdi32_LIBRARY}
	)

	include(CheckTypeSize)
	set(CMAKE_EXTRA_INCLUDE_FILES
		"windows.h"
		"dwrite.h")
	check_type_size("class IDWriteFontFileEnumerator" SIZEOF_IDWriteFontFileEnumerator
		LANGUAGE CXX)
	if (SIZEOF_IDWriteFontFileEnumerator)
		set(target_specific_sources "${target_specific_sources}"
			"upstream.patched/src/hb-directwrite.cc"
		)
		set(target_specific_definitions_private ${target_specific_definitions_private}
			-DHAVE_DIRECTWRITE=1
		)
		find_library(dwrite_LIBRARY dwrite)
		target_specific_libraries_public(
			${dwrite_LIBRARY}
		)
	endif()
	unset(CMAKE_EXTRA_INCLUDE_FILES)

elseif (CMAKE_TARGET_OS STREQUAL "macosx")

	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/hb-coretext.cc"
	)
	set(target_specific_definitions_private ${target_specific_definitions_private}
		-DHAVE_CORETEXT=1
		-DHAVE_PTHREAD
	)
	find_library(CoreText_LIBRARY CoreText)
	set(target_specific_libraries_public ${target_specific_libraries_public}
		"${CoreText_LIBRARY}/CoreText.tbd"
	)

elseif (CMAKE_TARGET_OS STREQUAL "ios")

	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/hb-coretext.cc"
	)
	set(target_specific_definitions_private ${target_specific_definitions_private}
		-DHAVE_CORETEXT=1
		-DHAVE_PTHREAD
	)
	find_library(CoreText_LIBRARY CoreText)
	set(target_specific_libraries_public ${target_specific_libraries_public}
		"${CoreText_LIBRARY}/CoreText.tbd"
	)

elseif (CMAKE_TARGET_OS STREQUAL "linux")

	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/hb-ft.cc"
	)
	set(target_specific_definitions_private ${target_specific_definitions_private}
		-DHAVE_FREETYPE=1
		-DHAVE_PTHREAD
	)
	set(target_specific_libraries_public ${target_specific_libraries_public}
		freetype_static
	)

elseif (CMAKE_TARGET_OS STREQUAL "android")

	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/hb-ft.cc"
	)
	set(target_specific_definitions_private ${target_specific_definitions_private}
		-DHAVE_FREETYPE=1
		-DHAVE_PTHREAD
	)
	set(target_specific_libraries_public ${target_specific_libraries_public}
		freetype_static
	)

endif()

add_library(harfbuzz_static STATIC
	${common_sources}
	${target_specific_sources}
)
target_link_libraries(harfbuzz_static
	LINK_PUBLIC
		icu4c_static
		${target_specific_libraries_public}
)

target_compile_definitions(harfbuzz_static
	PRIVATE
		-DHAVE_ICU=1
		${target_specific_definitions_private}
	PUBLIC
		-DHB_EXTERN=
		${target_specific_definitions_public}
)

target_include_directories(harfbuzz_static
	PUBLIC
		"upstream.patched/src"
)
set_property(TARGET harfbuzz_static
	PROPERTY
		CXX_STANDARD 11
)
if (CMAKE_TARGET_BUILD_TOOL STREQUAL "xcode")
	# NOTE: Workaround https://gitlab.kitware.com/cmake/cmake/-/issues/19713
	set_property(TARGET harfbuzz_static
		PROPERTY
			XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11"
	)
endif()
