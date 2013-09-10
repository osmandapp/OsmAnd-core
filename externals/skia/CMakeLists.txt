project(skia)

set(target_specific_sources
	"upstream.patched/src/opts/SkBlitRow_opts_none.cpp"
	#TODO: This is bad, use hw-specific
	"upstream.patched/src/opts/SkBlitMask_opts_none.cpp"
	"upstream.patched/src/opts/SkBitmapProcState_opts_none.cpp"
	"upstream.patched/src/opts/SkUtils_opts_none.cpp"
	"upstream.patched/src/ports/SkGlobalInitialization_default.cpp"
	"upstream.patched/src/ports/SkOSFile_stdio.cpp"
	"upstream.patched/src/ports/SkFontHost_sandbox_none.cpp"
)

set(target_specific_include_dirs_private "")
set(target_specific_include_dirs_public "")
set(target_specific_definitions_public "")
if(CMAKE_TARGET_OS STREQUAL "windows")
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
		-DSK_SUPPORT_GPU=1
	)
elseif(CMAKE_TARGET_OS STREQUAL "darwin")
	set(target_specific_include_dirs_public ${target_specific_include_dirs_public}
		"upstream.patched/include/utils/mac"
	)
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkThread_pthread.cpp"
		"upstream.patched/src/ports/SkTime_Unix.cpp"
		"upstream.patched/src/ports/SkImageDecoder_CG.cpp"
		"upstream.patched/src/ports/SkFontHost_mac.cpp"
		"upstream.patched/src/utils/mac/SkStream_mac.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_MAC
		-DSK_USE_CORETEXT
		-DSK_SUPPORT_GPU=0
	)
elseif(CMAKE_TARGET_OS STREQUAL "ios")
	set(target_specific_include_dirs_public ${target_specific_include_dirs_public}
		"upstream.patched/include/utils/mac"
		"upstream.patched/include/utils/ios"
	)
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkThread_pthread.cpp"
		"upstream.patched/src/ports/SkTime_Unix.cpp"
		"upstream.patched/src/ports/SkImageDecoder_CG.cpp"
		"upstream.patched/src/ports/SkFontHost_mac.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		"upstream.patched/src/utils/mac/SkStream_mac.cpp"
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_IOS
		-DSK_USE_CORETEXT
		-DSK_SUPPORT_GPU=0
	)
elseif(CMAKE_TARGET_OS STREQUAL "linux")
	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkFontHost_sandbox_none.cpp"
		"upstream.patched/src/ports/SkThread_pthread.cpp"
		"upstream.patched/src/ports/SkTime_Unix.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType_common.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType.cpp"
		"upstream.patched/src/ports/SkFontHost_linux.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_UNIX
		-DSK_SUPPORT_GPU=0
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
	"upstream.patched/src/gpu/GrAddPathRenderers*"
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
	"upstream.patched/src/gpu/GrAddPathRenderers_default.cpp"
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
	${target_specific_sources}
)

add_dependencies(skia_static
	expat_static
	freetype2_static
	harfbuzz_static
	png_static
	gif_static
	jpeg_static
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
		freetype2_static
		harfbuzz_static
		png_static
		gif_static
		jpeg_static
)
	
if(CMAKE_TARGET_OS STREQUAL "linux")
	target_link_libraries(skia_static
		LINK_PUBLIC
			pthread
	)
elseif(CMAKE_TARGET_OS STREQUAL "darwin" OR CMAKE_TARGET_OS STREQUAL "ios")
	find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
	find_library(CORE_TEXT_LIBRARY CoreText)
	find_library(CORE_GRAPHICS_LIBRARY CoreGraphics)
	target_link_libraries(skia_static
		LINK_PUBLIC
			${CORE_FOUNDATION_LIBRARY}
			${CORE_TEXT_LIBRARY}
			${CORE_GRAPHICS_LIBRARY}
	)
elseif(CMAKE_TARGET_OS STREQUAL "windows")
	target_link_libraries(skia_static
		LINK_PUBLIC
			Usp10
	)
endif()
