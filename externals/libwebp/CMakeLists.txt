project(webp)

prepare_upstream()

# libwebp is compiled into skia_static (SkWebpCodec, SkWebpEncoder), so the set of static libraries that apps link
# does not change. SIMD files compile to nothing unless the target enables that instruction set, so all are listed.
file(GLOB webp_sources_dec "${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/dec/*.c")
file(GLOB webp_sources_demux "${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/demux/*.c")
file(GLOB webp_sources_dsp "${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/dsp/*.c")
file(GLOB webp_sources_enc "${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/enc/*.c")
file(GLOB webp_sources_mux "${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/mux/*.c")
file(GLOB webp_sources_utils "${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/utils/*.c")
file(GLOB webp_sources_sharpyuv "${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/sharpyuv/*.c")
set(webp_sources
	${webp_sources_dec}
	${webp_sources_demux}
	${webp_sources_dsp}
	${webp_sources_enc}
	${webp_sources_mux}
	${webp_sources_utils}
	${webp_sources_sharpyuv}
	PARENT_SCOPE)
set(webp_include_dirs
	"${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched"
	"${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src"
	PARENT_SCOPE)
# Skia works with BGR_565 and asks libwebp for RGB_565
set(webp_definitions
	-DWEBP_SWAP_16BIT_CSP
	PARENT_SCOPE)
