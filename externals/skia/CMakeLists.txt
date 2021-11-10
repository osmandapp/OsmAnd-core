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
	"upstream.patched/src/opts/SkBitmapProcState_opts_none.cpp"
	"upstream.patched/src/opts/SkBlitMask_opts_none.cpp"
	"upstream.patched/src/opts/SkBlitRow_opts_none.cpp"
	"upstream.patched/src/opts/SkBlurImage_opts_none.cpp"
	"upstream.patched/src/opts/SkMorphology_opts_none.cpp"
	"upstream.patched/src/opts/SkTextureCompression_opts_none.cpp"
	"upstream.patched/src/opts/SkUtils_opts_none.cpp"
	"upstream.patched/src/opts/SkXfermode_opts_none.cpp")

set(target_specific_sources__x86 "")
if (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "x86" AND NOT CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "fat")
	set(used_arch_specific_sources TRUE)
	set(target_specific_sources__x86
		# x86
		"upstream.patched/src/opts/opts_check_x86.cpp"
		"upstream.patched/src/opts/SkTextureCompression_opts_none.cpp"

		# SSE2
		"upstream.patched/src/opts/SkBitmapProcState_opts_SSE2.cpp"
		"upstream.patched/src/opts/SkBitmapFilter_opts_SSE2.cpp"
		"upstream.patched/src/opts/SkBlitRow_opts_SSE2.cpp"
		"upstream.patched/src/opts/SkBlitRect_opts_SSE2.cpp"
		"upstream.patched/src/opts/SkBlurImage_opts_SSE2.cpp"
		"upstream.patched/src/opts/SkMorphology_opts_SSE2.cpp"
		"upstream.patched/src/opts/SkUtils_opts_SSE2.cpp"
		"upstream.patched/src/opts/SkXfermode_opts_SSE2.cpp"

		# SSSE3
		"upstream.patched/src/opts/SkBitmapProcState_opts_SSSE3.cpp"

		# SSE4
		# SkBlitRow_opts_SSE4_x64_asm.S
		# SkBlitRow_opts_SSE4_asm.S
		"upstream.patched/src/opts/SkBlurImage_opts_SSE4.cpp")
	set(target_specific_sources "${target_specific_sources}"
		"${target_specific_sources__x86}")
	if (CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
		set_source_files_properties(
			"upstream.patched/src/opts/SkBitmapProcState_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkBitmapFilter_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkBlitRow_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkBlitRect_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkBlurImage_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkMorphology_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkUtils_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkXfermode_opts_SSE2.cpp"
			PROPERTIES
				COMPILE_FLAGS "-msse2")
		set_source_files_properties(
			"upstream.patched/src/opts/SkBitmapProcState_opts_SSSE3.cpp"
			PROPERTIES
				COMPILE_FLAGS "-mssse3")
		set_source_files_properties(
			"upstream.patched/src/opts/SkBlurImage_opts_SSE4.cpp"
			PROPERTIES
				COMPILE_FLAGS "-msse4.1")
	elseif (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
		set_source_files_properties(
			"upstream.patched/src/opts/SkBitmapProcState_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkBitmapFilter_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkBlitRow_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkBlitRect_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkBlurImage_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkMorphology_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkUtils_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkXfermode_opts_SSE2.cpp"
			PROPERTIES
				COMPILE_FLAGS "/arch:SSE2")
		set_source_files_properties(
			"upstream.patched/src/opts/SkBitmapProcState_opts_SSSE3.cpp"
			PROPERTIES
				COMPILE_FLAGS "/arch:SSE2")
		set_source_files_properties(
			"upstream.patched/src/opts/SkBlurImage_opts_SSE4.cpp"
			PROPERTIES
				COMPILE_FLAGS "/arch:SSE2")
	endif()
endif()

set(target_specific_sources__arm "")
set(target_specific_sources__arm64 "")
if (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "arm" OR CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "arm64" OR
	(CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "fat" AND CMAKE_TARGET_OS STREQUAL "ios"))
	set(used_arch_specific_sources TRUE)

	# SKIA should be build with ARM instruction set instead of Thumb
	set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -marm")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -marm")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -marm")

	if (NOT CMAKE_TARGET_OS STREQUAL "ios")
		set(target_specific_definitions_public ${target_specific_definitions_public}
			-DSK_ARM_HAS_OPTIONAL_NEON
		)
	endif()
	set(target_specific_sources__arm
		# ARM
		"upstream.patched/src/opts/memset.arm.S"
		"upstream.patched/src/opts/SkBitmapProcState_opts_arm.cpp"
		"upstream.patched/src/opts/SkBlitMask_opts_arm.cpp"
		"upstream.patched/src/opts/SkBlitRow_opts_arm.cpp"
		"upstream.patched/src/opts/SkBlurImage_opts_arm.cpp"
		"upstream.patched/src/opts/SkMorphology_opts_arm.cpp"
		"upstream.patched/src/opts/SkTextureCompression_opts_arm.cpp"
		"upstream.patched/src/opts/SkUtils_opts_arm.cpp"
		"upstream.patched/src/opts/SkXfermode_opts_arm.cpp"

		# NEON
		"upstream.patched/src/opts/memset16_neon.S"
		"upstream.patched/src/opts/memset32_neon.S"
		"upstream.patched/src/opts/SkBitmapProcState_arm_neon.cpp"
		"upstream.patched/src/opts/SkBitmapProcState_matrixProcs_neon.cpp"
		"upstream.patched/src/opts/SkBlitMask_opts_arm_neon.cpp"
		"upstream.patched/src/opts/SkBlitRow_opts_arm_neon.cpp"
		"upstream.patched/src/opts/SkBlurImage_opts_neon.cpp"
		"upstream.patched/src/opts/SkMorphology_opts_neon.cpp"
		"upstream.patched/src/opts/SkTextureCompression_opts_neon.cpp"
		"upstream.patched/src/opts/SkXfermode_opts_arm_neon.cpp"
	)
	set(target_specific_sources__arm64
		# ARM64
		"upstream.patched/src/opts/SkBitmapProcState_arm_neon.cpp"
        "upstream.patched/src/opts/SkBitmapProcState_matrixProcs_neon.cpp"
        "upstream.patched/src/opts/SkBitmapProcState_opts_arm.cpp"
        "upstream.patched/src/opts/SkBlitMask_opts_arm.cpp"
        "upstream.patched/src/opts/SkBlitMask_opts_arm_neon.cpp"
        "upstream.patched/src/opts/SkBlitRow_opts_arm.cpp"
        "upstream.patched/src/opts/SkBlitRow_opts_arm_neon.cpp"
        "upstream.patched/src/opts/SkBlurImage_opts_arm.cpp"
        "upstream.patched/src/opts/SkBlurImage_opts_neon.cpp"
        "upstream.patched/src/opts/SkMorphology_opts_arm.cpp"
        "upstream.patched/src/opts/SkMorphology_opts_neon.cpp"
        "upstream.patched/src/opts/SkTextureCompression_opts_none.cpp"
        "upstream.patched/src/opts/SkUtils_opts_none.cpp"
        "upstream.patched/src/opts/SkXfermode_opts_arm.cpp"
        "upstream.patched/src/opts/SkXfermode_opts_arm_neon.cpp"
	)
	if (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "arm64")
		set(target_specific_sources "${target_specific_sources}"
			"${target_specific_sources__arm64}")
	else()
		set(target_specific_sources "${target_specific_sources}"
			"${target_specific_sources__arm}")
	endif()
	if (CMAKE_TARGET_OS STREQUAL "ios" AND CMAKE_COMPILER_FAMILY STREQUAL "clang")
		set_source_files_properties(
			# ARM
			"upstream.patched/src/opts/memset.arm.S"

			# NEON
			"upstream.patched/src/opts/memset32_neon.S"
			"upstream.patched/src/opts/memset16_neon.S"
			PROPERTIES
				COMPILE_FLAGS "-no-integrated-as")
	elseif (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "arm64")
		set_source_files_properties(
			# ARM64
			"upstream.patched/src/opts/SkBitmapProcState_arm_neon.cpp"
	        "upstream.patched/src/opts/SkBitmapProcState_matrixProcs_neon.cpp"
	        "upstream.patched/src/opts/SkBitmapProcState_opts_arm.cpp"
	        "upstream.patched/src/opts/SkBlitMask_opts_arm.cpp"
	        "upstream.patched/src/opts/SkBlitMask_opts_arm_neon.cpp"
	        "upstream.patched/src/opts/SkBlitRow_opts_arm.cpp"
	        "upstream.patched/src/opts/SkBlitRow_opts_arm_neon.cpp"
	        "upstream.patched/src/opts/SkBlurImage_opts_arm.cpp"
	        "upstream.patched/src/opts/SkBlurImage_opts_neon.cpp"
	        "upstream.patched/src/opts/SkMorphology_opts_arm.cpp"
	        "upstream.patched/src/opts/SkMorphology_opts_neon.cpp"
	        "upstream.patched/src/opts/SkTextureCompression_opts_none.cpp"
	        "upstream.patched/src/opts/SkUtils_opts_none.cpp"
	        "upstream.patched/src/opts/SkXfermode_opts_arm.cpp"
	        "upstream.patched/src/opts/SkXfermode_opts_arm_neon.cpp"
	        PROPERTIES
				COMPILE_FLAGS "-mfloat-abi=softfp -mfpu=neon -fomit-frame-pointer")
	else()
		set_source_files_properties(
			# ARM
			"upstream.patched/src/opts/memset.arm.S"
			"upstream.patched/src/opts/SkBitmapProcState_opts_arm.cpp"
			"upstream.patched/src/opts/SkBlitMask_opts_arm.cpp"
			"upstream.patched/src/opts/SkBlitRow_opts_arm.cpp"
			"upstream.patched/src/opts/SkBlurImage_opts_arm.cpp"
			"upstream.patched/src/opts/SkMorphology_opts_arm.cpp"
			"upstream.patched/src/opts/SkTextureCompression_opts_arm.cpp"
			"upstream.patched/src/opts/SkUtils_opts_arm.cpp"
			"upstream.patched/src/opts/SkXfermode_opts_arm.cpp"
			PROPERTIES
				COMPILE_FLAGS "-fomit-frame-pointer")
		set_source_files_properties(
			# NEON
			"upstream.patched/src/opts/memset16_neon.S"
			"upstream.patched/src/opts/memset32_neon.S"
			"upstream.patched/src/opts/SkBitmapProcState_arm_neon.cpp"
			"upstream.patched/src/opts/SkBitmapProcState_matrixProcs_neon.cpp"
			"upstream.patched/src/opts/SkBlitMask_opts_arm_neon.cpp"
			"upstream.patched/src/opts/SkBlitRow_opts_arm_neon.cpp"
			"upstream.patched/src/opts/SkBlurImage_opts_neon.cpp"
			"upstream.patched/src/opts/SkMorphology_opts_neon.cpp"
			"upstream.patched/src/opts/SkTextureCompression_opts_neon.cpp"
			"upstream.patched/src/opts/SkXfermode_opts_arm_neon.cpp"
			PROPERTIES
				COMPILE_FLAGS "-mfloat-abi=softfp -mfpu=neon -fomit-frame-pointer")
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
	set(target_specific_include_dirs_public ${target_specific_include_dirs_public}
		"upstream.patched/include/utils/win"
	)
	file(GLOB src_utils_win "upstream.patched/src/utils/win/*")
	file(GLOB src_utils_win__remove
		"upstream.patched/src/utils/win/SkDWrite*")
	list(REMOVE_ITEM src_utils_win ${src_utils_win__remove})
	file(GLOB src_gpu_gl_win "upstream.patched/src/gpu/gl/win/*")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/fonts/SkFontMgr_indirect.cpp"
		"upstream.patched/src/ports/SkDebug_win.cpp"
		"upstream.patched/src/ports/SkOSFile_win.cpp"
		"upstream.patched/src/ports/SkTime_win.cpp"
		"upstream.patched/src/ports/SkTLS_win.cpp"
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
			"upstream.patched/src/ports/SkFontMgr_win_dw.cpp"
			"upstream.patched/src/ports/SkFontMgr_default_dw.cpp"
			"upstream.patched/src/ports/SkRemotableFontMgr_win_dw.cpp"
			"upstream.patched/src/ports/SkScalerContext_win_dw.cpp"
			"upstream.patched/src/ports/SkTypeface_win_dw.cpp"
			"upstream.patched/src/utils/win/SkDWrite.cpp"
			"upstream.patched/src/utils/win/SkDWriteFontFileStream.cpp"
			"upstream.patched/src/utils/win/SkDWriteGeometrySink.cpp"
		)
	else()
		set(target_specific_sources "${target_specific_sources}"
			"upstream.patched/src/ports/SkFontHost_win.cpp"
			"upstream.patched/src/ports/SkFontMgr_default_gdi.cpp"
		)
	endif()
	unset(CMAKE_EXTRA_INCLUDE_FILES)

	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_WIN32
		-DSK_FONTHOST_USES_FONTMGR
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
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkDiscardableMemory_none.cpp"
		"upstream.patched/src/ports/SkFontHost_mac.cpp"
		"upstream.patched/src/ports/SkImageDecoder_CG.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTime_Unix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread_mach.cpp"
		${src_utils_mac}
		${src_gpu_gl_mac}
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_MAC
		-DSK_FONTHOST_USES_FONTMGR
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
	file(GLOB src_utils_ios "upstream.patched/src/utils/ios/*")
	file(GLOB src_gpu_gl_ios "upstream.patched/src/gpu/gl/iOS/*")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkDiscardableMemory_none.cpp"
		"upstream.patched/src/ports/SkFontHost_mac.cpp"
		"upstream.patched/src/ports/SkImageDecoder_CG.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTime_Unix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread_mach.cpp"
		#${src_utils_ios}
		"upstream.patched/src/utils/mac/SkCreateCGImageRef.cpp"
		"upstream.patched/src/utils/mac/SkStream_mac.cpp"
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
		"upstream.patched/src/fonts/SkFontMgr_fontconfig.cpp"
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkDiscardableMemory_none.cpp"
		"upstream.patched/src/ports/SkFontConfigInterface_direct.cpp"
		"upstream.patched/src/ports/SkFontHost_fontconfig.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType_common.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType.cpp"
		"upstream.patched/src/ports/SkFontHost_linux.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTime_Unix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread_linux.cpp"
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
		"upstream.patched/src/ports/SkDebug_android.cpp"
		"upstream.patched/src/ports/SkDiscardableMemory_none.cpp"
		"upstream.patched/src/ports/SkFontConfigParser_android.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType_common.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType.cpp"
		"upstream.patched/src/ports/SkFontMgr_android.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTime_Unix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread_other.cpp"
		${src_gpu_gl_android}
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_ANDROID
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
	)
endif()

file(GLOB src_core "upstream.patched/src/core/*")
file(GLOB src_effects "upstream.patched/src/effects/*")
file(GLOB src_effects_gradients "upstream.patched/src/effects/gradients/*")
file(GLOB src_fonts
	#"upstream.patched/src/fonts/*"
	"upstream.patched/src/fonts/SkGScalerContext.cpp"
	"upstream.patched/src/fonts/SkRemotableFontMgr.cpp")
file(GLOB src_image "upstream.patched/src/image/*")
file(GLOB src_images "upstream.patched/src/images/*")
file(GLOB src_images__remove
	"upstream.patched/src/images/SkImageDecoder_astc.*"
	"upstream.patched/src/images/SkImageDecoder_ktx.*"
	"upstream.patched/src/images/SkImageDecoder_libwebp.*"
	"upstream.patched/src/images/SkImageDecoder_pkm.*"
	"upstream.patched/src/images/SkImageRef_ashmem.*"
	"upstream.patched/src/images/SkForceLinking.*")
list(REMOVE_ITEM src_images ${src_images__remove})
file(GLOB src_lazy "upstream.patched/src/lazy/*")
file(GLOB src_sfnt "upstream.patched/src/sfnt/*")
file(GLOB src_pathops "upstream.patched/src/pathops/*")
file(GLOB src_pipe "upstream.patched/src/pipe/*")
file(GLOB src_ports
	"upstream.patched/src/ports/SkGlobalInitialization_default.cpp"
	"upstream.patched/src/ports/SkMemory_malloc.cpp"
	"upstream.patched/src/ports/SkOSFile_stdio.cpp"
	"upstream.patched/src/ports/SkImageGenerator_skia.cpp")
file(GLOB src_gpu "upstream.patched/src/gpu/*")
file(GLOB src_gpu_effects "upstream.patched/src/gpu/effects/*")
file(GLOB src_gpu_gl "upstream.patched/src/gpu/gl/*")
file(GLOB src_gpu_gl__remove
	"upstream.patched/src/gpu/gl/GrGLCreateNativeInterface_none.cpp"
	"upstream.patched/src/gpu/gl/GrGLDefaultInterface_none.cpp")
list(REMOVE_ITEM src_gpu_gl ${src_gpu_gl__remove})
file(GLOB src_gpu_gl_builders "upstream.patched/src/gpu/gl/builders/*")
file(GLOB src_utils "upstream.patched/src/utils/*")
file(GLOB src_utils__remove
	"upstream.patched/src/utils/SkLua.*"
	"upstream.patched/src/utils/SkLuaCanvas.*"
	"upstream.patched/src/utils/SkThreadUtils_*")
list(REMOVE_ITEM src_utils ${src_utils__remove})
file(GLOB src_utils_debugger "upstream.patched/src/utils/debugger/*")

add_library(skia_static STATIC
	${src_core}
	${src_effects}
	${src_effects_gradients}
	${src_fonts}
	${src_image}
	${src_images}
	${src_lazy}
	${src_sfnt}
	${src_pathops}
	${src_pipe}
	${src_ports}
	${src_gpu}
	${src_gpu_effects}
	${src_gpu_gl}
	${src_gpu_gl_builders}
	${src_utils}
	${src_utils_debugger}
	"upstream.patched/src/ports/SkDiscardableMemory_none.cpp"
	${target_specific_sources}
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
		${target_specific_definitions_private}
	PUBLIC
	    -DSK_RELEASE
		${target_specific_definitions_public}
)
target_include_directories(skia_static
	PRIVATE
		"upstream.patched/src/core"
		"upstream.patched/src/config"
		"upstream.patched/src/effects"
		"upstream.patched/src/fonts"
		"upstream.patched/src/image"
		"upstream.patched/src/images"
		"upstream.patched/src/lazy"
		"upstream.patched/src/ports"
		"upstream.patched/src/utils"
		"upstream.patched/src/xml"
		"upstream.patched/src/sfnt"
		"upstream.patched/src/gpu"
		"upstream.patched/src/opts"
		${target_specific_include_dirs_private}
	PUBLIC
		"upstream.patched/include/core"
		"upstream.patched/include/lazy"
		"upstream.patched/include/pathops"
		"upstream.patched/src/core"
		"upstream.patched/include/config"
		"upstream.patched/include/effects"
		"upstream.patched/include/images"
		"upstream.patched/include/pipe"
		"upstream.patched/include/ports"
		"upstream.patched/include/utils"
		"upstream.patched/include/xml"
		"upstream.patched/include/gpu"
		${target_specific_include_dirs_public}
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
