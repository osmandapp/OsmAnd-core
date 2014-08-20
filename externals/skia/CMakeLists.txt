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
if (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "x86" OR (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "fat" AND CMAKE_TARGET_OS STREQUAL "ios"))
	set(used_arch_specific_sources TRUE)
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/opts/opts_check_SSE2.cpp"
		"upstream.patched/src/opts/SkBitmapFilter_opts_SSE2.cpp"
		"upstream.patched/src/opts/SkBitmapProcState_opts_SSE2.cpp"
		"upstream.patched/src/opts/SkBitmapProcState_opts_SSSE3.cpp"
		"upstream.patched/src/opts/SkBlitRect_opts_SSE2.cpp"
		"upstream.patched/src/opts/SkBlitRow_opts_SSE2.cpp"
		"upstream.patched/src/opts/SkUtils_opts_SSE2.cpp"
	)
	if (CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
		set_source_files_properties(
			"upstream.patched/src/opts/SkBitmapFilter_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkBitmapProcState_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkBlitRect_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkBlitRow_opts_SSE2.cpp"
			"upstream.patched/src/opts/SkUtils_opts_SSE2.cpp"
			PROPERTIES
				COMPILE_FLAGS "-msse2")
		set_source_files_properties(
			"upstream.patched/src/opts/SkBitmapProcState_opts_SSSE3.cpp"
			PROPERTIES
				COMPILE_FLAGS "-mssse3")
	endif()
endif()
if (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "arm" OR (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "fat" AND CMAKE_TARGET_OS STREQUAL "ios"))
	set(used_arch_specific_sources TRUE)
	if (NOT CMAKE_TARGET_OS STREQUAL "ios")
		set(target_specific_definitions_public ${target_specific_definitions_public}
			-D__ARM_HAVE_OPTIONAL_NEON_SUPPORT
		)
	endif()
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/opts/opts_check_arm.cpp"
		"upstream.patched/src/opts/SkBlitRow_opts_arm.cpp"
		"upstream.patched/src/opts/SkBlitRow_opts_arm_neon.cpp"
		"upstream.patched/src/opts/SkBlitMask_opts_arm.cpp"
		"upstream.patched/src/opts/SkBitmapProcState_opts_arm.cpp"
		"upstream.patched/src/opts/SkBitmapProcState_matrixProcs_neon.cpp"
		"upstream.patched/src/opts/SkBitmapProcState_arm_neon.cpp"
		"upstream.patched/src/opts/memset32_neon.S"
		"upstream.patched/src/opts/memset16_neon.S"
		"upstream.patched/src/opts/memset.arm.S"
	)
	if (CMAKE_TARGET_OS STREQUAL "ios" AND CMAKE_COMPILER_FAMILY STREQUAL "clang")
		set_source_files_properties(
			"upstream.patched/src/opts/memset32_neon.S"
			"upstream.patched/src/opts/memset16_neon.S"
			"upstream.patched/src/opts/memset.arm.S"
			PROPERTIES
				COMPILE_FLAGS "-no-integrated-as")
	endif()
endif()
if (NOT used_arch_specific_sources)
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/opts/SkBlitRow_opts_none.cpp"
		"upstream.patched/src/opts/SkBlitMask_opts_none.cpp"
		"upstream.patched/src/opts/SkBitmapProcState_opts_none.cpp"
		"upstream.patched/src/opts/SkUtils_opts_none.cpp"
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
	file(GLOB src_gpu_gl_win "upstream.patched/src/gpu/gl/win/*")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkDebug_win.cpp"
		"upstream.patched/src/ports/SkFontHost_win_dw.cpp"
		"upstream.patched/src/ports/SkFontMgr_default_dw.cpp"
		"upstream.patched/src/ports/SkTime_win.cpp"
		"upstream.patched/src/ports/SkThread_win.cpp"
		"upstream.patched/src/ports/SkOSFile_win.cpp"
		"upstream.patched/src/ports/SkTLS_win.cpp"
		${src_utils_win}
		${src_gpu_gl_win}
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_WIN32
		-DSK_FONTHOST_USES_FONTMGR
		-D_CRT_SECURE_NO_WARNINGS
		-DGR_GL_FUNCTION_TYPE=__stdcall
	)
	set(target_specific_libraries_public ${target_specific_libraries_public}
		Usp10
	)
elseif (CMAKE_TARGET_OS STREQUAL "macosx")
	set(target_specific_include_dirs_public ${target_specific_include_dirs_public}
		"upstream.patched/include/utils/mac"
	)
	file(GLOB src_utils_mac "upstream.patched/src/utils/mac/*")
	file(GLOB src_gpu_gl_mac "upstream.patched/src/gpu/gl/mac/*")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkThread_pthread.cpp"
		"upstream.patched/src/ports/SkTime_Unix.cpp"
		"upstream.patched/src/ports/SkImageDecoder_CG.cpp"
		"upstream.patched/src/ports/SkFontHost_mac.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		${src_utils_mac}
		${src_gpu_gl_mac}
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_MAC
		-DSK_FONTHOST_USES_FONTMGR
	)
	find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
	find_library(CORE_TEXT_LIBRARY CoreText)
	find_library(CORE_GRAPHICS_LIBRARY CoreGraphics)
	set(target_specific_libraries_public ${target_specific_libraries_public}
		"${CORE_FOUNDATION_LIBRARY}/CoreFoundation"
		"${CORE_TEXT_LIBRARY}/CoreText"
		"${CORE_GRAPHICS_LIBRARY}/CoreGraphics"
	)
elseif (CMAKE_TARGET_OS STREQUAL "ios")
	set(target_specific_include_dirs_public ${target_specific_include_dirs_public}
		"upstream.patched/include/utils/mac"
		"upstream.patched/include/utils/ios"
	)
	file(GLOB src_gpu_gl_ios "upstream.patched/src/gpu/gl/iOS/*")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkThread_pthread.cpp"
		"upstream.patched/src/ports/SkTime_Unix.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		"upstream.patched/src/ports/SkFontHost_mac.cpp"
		"upstream.patched/src/utils/mac/SkStream_mac.cpp"
		${src_gpu_gl_ios}
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_IOS
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
	file(GLOB src_gpu_gl_unix "upstream.patched/src/gpu/gl/unix/*")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkThread_pthread.cpp"
		"upstream.patched/src/ports/SkTime_Unix.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType_common.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType.cpp"
		"upstream.patched/src/ports/SkFontHost_linux.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		${src_gpu_gl_unix}
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_UNIX
	)
	set(target_specific_dependencies ${target_specific_dependencies}
		freetype2_static
	)
	set(target_specific_libraries_public ${target_specific_libraries_public}
		freetype2_static
		pthread
	)
endif()

file(GLOB src_core "upstream.patched/src/core/*")
file(GLOB src_effects "upstream.patched/src/effects/*")
file(GLOB src_effects_gradients "upstream.patched/src/effects/gradients/*")
file(GLOB src_image "upstream.patched/src/image/*")
file(GLOB src_images "upstream.patched/src/images/*")
file(GLOB src_images__remove
	"upstream.patched/src/images/SkImageDecoder_libwebp.*"
	"upstream.patched/src/images/SkImageRef_ashmem.*")
list(REMOVE_ITEM src_images ${src_images__remove})
file(GLOB src_sfnt "upstream.patched/src/sfnt/*")
file(GLOB src_pathops "upstream.patched/src/pathops/*")
file(GLOB src_pathops__remove
	"upstream.patched/src/pathops/main.cpp")
list(REMOVE_ITEM src_pathops ${src_pathops__remove})
file(GLOB src_gpu "upstream.patched/src/gpu/*")
file(GLOB src_gpu__remove
	"upstream.patched/src/gpu/GrAddPathRenderers_none.cpp"
	"upstream.patched/src/gpu/gr_unittests.cpp"
	"upstream.patched/src/gpu/FlingState.cpp"
	"upstream.patched/src/gpu/GrRectanizer_fifo.cpp")
list(REMOVE_ITEM src_gpu ${src_gpu__remove})
file(GLOB src_gpu_effects "upstream.patched/src/gpu/effects/*")
file(GLOB src_gpu_gl "upstream.patched/src/gpu/gl/*")
file(GLOB src_gpu_gl__remove
	"upstream.patched/src/gpu/gl/GrGLCreateNativeInterface_none.cpp"
	"upstream.patched/src/gpu/gl/GrGLDefaultInterface_none.cpp")
list(REMOVE_ITEM src_gpu_gl ${src_gpu_gl__remove})

add_library(skia_static STATIC
	${src_core}
	${src_effects}
	${src_effects_gradients}
	${src_image}
	${src_images}
	${src_sfnt}
	${src_pathops}
	${src_gpu}
	${src_gpu_effects}
	${src_gpu_gl}
	"upstream.patched/src/utils/SkBase64.cpp"
	"upstream.patched/src/utils/SkBitSet.cpp"
	"upstream.patched/src/utils/SkBoundaryPatch.cpp"
	"upstream.patched/src/utils/SkCamera.cpp"
	"upstream.patched/src/utils/SkCondVar.cpp"
	"upstream.patched/src/utils/SkCountdown.cpp"
	"upstream.patched/src/utils/SkCubicInterval.cpp"
	"upstream.patched/src/utils/SkCullPoints.cpp"
	"upstream.patched/src/utils/SkDeferredCanvas.cpp"
	"upstream.patched/src/utils/SkDumpCanvas.cpp"
	"upstream.patched/src/utils/SkInterpolator.cpp"
	"upstream.patched/src/utils/SkJSON.cpp"
	"upstream.patched/src/utils/SkLayer.cpp"
	"upstream.patched/src/utils/SkMatrix44.cpp"
	"upstream.patched/src/utils/SkMeshUtils.cpp"
	"upstream.patched/src/utils/SkNinePatch.cpp"
	"upstream.patched/src/utils/SkNullCanvas.cpp"
	"upstream.patched/src/utils/SkNWayCanvas.cpp"
	"upstream.patched/src/utils/SkOSFile.cpp"
	"upstream.patched/src/utils/SkParse.cpp"
	"upstream.patched/src/utils/SkParseColor.cpp"
	"upstream.patched/src/utils/SkParsePath.cpp"
	"upstream.patched/src/utils/SkPictureUtils.cpp"
	"upstream.patched/src/utils/SkProxyCanvas.cpp"
	"upstream.patched/src/utils/SkThreadPool.cpp"
	"upstream.patched/src/utils/SkUnitMappers.cpp"
	"upstream.patched/src/pipe/SkGPipeRead.cpp"
	"upstream.patched/src/pipe/SkGPipeWrite.cpp"
	"upstream.patched/src/ports/SkGlobalInitialization_default.cpp"
	"upstream.patched/src/ports/SkOSFile_stdio.cpp"
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
	PUBLIC
		${target_specific_definitions_public}
)
target_include_directories(skia_static
	PRIVATE
		"upstream.patched/src/core"
		"upstream.patched/src/config"
		"upstream.patched/src/effects"
		"upstream.patched/src/image"
		"upstream.patched/src/images"
		"upstream.patched/src/ports"
		"upstream.patched/src/utils"
		"upstream.patched/src/xml"
		"upstream.patched/src/sfnt"
		"upstream.patched/src/gpu"
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
	set(ios_iphoneos_exclude_list
		"opts_check_SSE2.cpp"
		"SkBitmapFilter_opts_SSE2.cpp"
		"SkBitmapProcState_opts_SSE2.cpp"
		"SkBitmapProcState_opts_SSSE3.cpp"
		"SkBlitRect_opts_SSE2.cpp"
		"SkBlitRow_opts_SSE2.cpp"
		"SkUtils_opts_SSE2.cpp")
	string(REPLACE ";" " " ios_iphoneos_exclude "${ios_iphoneos_exclude_list}")
	set_target_properties(skia_static
		PROPERTIES
			XCODE_ATTRIBUTE_EXCLUDED_SOURCE_FILE_NAMES[sdk=iphoneos*][arch=*] "${ios_iphoneos_exclude}")

	set(ios_iphonesimulator_exclude_list
		"opts_check_arm.cpp"
		"SkBlitRow_opts_arm.cpp"
		"SkBlitRow_opts_arm_neon.cpp"
		"SkBlitMask_opts_arm.cpp"
		"SkBitmapProcState_opts_arm.cpp"
		"SkBitmapProcState_matrixProcs_neon.cpp"
		"SkBitmapProcState_arm_neon.cpp"
		"memset32_neon.S"
		"memset16_neon.S"
		"memset.arm.S")
	string(REPLACE ";" " " ios_iphonesimulator_exclude "${ios_iphonesimulator_exclude_list}")
	set_target_properties(skia_static
		PROPERTIES
			XCODE_ATTRIBUTE_EXCLUDED_SOURCE_FILE_NAMES[sdk=iphonesimulator*][arch=*] "${ios_iphonesimulator_exclude}")
endif()
