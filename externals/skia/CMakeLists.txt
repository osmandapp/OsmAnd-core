project(skia)

prepare_upstream()

set(target_specific_sources "")
set(target_specific_dependencies "")
set(target_specific_libraries_public "")
set(target_specific_include_dirs_private "")
set(target_specific_include_dirs_public "")
set(target_specific_definitions_private "")
set(target_specific_definitions_public "")

set(used_arch_specific_sources FALSE)

set(target_specific_sources__none
)

set(target_specific_sources__x86 "")
if (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "x86" AND NOT CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "fat")
	set(used_arch_specific_sources TRUE)
	set(target_specific_sources__x86
		"upstream.patched/src/opts/SkOpts_ssse3.cpp"
		"upstream.patched/src/opts/SkOpts_sse41.cpp"
		"upstream.patched/src/opts/SkOpts_sse42.cpp"
		"upstream.patched/src/opts/SkOpts_avx.cpp"
		"upstream.patched/src/opts/SkOpts_hsw.cpp"
		"upstream.patched/src/opts/SkOpts_skx.cpp"
	)
	set(target_specific_sources "${target_specific_sources}"
		"${target_specific_sources__x86}")
	if (CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
		set_source_files_properties(
			"upstream.patched/src/opts/SkOpts_ssse3.cpp"
			PROPERTIES
				COMPILE_FLAGS "-mssse3")
		set_source_files_properties(
			"upstream.patched/src/opts/SkOpts_sse41.cpp"
			PROPERTIES
				COMPILE_FLAGS "-msse4.1")
		set_source_files_properties(
			"upstream.patched/src/opts/SkOpts_sse42.cpp"
			PROPERTIES
				COMPILE_FLAGS "-msse4.2")
		set_source_files_properties(
			"upstream.patched/src/opts/SkOpts_avx.cpp"
			PROPERTIES
				COMPILE_FLAGS "-mavx")
		set_source_files_properties(
			"upstream.patched/src/opts/SkOpts_hsw.cpp"
			PROPERTIES
				COMPILE_FLAGS "-march=haswell")
		set_source_files_properties(
			"upstream.patched/src/opts/SkOpts_skx.cpp"
			PROPERTIES
				COMPILE_FLAGS "-march=skylake-avx512")
	elseif (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
		set_source_files_properties(
			"upstream.patched/src/opts/SkOpts_ssse3.cpp"
			PROPERTIES
				COMPILE_FLAGS "/arch:SSE2"
				COMPILE_DEFINITIONS
				S	K_CPU_SSE_LEVEL=SK_CPU_SSE_LEVEL_SSSE3
		)
		set_source_files_properties(
			"upstream.patched/src/opts/SkOpts_sse41.cpp"
			PROPERTIES
				COMPILE_FLAGS "/arch:SSE2"
				COMPILE_DEFINITIONS
					SK_CPU_SSE_LEVEL=SK_CPU_SSE_LEVEL_SSE41	
		)
		set_source_files_properties(
			"upstream.patched/src/opts/SkOpts_sse42.cpp"
			PROPERTIES
				COMPILE_FLAGS "/arch:SSE2"
				COMPILE_DEFINITIONS
					SK_CPU_SSE_LEVEL=SK_CPU_SSE_LEVEL_SSE42	
		)
		set_source_files_properties(
			"upstream.patched/src/opts/SkOpts_avx.cpp"
			PROPERTIES
				COMPILE_FLAGS "/arch:AVX"
		)
		set_source_files_properties(
			"upstream.patched/src/opts/SkOpts_hsw.cpp"
			PROPERTIES
				COMPILE_FLAGS "/arch:AVX2"
		)
		set_source_files_properties(
			"upstream.patched/src/opts/SkOpts_skx.cpp"
			PROPERTIES
				COMPILE_FLAGS "/arch:AVX512"
		)
	endif()
endif()

set(target_specific_sources__arm "")
set(target_specific_sources__arm64 "")
if (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "arm" OR CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "arm64" OR
	(CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "fat" AND CMAKE_TARGET_OS STREQUAL "ios"))
	set(used_arch_specific_sources TRUE)

	set(target_specific_sources__arm
	)

	set(target_specific_sources__arm64
		"upstream.patched/src/opts/SkOpts_crc32.cpp"
	)
	if (CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
		set_source_files_properties(
			"upstream.patched/src/opts/SkOpts_crc32.cpp"
			PROPERTIES
				COMPILE_FLAGS "-march=armv8-a+crc")
	endif()

	if (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "arm64")
		set(target_specific_sources "${target_specific_sources}"
			"${target_specific_sources__arm64}")
	else()
		set(target_specific_sources "${target_specific_sources}"
			"${target_specific_sources__arm}")
	endif()
endif()

if (NOT used_arch_specific_sources OR
	(CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "fat" AND CMAKE_TARGET_OS STREQUAL "ios"))
	set(target_specific_sources "${target_specific_sources}"
		"${target_specific_sources__none}"
	)
endif()

if (CMAKE_TARGET_OS STREQUAL "windows")
	set(target_specific_include_dirs_private ${target_specific_include_dirs_private}
		"upstream.patched/src/utils/win"
	)
	file(GLOB src_utils_win "upstream.patched/src/utils/win/*")
	file(GLOB src_utils_win_dw
		"upstream.patched/src/utils/win/SkDWrite*")
	list(REMOVE_ITEM src_utils_win ${src_utils_win_dw})
	file(GLOB src_gpu_gl_win "upstream.patched/src/gpu/gl/win/*")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkImageEncoder_WIC.cpp"
		"upstream.patched/src/ports/SkImageGeneratorWIC.cpp"
		"upstream.patched/src/ports/SkOSLibrary_win.cpp"
		"upstream.patched/src/ports/SkDebug_win.cpp"
		"upstream.patched/src/ports/SkFontHost_win.cpp"
		"upstream.patched/src/ports/SkTLS_win.cpp"
		"upstream.patched/src/ports/SkOSFile_win.cpp"
		"upstream.patched/src/utils/SkThreadUtils_win.cpp"
		${src_utils_win}
		${src_gpu_gl_win}
	)

	include(CheckTypeSize)
	set(CMAKE_EXTRA_INCLUDE_FILES
		"windows.h"
		"dwrite.h")
	check_type_size("class IDWriteFontFileEnumerator" SIZEOF_IDWriteFontFileEnumerator
		LANGUAGE CXX)
	if (SIZEOF_IDWriteFontFileEnumerator)
		set(target_specific_sources "${target_specific_sources}"
			"upstream.patched/src/ports/SkTypeface_win_dw.cpp"
			"upstream.patched/src/ports/SkFontMgr_win_dw.cpp"
			"upstream.patched/src/ports/SkScalerContext_win_dw.cpp"
			"upstream.patched/src/ports/SkRemotableFontMgr_win_dw.cpp"
			"upstream.patched/src/utils/win/SkDWrite.cpp"
			"upstream.patched/src/utils/win/SkDWriteFontFileStream.cpp"
			"upstream.patched/src/utils/win/SkDWriteGeometrySink.cpp"
			${src_utils_win_dw}
		)
	else()
		set(target_specific_sources "${target_specific_sources}"
		)
	endif()
	unset(CMAKE_EXTRA_INCLUDE_FILES)

	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_WIN
		-D_CRT_SECURE_NO_WARNINGS
		-DGR_GL_FUNCTION_TYPE=__stdcall
	)
	# This is not done via find_library() since under Linux it won't find needed library
	#find_library(usp10_LIBRARY usp10)
	set(target_specific_libraries_public ${target_specific_libraries_public}
		#"${usp10_LIBRARY}"
		usp10
	)
elseif (CMAKE_TARGET_OS STREQUAL "macosx")
	set(target_specific_include_dirs_public ${target_specific_include_dirs_public}
		"upstream.patched/include/utils/mac"
	)
	file(GLOB src_utils_mac "upstream.patched/src/utils/mac/*")
	file(GLOB src_gpu_gl_mac "upstream.patched/src/gpu/gl/mac/*")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkFontMgr_mac_ct.cpp"
		"upstream.patched/src/ports/SkFontMgr_mac_ct_factory.cpp"
		"upstream.patched/src/ports/SkTypeface_mac_ct.cpp"
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkDiscardableMemory_none.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		"upstream.patched/src/ports/SkScalerContext_mac_ct.cpp"
		"upstream.patched/src/ports/SkImageEncoder_CG.cpp"
      		"upstream.patched/src/ports/SkImageGeneratorCG.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread.cpp"
		${src_utils_mac}
		${src_gpu_gl_mac}
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_MAC
	)
	set(target_specific_definitions_private ${target_specific_definitions_private}
		-DSK_USE_POSIX_THREADS
	)
	set(library_suffix_tbd ".tbd")
	find_library(CoreGraphics_LIBRARY CoreGraphics)
	find_library(CoreText_LIBRARY CoreText)
	find_library(CoreFoundation_LIBRARY CoreFoundation)
	set(target_specific_libraries_public ${target_specific_libraries_public}
		"${CoreText_LIBRARY}/CoreText${library_suffix_tbd}"
		"${CoreGraphics_LIBRARY}/CoreGraphics${library_suffix_tbd}"
		"${CoreFoundation_LIBRARY}/CoreFoundation${library_suffix_tbd}"
	)
elseif (CMAKE_TARGET_OS STREQUAL "ios")
	set(target_specific_include_dirs_public ${target_specific_include_dirs_public}
		"upstream.patched/include/utils/mac"
		"upstream.patched/include/utils/ios"
	)
	file(GLOB src_utils_mac "upstream.patched/src/utils/mac/*")
	file(GLOB src_gpu_gl_ios "upstream.patched/src/gpu/gl/iOS/*")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkFontMgr_mac_ct.cpp"
		"upstream.patched/src/ports/SkFontMgr_mac_ct_factory.cpp"
		"upstream.patched/src/ports/SkTypeface_mac_ct.cpp"
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkDiscardableMemory_none.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		"upstream.patched/src/ports/SkScalerContext_mac_ct.cpp"
		"upstream.patched/src/ports/SkImageEncoder_CG.cpp"
		"upstream.patched/src/ports/SkImageGeneratorCG.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread.cpp"
		"upstream.patched/src/utils/mac/SkCreateCGImageRef.cpp"
		${src_utils_mac}
		${src_gpu_gl_ios}
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_IOS
	)
	set(target_specific_definitions_private ${target_specific_definitions_private}
		-DSK_USE_POSIX_THREADS
	)
	find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
	find_library(CORE_TEXT_LIBRARY CoreText)
	find_library(CORE_GRAPHICS_LIBRARY CoreGraphics)
	set(target_specific_libraries_public ${target_specific_libraries_public}
		"${CORE_FOUNDATION_LIBRARY}/CoreFoundation"
		"${CORE_TEXT_LIBRARY}/CoreText"
		"${CORE_GRAPHICS_LIBRARY}/CoreGraphics"
	)
elseif (CMAKE_TARGET_OS STREQUAL "linux")
	file(GLOB src_gpu_gl_glx "upstream.patched/src/gpu/gl/glx/*")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkFontMgr_fontconfig.cpp"
		"upstream.patched/src/ports/SkFontMgr_fontconfig_factory.cpp"
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkDiscardableMemory_none.cpp"
		"upstream.patched/src/ports/SkFontConfigInterface_direct.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType_common.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread.cpp"
		${src_gpu_gl_glx}
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_UNIX
	)
	set(target_specific_definitions_private ${target_specific_definitions_private}
		-DSK_USE_POSIX_THREADS
	)
	set(target_specific_dependencies ${target_specific_dependencies}
		freetype2_static
	)
	find_library(PTHREAD_LIBRARY pthread)
	find_library(FONTCONFIG_LIBRARY fontconfig)
	set(target_specific_libraries_public ${target_specific_libraries_public}
		freetype2_static
		${PTHREAD_LIBRARY}
		${FONTCONFIG_LIBRARY}
	)
elseif (CMAKE_TARGET_OS STREQUAL "android")
	file(GLOB src_gpu_gl_android "upstream.patched/src/gpu/gl/android/*")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkFontMgr_android.cpp"
		"upstream.patched/src/ports/SkFontMgr_android_factory.cpp"
		"upstream.patched/src/ports/SkFontMgr_android_parser.cpp"
		"upstream.patched/src/ports/SkDebug_android.cpp"
		"upstream.patched/src/ports/SkDiscardableMemory_none.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType_common.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		#"upstream.patched/src/ports/SkImageEncoder_NDK.cpp" # NOTE: requires API30+
		#"upstream.patched/src/ports/SkImageGeneratorNDK.cpp" # NOTE: requires API30+
		#"upstream.patched/src/ports/SkNDKConversions.cpp" # NOTE: requires API30+
		"upstream.patched/src/utils/SkThreadUtils_pthread.cpp"
		${src_gpu_gl_android}
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_ANDROID
		#-DSK_ENABLE_NDK_IMAGES # NOTE: requires API30+
		-DANDROID_LARGE_MEMORY_DEVICE
	)
	set(target_specific_definitions_private ${target_specific_definitions_private}
		-DSK_USE_POSIX_THREADS
	)
	set(target_specific_dependencies ${target_specific_dependencies}
		freetype2_static
		android_cpufeatures
	)
	set(target_specific_libraries_public ${target_specific_libraries_public}
		freetype2_static
		android_cpufeatures
		jnigraphics
	)
endif()

file(GLOB src_core "upstream.patched/src/core/*")
file(GLOB src_codec "upstream.patched/src/codec/*")
file(GLOB src_codec__remove
	"upstream.patched/src/codec/SkAndroid*"
	"upstream.patched/src/codec/SkRaw*"
	"upstream.patched/src/codec/SkWebp*"
	"upstream.patched/src/codec/SkWuffs*"
)
list(REMOVE_ITEM src_codec ${src_codec__remove})
file(GLOB src_effects "upstream.patched/src/effects/*")
file(GLOB src_effects_gradients "upstream.patched/src/effects/gradients/*")
file(GLOB src_effects_imagefilters "upstream.patched/src/effects/imagefilters/*")
file(GLOB src_fonts
	"upstream.patched/src/fonts/SkRemotableFontMgr.cpp"
)
file(GLOB src_image "upstream.patched/src/image/*")
file(GLOB src_image__remove
	"upstream.patched/src/image/SkSurface_GpuMtl.mm"
)
list(REMOVE_ITEM src_image ${src_image__remove})
file(GLOB src_images "upstream.patched/src/images/*")
file(GLOB src_lazy "upstream.patched/src/lazy/*")
file(GLOB src_shaders "upstream.patched/src/shaders/*")
file(GLOB src_shaders_gradients "upstream.patched/src/shaders/gradients/*")
file(GLOB src_sfnt "upstream.patched/src/sfnt/*")
file(GLOB src_pathops "upstream.patched/src/pathops/*")
file(GLOB src_pipe "upstream.patched/src/pipe/*")
file(GLOB src_ports
	"upstream.patched/src/ports/SkGlobalInitialization_default.cpp"
	"upstream.patched/src/ports/SkMemory_malloc.cpp"
	"upstream.patched/src/ports/SkOSFile_stdio.cpp"
	"upstream.patched/src/ports/SkDiscardableMemory_none.cpp"
	"upstream.patched/src/ports/SkImageGenerator_skia.cpp"
)
file(GLOB src_gpu "upstream.patched/src/gpu/*")
file(GLOB src_gpu__remove
	"upstream.patched/src/gpu/*/*")
list(REMOVE_ITEM src_gpu ${src_gpu__remove})
file(GLOB src_gpu_effects "upstream.patched/src/gpu/effects/*")
file(GLOB src_gpu_geometry "upstream.patched/src/gpu/geometry/*")
file(GLOB src_gpu_gradients "upstream.patched/src/gpu/gradients/*")
file(GLOB src_gpu_mock "upstream.patched/src/gpu/mock/*")
file(GLOB src_gpu_ops "upstream.patched/src/gpu/ops/*")
file(GLOB src_gpu_tessellate "upstream.patched/src/gpu/tessellate/*")
file(GLOB src_gpu_tessellate_shaders "upstream.patched/src/gpu/tessellate/shaders/*")
file(GLOB src_gpu_text "upstream.patched/src/gpu/text/*")
file(GLOB src_gpu_gl "upstream.patched/src/gpu/gl/*")
file(GLOB src_gpu_gl_builders "upstream.patched/src/gpu/gl/builders/*")
file(GLOB src_gpu_glsl "upstream.patched/src/gpu/glsl/*")
file(GLOB src_gpu_v1 "upstream.patched/src/gpu/v1/*")
file(GLOB src_sksl "upstream.patched/src/sksl/*")
file(GLOB src_sksl__remove
	"upstream.patched/src/sksl/SkSLMain.cpp"
)
list(REMOVE_ITEM src_sksl ${src_sksl__remove})
file(GLOB src_sksl_analysis "upstream.patched/src/sksl/analysis/*")
file(GLOB src_sksl_codegen "upstream.patched/src/sksl/codegen/*")
file(GLOB src_sksl_dsl "upstream.patched/src/sksl/dsl/*")
file(GLOB src_sksl_dsl_priv "upstream.patched/src/sksl/dsl/priv/*")
file(GLOB src_sksl_generated "upstream.patched/src/sksl/generated/*")
file(GLOB src_sksl_ir "upstream.patched/src/sksl/ir/*")
file(GLOB src_sksl_lex "upstream.patched/src/sksl/lex/*")
file(GLOB src_sksl_lex__remove
        "upstream.patched/src/sksl/lex/Main.cpp"
)
list(REMOVE_ITEM src_sksl_lex ${src_sksl_lex__remove})
file(GLOB src_sksl_transform "upstream.patched/src/sksl/transform/*")
file(GLOB src_utils "upstream.patched/src/utils/*")
file(GLOB src_utils__remove
)
list(REMOVE_ITEM src_utils ${src_utils__remove})
file(GLOB src_utils_debugger "upstream.patched/src/utils/debugger/*")

file(GLOB thirdparty_skcms "upstream.patched/third_party/skcms/*.cc")

add_library(skia_static STATIC
	${src_core}
	${src_codec}
	${src_effects}
	${src_effects_gradients}
	${src_effects_imagefilters}
	${src_fonts}
	${src_image}
	${src_images}
	${src_lazy}
	${src_shaders}
	${src_shaders_gradients}
	${src_sfnt}
	${src_pathops}
	${src_pipe}
	${src_ports}
	${src_gpu}
	${src_gpu_effects}
	${src_gpu_geometry}
	${src_gpu_gradients}
	${src_gpu_mock}
	${src_gpu_ops}
	${src_gpu_tessellate}
	${src_gpu_tessellate_shaders}
	${src_gpu_text}
	${src_gpu_gl}
	${src_gpu_gl_builders}
	${src_gpu_glsl}
	${src_gpu_v1}
	${src_sksl}
	${src_sksl_analysis}
	${src_sksl_codegen}
	${src_sksl_dsl}
	${src_sksl_dsl_priv}
	${src_sksl_generated}
	${src_sksl_ir}
	${src_sksl_lex}
	${src_sksl_transform}
	${src_utils}
	${src_utils_debugger}
	${target_specific_sources}
	${thirdparty_skcms}
)

add_dependencies(skia_static
	expat_static
	png_static
	gif_static
	jpeg_static
	${target_specific_dependencies}
)

target_compile_definitions(skia_static
	PRIVATE
		-DSK_IGNORE_ETC1_SUPPORT
		-DSK_CODEC_DECODES_JPEG
		-DSK_CODEC_DECODES_PNG
		-DSK_ENCODE_JPEG
		-DSK_ENCODE_PNG
		${target_specific_definitions_private}
	PUBLIC
		-DSK_RELEASE
		-DSK_GL
		#-DSK_SUPPORT_GPU
		${target_specific_definitions_public}
)
target_include_directories(skia_static
	PRIVATE
		#"upstream.patched"
		#"upstream.patched/src/core"
		#"upstream.patched/src/config"
		#"upstream.patched/src/effects"
		#"upstream.patched/src/fonts"
		#"upstream.patched/src/image"
		#"upstream.patched/src/images"
		#"upstream.patched/src/lazy"
		#"upstream.patched/src/ports"
		#"upstream.patched/src/utils"
		#"upstream.patched/src/xml"
		#"upstream.patched/src/sfnt"
		#"upstream.patched/src/gpu"
		#"upstream.patched/src/opts"
		${target_specific_include_dirs_private}
	PUBLIC
		"upstream.patched"
		"upstream.patched/include/core"
		#"upstream.patched/include/lazy"
		#"upstream.patched/include/pathops"
		#"upstream.patched/src/core"
		#"upstream.patched/include/config"
		"upstream.patched/include/effects"
		#"upstream.patched/include/images"
		#"upstream.patched/include/pipe"
		#"upstream.patched/include/ports"
		#"upstream.patched/include/utils"
		#"upstream.patched/include/xml"
		#"upstream.patched/include/gpu"
		${target_specific_include_dirs_public}

		"upstream.patched/include/third_party/skcms"

		# SkMathPriv.h
		"upstream.patched/src/core"
)
target_link_libraries(skia_static
	LINK_PUBLIC
		expat_static
		png_static
		gif_static
		jpeg_static
		${target_specific_libraries_public}
)

if (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "fat" AND CMAKE_TARGET_OS STREQUAL "ios")
	set(ios_iphoneos_armv7_exclude_list
		"${target_specific_sources__x86}"
		"${target_specific_sources__none}")
	string(REPLACE ";" " " ios_iphoneos_armv7_exclude "${ios_iphoneos_armv7_exclude_list}")
	set_target_properties(skia_static
		PROPERTIES
			XCODE_ATTRIBUTE_EXCLUDED_SOURCE_FILE_NAMES[sdk=iphoneos*][arch=armv7] "${ios_iphoneos_armv7_exclude}")

	set(ios_iphoneos_arm64_exclude_list
		"${target_specific_sources__x86}"
		"${target_specific_sources__arm}"
		"${target_specific_sources__arm64}")
	list(REMOVE_ITEM ios_iphoneos_arm64_exclude_list "upstream.patched/src/opts/SkTextureCompression_opts_none.cpp")
	list(REMOVE_ITEM ios_iphoneos_arm64_exclude_list "upstream.patched/src/opts/SkUtils_opts_none.cpp")
	string(REPLACE ";" " " ios_iphoneos_arm64_exclude "${ios_iphoneos_arm64_exclude_list}")
	set_target_properties(skia_static
		PROPERTIES
			XCODE_ATTRIBUTE_EXCLUDED_SOURCE_FILE_NAMES[sdk=iphoneos*][arch=arm64] "${ios_iphoneos_arm64_exclude}"
	)

	set(ios_iphonesimulator_exclude_list
		"${target_specific_sources__arm}"
		"${target_specific_sources__arm64}"
		"${target_specific_sources__x86}")
	list(REMOVE_ITEM ios_iphonesimulator_exclude_list "upstream.patched/src/opts/SkUtils_opts_none.cpp")
	list(REMOVE_ITEM ios_iphonesimulator_exclude_list "upstream.patched/src/opts/SkTextureCompression_opts_none.cpp")
	string(REPLACE ";" " " ios_iphonesimulator_exclude "${ios_iphonesimulator_exclude_list}")
	set_target_properties(skia_static
		PROPERTIES
			XCODE_ATTRIBUTE_EXCLUDED_SOURCE_FILE_NAMES[sdk=iphonesimulator*][arch=*] "${ios_iphonesimulator_exclude}")
endif()
