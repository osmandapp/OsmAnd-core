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
add_definitions(
    -Wfatal-errors
    )
#set(target_specific_sources__none
#	"upstream.patched/src/opts/SkBitmapProcState_opts_none.cpp"
#	"upstream.patched/src/opts/SkBlitMask_opts_none.cpp"
#	"upstream.patched/src/opts/SkBlitRow_opts_none.cpp"
#	"upstream.patched/src/opts/SkBlurImage_opts_none.cpp"
#	"upstream.patched/src/opts/SkMorphology_opts_none.cpp"
#	"upstream.patched/src/opts/SkTextureCompression_opts_none.cpp"
#	"upstream.patched/src/opts/SkUtils_opts_none.cpp"
#	"upstream.patched/src/opts/SkXfermode_opts_none.cpp")

set(target_specific_sources__x86 "")
if (CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "x86" AND NOT CMAKE_TARGET_CPU_ARCH_FAMILY STREQUAL "fat")
	# set(used_arch_specific_sources TRUE)
	# set(target_specific_sources__x86
	# 	# x86
	# 	#"upstream.patched/src/opts/opts_check_x86.cpp"
	# 	#"upstream.patched/src/opts/SkTextureCompression_opts_none.cpp"

	# 	# SSE2
	# 	"upstream.patched/src/opts/SkBitmapProcState_opts_SSE2.cpp"
	# 	"upstream.patched/src/opts/SkBitmapFilter_opts_SSE2.cpp"
	# 	"upstream.patched/src/opts/SkBlitRow_opts_SSE2.cpp"
	# 	"upstream.patched/src/opts/SkBlitRect_opts_SSE2.cpp"
	# 	"upstream.patched/src/opts/SkBlurImage_opts_SSE2.cpp"
	# 	"upstream.patched/src/opts/SkMorphology_opts_SSE2.cpp"
	# 	"upstream.patched/src/opts/SkUtils_opts_SSE2.cpp"
	# 	"upstream.patched/src/opts/SkXfermode_opts_SSE2.cpp"
		
	# 	# SSSE3
	# 	"upstream.patched/src/opts/SkBitmapProcState_opts_SSSE3.cpp"
		
	# 	# SSE4
	# 	# SkBlitRow_opts_SSE4_x64_asm.S
	# 	# SkBlitRow_opts_SSE4_asm.S
	# 	"upstream.patched/src/opts/SkBlurImage_opts_SSE4.cpp")
	# set(target_specific_sources "${target_specific_sources}"
	# 	"${target_specific_sources__x86}")
	# if (CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	# 	set_source_files_properties(
	# 		"upstream.patched/src/opts/SkBitmapProcState_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkBitmapFilter_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkBlitRow_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkBlitRect_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkBlurImage_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkMorphology_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkUtils_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkXfermode_opts_SSE2.cpp"
	# 		PROPERTIES
	# 			COMPILE_FLAGS "-msse2")
	# 	set_source_files_properties(
	# 		"upstream.patched/src/opts/SkBitmapProcState_opts_SSSE3.cpp"
	# 		PROPERTIES
	# 			COMPILE_FLAGS "-mssse3")
	# 	set_source_files_properties(
	# 		"upstream.patched/src/opts/SkBlurImage_opts_SSE4.cpp"
	# 		PROPERTIES
	# 			COMPILE_FLAGS "-msse4.1")
	# elseif (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
	# 	set_source_files_properties(
	# 		"upstream.patched/src/opts/SkBitmapProcState_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkBitmapFilter_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkBlitRow_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkBlitRect_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkBlurImage_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkMorphology_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkUtils_opts_SSE2.cpp"
	# 		"upstream.patched/src/opts/SkXfermode_opts_SSE2.cpp"
	# 		PROPERTIES
	# 			COMPILE_FLAGS "/arch:SSE2")
	# 	set_source_files_properties(
	# 		"upstream.patched/src/opts/SkBitmapProcState_opts_SSSE3.cpp"
	# 		PROPERTIES
	# 			COMPILE_FLAGS "/arch:SSE2")
	# 	set_source_files_properties(
	# 		"upstream.patched/src/opts/SkBlurImage_opts_SSE4.cpp"
	# 		PROPERTIES
	# 			COMPILE_FLAGS "/arch:SSE2")
	# endif()
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
endif()

###################################################################################
set(skia_sksl_sources
	"upstream.patched/src/sksl/SkSLASTNode.cpp"
	"upstream.patched/src/sksl/SkSLAnalysis.cpp"
	"upstream.patched/src/sksl/SkSLBuiltinTypes.cpp"
	"upstream.patched/src/sksl/SkSLCPPUniformCTypes.cpp"
	"upstream.patched/src/sksl/SkSLCompiler.cpp"
	"upstream.patched/src/sksl/SkSLConstantFolder.cpp"
	"upstream.patched/src/sksl/SkSLContext.cpp"
	"upstream.patched/src/sksl/SkSLIRGenerator.cpp"
	"upstream.patched/src/sksl/SkSLInliner.cpp"
	"upstream.patched/src/sksl/SkSLLexer.cpp"
	"upstream.patched/src/sksl/SkSLMangler.cpp"
	"upstream.patched/src/sksl/SkSLOperators.cpp"
	"upstream.patched/src/sksl/SkSLOutputStream.cpp"
	"upstream.patched/src/sksl/SkSLParser.cpp"
	"upstream.patched/src/sksl/SkSLPool.cpp"
	"upstream.patched/src/sksl/SkSLRehydrator.cpp"
	"upstream.patched/src/sksl/SkSLSampleUsage.cpp"
	"upstream.patched/src/sksl/SkSLSectionAndParameterHelper.cpp"
	"upstream.patched/src/sksl/SkSLString.cpp"
	"upstream.patched/src/sksl/SkSLUtil.cpp"
	"upstream.patched/src/sksl/codegen/SkSLCPPCodeGenerator.cpp"
	"upstream.patched/src/sksl/codegen/SkSLDSLCPPCodeGenerator.cpp"
	"upstream.patched/src/sksl/codegen/SkSLGLSLCodeGenerator.cpp"
	"upstream.patched/src/sksl/codegen/SkSLHCodeGenerator.cpp"
	"upstream.patched/src/sksl/codegen/SkSLMetalCodeGenerator.cpp"
	"upstream.patched/src/sksl/codegen/SkSLPipelineStageCodeGenerator.cpp"
	"upstream.patched/src/sksl/codegen/SkSLSPIRVCodeGenerator.cpp"
	"upstream.patched/src/sksl/codegen/SkSLSPIRVtoHLSL.cpp"
	"upstream.patched/src/sksl/codegen/SkSLVMCodeGenerator.cpp"
	"upstream.patched/src/sksl/dsl/DSLBlock.cpp"
	"upstream.patched/src/sksl/dsl/DSLCase.cpp"
	"upstream.patched/src/sksl/dsl/DSLCore.cpp"
	"upstream.patched/src/sksl/dsl/DSLExpression.cpp"
	"upstream.patched/src/sksl/dsl/DSLFunction.cpp"
	"upstream.patched/src/sksl/dsl/DSLLayout.cpp"
	"upstream.patched/src/sksl/dsl/DSLStatement.cpp"
	"upstream.patched/src/sksl/dsl/DSLType.cpp"
	"upstream.patched/src/sksl/dsl/DSLVar.cpp"
	"upstream.patched/src/sksl/dsl/priv/DSLFPs.cpp"
	"upstream.patched/src/sksl/dsl/priv/DSLWriter.cpp"
	"upstream.patched/src/sksl/ir/SkSLBinaryExpression.cpp"
	"upstream.patched/src/sksl/ir/SkSLBlock.cpp"
	"upstream.patched/src/sksl/ir/SkSLConstructor.cpp"
	"upstream.patched/src/sksl/ir/SkSLConstructorArray.cpp"
	"upstream.patched/src/sksl/ir/SkSLConstructorCompound.cpp"
	"upstream.patched/src/sksl/ir/SkSLConstructorCompoundCast.cpp"
	"upstream.patched/src/sksl/ir/SkSLConstructorDiagonalMatrix.cpp"
	"upstream.patched/src/sksl/ir/SkSLConstructorMatrixResize.cpp"
	"upstream.patched/src/sksl/ir/SkSLConstructorScalarCast.cpp"
	"upstream.patched/src/sksl/ir/SkSLConstructorSplat.cpp"
	"upstream.patched/src/sksl/ir/SkSLConstructorStruct.cpp"
	"upstream.patched/src/sksl/ir/SkSLDoStatement.cpp"
	"upstream.patched/src/sksl/ir/SkSLExpressionStatement.cpp"
	"upstream.patched/src/sksl/ir/SkSLFieldAccess.cpp"
	"upstream.patched/src/sksl/ir/SkSLForStatement.cpp"
	"upstream.patched/src/sksl/ir/SkSLFunctionCall.cpp"
	"upstream.patched/src/sksl/ir/SkSLFunctionDeclaration.cpp"
	"upstream.patched/src/sksl/ir/SkSLIfStatement.cpp"
	"upstream.patched/src/sksl/ir/SkSLIndexExpression.cpp"
	"upstream.patched/src/sksl/ir/SkSLPostfixExpression.cpp"
	"upstream.patched/src/sksl/ir/SkSLPrefixExpression.cpp"
	"upstream.patched/src/sksl/ir/SkSLSetting.cpp"
	"upstream.patched/src/sksl/ir/SkSLSwitchStatement.cpp"
	"upstream.patched/src/sksl/ir/SkSLSwizzle.cpp"
	"upstream.patched/src/sksl/ir/SkSLSymbolTable.cpp"
	"upstream.patched/src/sksl/ir/SkSLTernaryExpression.cpp"
	"upstream.patched/src/sksl/ir/SkSLType.cpp"
	"upstream.patched/src/sksl/ir/SkSLVarDeclarations.cpp"
	"upstream.patched/src/sksl/ir/SkSLVariable.cpp"
	"upstream.patched/src/sksl/ir/SkSLVariableReference.cpp"
)

set(skia_glsl_sources
	"upstream.patched/src/gpu/glsl/GrGLSL.cpp"
	"upstream.patched/src/gpu/glsl/GrGLSLBlend.cpp"
	"upstream.patched/src/gpu/glsl/GrGLSLFragmentProcessor.cpp"
	"upstream.patched/src/gpu/glsl/GrGLSLFragmentShaderBuilder.cpp"
	"upstream.patched/src/gpu/glsl/GrGLSLGeometryProcessor.cpp"
	"upstream.patched/src/gpu/glsl/GrGLSLProgramBuilder.cpp"
	"upstream.patched/src/gpu/glsl/GrGLSLProgramDataManager.cpp"
	"upstream.patched/src/gpu/glsl/GrGLSLShaderBuilder.cpp"
	"upstream.patched/src/gpu/glsl/GrGLSLUniformHandler.cpp"
	"upstream.patched/src/gpu/glsl/GrGLSLVarying.cpp"
	"upstream.patched/src/gpu/glsl/GrGLSLVertexGeoBuilder.cpp"
	"upstream.patched/src/gpu/glsl/GrGLSLXferProcessor.cpp"
)

set(skia_shader_sources
	"upstream.patched/src/shaders/SkShader.cpp"
	"upstream.patched/src/shaders/SkLocalMatrixShader.cpp"
	"upstream.patched/src/shaders/SkColorFilterShader.cpp"
	"upstream.patched/src/shaders/SkColorShader.cpp"
	"upstream.patched/src/shaders/SkComposeShader.cpp"
	"upstream.patched/src/shaders/SkImageShader.cpp"
	"upstream.patched/src/shaders/SkBitmapProcShader.cpp"
)

set(skia_gpu
	"upstream.patched/src/gpu/ccpr/GrCCAtlas.cpp"
	"upstream.patched/src/gpu/ccpr/GrCCClipPath.cpp"
	"upstream.patched/src/gpu/ccpr/GrCCClipProcessor.cpp"
	"upstream.patched/src/gpu/ccpr/GrCCPerFlushResources.cpp"
	"upstream.patched/src/gpu/ccpr/GrCoverageCountingPathRenderer.cpp"
	"upstream.patched/src/gpu/effects/GrBezierEffect.cpp"
	"upstream.patched/src/gpu/effects/GrBicubicEffect.cpp"
	"upstream.patched/src/gpu/effects/GrBitmapTextGeoProc.cpp"
	"upstream.patched/src/gpu/effects/GrBlendFragmentProcessor.cpp"
	"upstream.patched/src/gpu/effects/GrConvexPolyEffect.cpp"
	"upstream.patched/src/gpu/effects/GrCoverageSetOpXP.cpp"
	"upstream.patched/src/gpu/effects/GrCustomXfermode.cpp"
	"upstream.patched/src/gpu/effects/GrDisableColorXP.cpp"
	"upstream.patched/src/gpu/effects/GrDistanceFieldGeoProc.cpp"
	"upstream.patched/src/gpu/effects/GrGaussianConvolutionFragmentProcessor.cpp"
	"upstream.patched/src/gpu/effects/GrMatrixConvolutionEffect.cpp"
	"upstream.patched/src/gpu/effects/GrMatrixEffect.cpp"
	"upstream.patched/src/gpu/effects/GrOvalEffect.cpp"
	"upstream.patched/src/gpu/effects/GrPorterDuffXferProcessor.cpp"
	"upstream.patched/src/gpu/effects/GrRRectEffect.cpp"
	"upstream.patched/src/gpu/effects/GrShadowGeoProc.cpp"
	"upstream.patched/src/gpu/effects/GrSkSLFP.cpp"
	"upstream.patched/src/gpu/effects/GrTextureEffect.cpp"
	"upstream.patched/src/gpu/effects/GrYUVtoRGBEffect.cpp"
	"upstream.patched/src/gpu/effects/generated/GrAARectEffect.cpp"
	"upstream.patched/src/gpu/effects/generated/GrBlurredEdgeFragmentProcessor.cpp"
	"upstream.patched/src/gpu/effects/generated/GrCircleBlurFragmentProcessor.cpp"
	"upstream.patched/src/gpu/effects/generated/GrCircleEffect.cpp"
	"upstream.patched/src/gpu/effects/generated/GrClampFragmentProcessor.cpp"
	"upstream.patched/src/gpu/effects/generated/GrColorMatrixFragmentProcessor.cpp"
	"upstream.patched/src/gpu/effects/generated/GrConfigConversionEffect.cpp"
	"upstream.patched/src/gpu/effects/generated/GrDeviceSpaceEffect.cpp"
	"upstream.patched/src/gpu/effects/generated/GrDitherEffect.cpp"
	"upstream.patched/src/gpu/effects/generated/GrEllipseEffect.cpp"
	"upstream.patched/src/gpu/effects/generated/GrOverrideInputFragmentProcessor.cpp"
	"upstream.patched/src/gpu/effects/generated/GrRRectBlurEffect.cpp"
	"upstream.patched/src/gpu/effects/generated/GrRectBlurEffect.cpp"
	"upstream.patched/src/gpu/geometry/GrPathUtils.cpp"
	"upstream.patched/src/gpu/geometry/GrQuad.cpp"
	"upstream.patched/src/gpu/geometry/GrQuadUtils.cpp"
	"upstream.patched/src/gpu/geometry/GrShape.cpp"
	"upstream.patched/src/gpu/geometry/GrStyledShape.cpp"
	"upstream.patched/src/gpu/mock/GrMockTypes.cpp"
	"upstream.patched/src/gpu/ops/GrAAConvexPathRenderer.cpp"
	"upstream.patched/src/gpu/ops/GrAAConvexTessellator.cpp"
	"upstream.patched/src/gpu/ops/GrAAHairLinePathRenderer.cpp"
	"upstream.patched/src/gpu/ops/GrAALinearizingConvexPathRenderer.cpp"
	"upstream.patched/src/gpu/ops/GrAtlasTextOp.cpp"
	"upstream.patched/src/gpu/ops/GrClearOp.cpp"
	"upstream.patched/src/gpu/ops/GrDashLinePathRenderer.cpp"
	"upstream.patched/src/gpu/ops/GrDashOp.cpp"
	"upstream.patched/src/gpu/ops/GrDefaultPathRenderer.cpp"
	"upstream.patched/src/gpu/ops/GrDrawAtlasOp.cpp"
	"upstream.patched/src/gpu/ops/GrDrawVerticesOp.cpp"
	"upstream.patched/src/gpu/ops/GrDrawableOp.cpp"
	"upstream.patched/src/gpu/ops/GrFillRRectOp.cpp"
	"upstream.patched/src/gpu/ops/GrFillRectOp.cpp"
	"upstream.patched/src/gpu/ops/GrLatticeOp.cpp"
	"upstream.patched/src/gpu/ops/GrMeshDrawOp.cpp"
	"upstream.patched/src/gpu/ops/GrOp.cpp"
	"upstream.patched/src/gpu/ops/GrOvalOpFactory.cpp"
	"upstream.patched/src/gpu/ops/GrQuadPerEdgeAA.cpp"
	"upstream.patched/src/gpu/ops/GrRegionOp.cpp"
	"upstream.patched/src/gpu/ops/GrShadowRRectOp.cpp"
	"upstream.patched/src/gpu/ops/GrSimpleMeshDrawOpHelper.cpp"
	"upstream.patched/src/gpu/ops/GrSimpleMeshDrawOpHelperWithStencil.cpp"
	"upstream.patched/src/gpu/ops/GrSmallPathAtlasMgr.cpp"
	"upstream.patched/src/gpu/ops/GrSmallPathRenderer.cpp"
	"upstream.patched/src/gpu/ops/GrSmallPathShapeData.cpp"
	"upstream.patched/src/gpu/ops/GrStrokeRectOp.cpp"
	"upstream.patched/src/gpu/ops/GrTextureOp.cpp"
	"upstream.patched/src/gpu/ops/GrTriangulatingPathRenderer.cpp"
	"upstream.patched/src/gpu/tessellate/GrDrawAtlasPathOp.cpp"
	"upstream.patched/src/gpu/tessellate/GrFillPathShader.cpp"
	"upstream.patched/src/gpu/tessellate/GrPathInnerTriangulateOp.cpp"
	"upstream.patched/src/gpu/tessellate/GrPathStencilFillOp.cpp"
	"upstream.patched/src/gpu/tessellate/GrPathTessellator.cpp"
	"upstream.patched/src/gpu/tessellate/GrStencilPathShader.cpp"
	"upstream.patched/src/gpu/tessellate/GrStrokeFixedCountTessellator.cpp"
	"upstream.patched/src/gpu/tessellate/GrStrokeHardwareTessellator.cpp"
	"upstream.patched/src/gpu/tessellate/GrStrokeIndirectTessellator.cpp"
	"upstream.patched/src/gpu/tessellate/GrStrokeInstancedShaderImpl.cpp"
	"upstream.patched/src/gpu/tessellate/GrStrokeShader.cpp"
	"upstream.patched/src/gpu/tessellate/GrStrokeTessellateOp.cpp"
	"upstream.patched/src/gpu/tessellate/GrStrokeTessellationShaderImpl.cpp"
	"upstream.patched/src/gpu/tessellate/GrTessellationPathRenderer.cpp"
	"upstream.patched/src/gpu/text/GrAtlasManager.cpp"
	"upstream.patched/src/gpu/text/GrDistanceFieldAdjustTable.cpp"
	"upstream.patched/src/gpu/text/GrSDFMaskFilter.cpp"
	"upstream.patched/src/gpu/text/GrSDFTControl.cpp"
	"upstream.patched/src/gpu/text/GrStrikeCache.cpp"
	"upstream.patched/src/gpu/text/GrTextBlob.cpp"
	"upstream.patched/src/gpu/text/GrTextBlobCache.cpp"
)

set(skia_codec
"upstream.patched/src/codec/SkCodecImageGenerator.cpp"
"upstream.patched/src/codec/SkCodec.cpp"
"upstream.patched/src/codec/SkMasks.cpp"
"upstream.patched/src/codec/SkBmpCodec.cpp"
"upstream.patched/src/codec/SkWbmpCodec.cpp"
"upstream.patched/src/codec/SkSwizzler.cpp"
"upstream.patched/src/codec/SkBmpMaskCodec.cpp"
"upstream.patched/src/codec/SkBmpRLECodec.cpp"
"upstream.patched/src/codec/SkBmpStandardCodec.cpp"
"upstream.patched/src/codec/SkSampler.cpp"
"upstream.patched/src/codec/SkBmpBaseCodec.cpp"
"upstream.patched/src/codec/SkMaskSwizzler.cpp"
"upstream.patched/src/codec/SkColorTable.cpp"
)
###################################################################################

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
		"upstream.patched/include/third_party/skcms"
	)
	#file(GLOB src_utils_ios "upstream.patched/src/utils/ios/*")
	file(GLOB src_gpu_gl_ios "upstream.patched/src/gpu/gl/iOS/*")
	
	# set(skia_sksl_sources
	# 	"upstream.patched/src/sksl/SkSLASTNode.cpp"
	# 	"upstream.patched/src/sksl/SkSLAnalysis.cpp"
	# 	"upstream.patched/src/sksl/SkSLBuiltinTypes.cpp"
	# 	"upstream.patched/src/sksl/SkSLCPPUniformCTypes.cpp"
	# 	"upstream.patched/src/sksl/SkSLCompiler.cpp"
	# 	"upstream.patched/src/sksl/SkSLConstantFolder.cpp"
	# 	"upstream.patched/src/sksl/SkSLContext.cpp"
	# 	"upstream.patched/src/sksl/SkSLIRGenerator.cpp"
	# 	"upstream.patched/src/sksl/SkSLInliner.cpp"
	# 	"upstream.patched/src/sksl/SkSLLexer.cpp"
	# 	"upstream.patched/src/sksl/SkSLMangler.cpp"
	# 	"upstream.patched/src/sksl/SkSLOperators.cpp"
	# 	"upstream.patched/src/sksl/SkSLOutputStream.cpp"
	# 	"upstream.patched/src/sksl/SkSLParser.cpp"
	# 	"upstream.patched/src/sksl/SkSLPool.cpp"
	# 	"upstream.patched/src/sksl/SkSLRehydrator.cpp"
	# 	"upstream.patched/src/sksl/SkSLSampleUsage.cpp"
	# 	"upstream.patched/src/sksl/SkSLSectionAndParameterHelper.cpp"
	# 	"upstream.patched/src/sksl/SkSLString.cpp"
	# 	"upstream.patched/src/sksl/SkSLUtil.cpp"
	# 	"upstream.patched/src/sksl/codegen/SkSLCPPCodeGenerator.cpp"
	# 	"upstream.patched/src/sksl/codegen/SkSLDSLCPPCodeGenerator.cpp"
	# 	"upstream.patched/src/sksl/codegen/SkSLGLSLCodeGenerator.cpp"
	# 	"upstream.patched/src/sksl/codegen/SkSLHCodeGenerator.cpp"
	# 	"upstream.patched/src/sksl/codegen/SkSLMetalCodeGenerator.cpp"
	# 	"upstream.patched/src/sksl/codegen/SkSLPipelineStageCodeGenerator.cpp"
	# 	"upstream.patched/src/sksl/codegen/SkSLSPIRVCodeGenerator.cpp"
	# 	"upstream.patched/src/sksl/codegen/SkSLSPIRVtoHLSL.cpp"
	# 	"upstream.patched/src/sksl/codegen/SkSLVMCodeGenerator.cpp"
	# 	"upstream.patched/src/sksl/dsl/DSLBlock.cpp"
	# 	"upstream.patched/src/sksl/dsl/DSLCase.cpp"
	# 	"upstream.patched/src/sksl/dsl/DSLCore.cpp"
	# 	"upstream.patched/src/sksl/dsl/DSLExpression.cpp"
	# 	"upstream.patched/src/sksl/dsl/DSLFunction.cpp"
	# 	"upstream.patched/src/sksl/dsl/DSLLayout.cpp"
	# 	"upstream.patched/src/sksl/dsl/DSLStatement.cpp"
	# 	"upstream.patched/src/sksl/dsl/DSLType.cpp"
	# 	"upstream.patched/src/sksl/dsl/DSLVar.cpp"
	# 	"upstream.patched/src/sksl/dsl/priv/DSLFPs.cpp"
	# 	"upstream.patched/src/sksl/dsl/priv/DSLWriter.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLBinaryExpression.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLBlock.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLConstructor.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLConstructorArray.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLConstructorCompound.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLConstructorCompoundCast.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLConstructorDiagonalMatrix.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLConstructorMatrixResize.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLConstructorScalarCast.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLConstructorSplat.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLConstructorStruct.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLDoStatement.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLExpressionStatement.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLFieldAccess.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLForStatement.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLFunctionCall.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLFunctionDeclaration.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLIfStatement.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLIndexExpression.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLPostfixExpression.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLPrefixExpression.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLSetting.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLSwitchStatement.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLSwizzle.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLSymbolTable.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLTernaryExpression.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLType.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLVarDeclarations.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLVariable.cpp"
	# 	"upstream.patched/src/sksl/ir/SkSLVariableReference.cpp"
	# )
	
	# set(skia_glsl_sources
	# 	"upstream.patched/src/gpu/glsl/GrGLSL.cpp"
	# 	"upstream.patched/src/gpu/glsl/GrGLSLBlend.cpp"
	# 	"upstream.patched/src/gpu/glsl/GrGLSLFragmentProcessor.cpp"
	# 	"upstream.patched/src/gpu/glsl/GrGLSLFragmentShaderBuilder.cpp"
	# 	"upstream.patched/src/gpu/glsl/GrGLSLGeometryProcessor.cpp"
	# 	"upstream.patched/src/gpu/glsl/GrGLSLProgramBuilder.cpp"
	# 	"upstream.patched/src/gpu/glsl/GrGLSLProgramDataManager.cpp"
	# 	"upstream.patched/src/gpu/glsl/GrGLSLShaderBuilder.cpp"
	# 	"upstream.patched/src/gpu/glsl/GrGLSLUniformHandler.cpp"
	# 	"upstream.patched/src/gpu/glsl/GrGLSLVarying.cpp"
	# 	"upstream.patched/src/gpu/glsl/GrGLSLVertexGeoBuilder.cpp"
	# 	"upstream.patched/src/gpu/glsl/GrGLSLXferProcessor.cpp"
	# )

	# set(skia_shader_sources
	# 	"upstream.patched/src/shaders/SkShader.cpp"
	# 	"upstream.patched/src/shaders/SkLocalMatrixShader.cpp"
	# 	"upstream.patched/src/shaders/SkColorFilterShader.cpp"
	# 	"upstream.patched/src/shaders/SkColorShader.cpp"
	# 	"upstream.patched/src/shaders/SkComposeShader.cpp"
	# 	"upstream.patched/src/shaders/SkImageShader.cpp"
	# 	"upstream.patched/src/shaders/SkBitmapProcShader.cpp"
	# )

	# set(skia_gpu
	# 	"upstream.patched/src/gpu/ccpr/GrCCAtlas.cpp"
	# 	"upstream.patched/src/gpu/ccpr/GrCCClipPath.cpp"
	# 	"upstream.patched/src/gpu/ccpr/GrCCClipProcessor.cpp"
	# 	"upstream.patched/src/gpu/ccpr/GrCCPerFlushResources.cpp"
	# 	"upstream.patched/src/gpu/ccpr/GrCoverageCountingPathRenderer.cpp"
	# 	"upstream.patched/src/gpu/effects/GrBezierEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/GrBicubicEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/GrBitmapTextGeoProc.cpp"
	# 	"upstream.patched/src/gpu/effects/GrBlendFragmentProcessor.cpp"
	# 	"upstream.patched/src/gpu/effects/GrConvexPolyEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/GrCoverageSetOpXP.cpp"
	# 	"upstream.patched/src/gpu/effects/GrCustomXfermode.cpp"
	# 	"upstream.patched/src/gpu/effects/GrDisableColorXP.cpp"
	# 	"upstream.patched/src/gpu/effects/GrDistanceFieldGeoProc.cpp"
	# 	"upstream.patched/src/gpu/effects/GrGaussianConvolutionFragmentProcessor.cpp"
	# 	"upstream.patched/src/gpu/effects/GrMatrixConvolutionEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/GrMatrixEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/GrOvalEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/GrPorterDuffXferProcessor.cpp"
	# 	"upstream.patched/src/gpu/effects/GrRRectEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/GrShadowGeoProc.cpp"
	# 	"upstream.patched/src/gpu/effects/GrSkSLFP.cpp"
	# 	"upstream.patched/src/gpu/effects/GrTextureEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/GrYUVtoRGBEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/generated/GrAARectEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/generated/GrBlurredEdgeFragmentProcessor.cpp"
	# 	"upstream.patched/src/gpu/effects/generated/GrCircleBlurFragmentProcessor.cpp"
	# 	"upstream.patched/src/gpu/effects/generated/GrCircleEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/generated/GrClampFragmentProcessor.cpp"
	# 	"upstream.patched/src/gpu/effects/generated/GrColorMatrixFragmentProcessor.cpp"
	# 	"upstream.patched/src/gpu/effects/generated/GrConfigConversionEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/generated/GrDeviceSpaceEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/generated/GrDitherEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/generated/GrEllipseEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/generated/GrOverrideInputFragmentProcessor.cpp"
	# 	"upstream.patched/src/gpu/effects/generated/GrRRectBlurEffect.cpp"
	# 	"upstream.patched/src/gpu/effects/generated/GrRectBlurEffect.cpp"
	# 	"upstream.patched/src/gpu/geometry/GrPathUtils.cpp"
	# 	"upstream.patched/src/gpu/geometry/GrQuad.cpp"
	# 	"upstream.patched/src/gpu/geometry/GrQuadUtils.cpp"
	# 	"upstream.patched/src/gpu/geometry/GrShape.cpp"
	# 	"upstream.patched/src/gpu/geometry/GrStyledShape.cpp"
	# 	"upstream.patched/src/gpu/mock/GrMockTypes.cpp"
	# 	"upstream.patched/src/gpu/ops/GrAAConvexPathRenderer.cpp"
	# 	"upstream.patched/src/gpu/ops/GrAAConvexTessellator.cpp"
	# 	"upstream.patched/src/gpu/ops/GrAAHairLinePathRenderer.cpp"
	# 	"upstream.patched/src/gpu/ops/GrAALinearizingConvexPathRenderer.cpp"
	# 	"upstream.patched/src/gpu/ops/GrAtlasTextOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrClearOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrDashLinePathRenderer.cpp"
	# 	"upstream.patched/src/gpu/ops/GrDashOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrDefaultPathRenderer.cpp"
	# 	"upstream.patched/src/gpu/ops/GrDrawAtlasOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrDrawVerticesOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrDrawableOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrFillRRectOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrFillRectOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrLatticeOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrMeshDrawOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrOvalOpFactory.cpp"
	# 	"upstream.patched/src/gpu/ops/GrQuadPerEdgeAA.cpp"
	# 	"upstream.patched/src/gpu/ops/GrRegionOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrShadowRRectOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrSimpleMeshDrawOpHelper.cpp"
	# 	"upstream.patched/src/gpu/ops/GrSimpleMeshDrawOpHelperWithStencil.cpp"
	# 	"upstream.patched/src/gpu/ops/GrSmallPathAtlasMgr.cpp"
	# 	"upstream.patched/src/gpu/ops/GrSmallPathRenderer.cpp"
	# 	"upstream.patched/src/gpu/ops/GrSmallPathShapeData.cpp"
	# 	"upstream.patched/src/gpu/ops/GrStrokeRectOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrTextureOp.cpp"
	# 	"upstream.patched/src/gpu/ops/GrTriangulatingPathRenderer.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrDrawAtlasPathOp.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrFillPathShader.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrPathInnerTriangulateOp.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrPathStencilFillOp.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrPathTessellator.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrStencilPathShader.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrStrokeFixedCountTessellator.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrStrokeHardwareTessellator.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrStrokeIndirectTessellator.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrStrokeInstancedShaderImpl.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrStrokeShader.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrStrokeTessellateOp.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrStrokeTessellationShaderImpl.cpp"
	# 	"upstream.patched/src/gpu/tessellate/GrTessellationPathRenderer.cpp"
	# 	"upstream.patched/src/gpu/text/GrAtlasManager.cpp"
	# 	"upstream.patched/src/gpu/text/GrDistanceFieldAdjustTable.cpp"
	# 	"upstream.patched/src/gpu/text/GrSDFMaskFilter.cpp"
	# 	"upstream.patched/src/gpu/text/GrSDFTControl.cpp"
	# 	"upstream.patched/src/gpu/text/GrStrikeCache.cpp"
	# 	"upstream.patched/src/gpu/text/GrTextBlob.cpp"
	# 	"upstream.patched/src/gpu/text/GrTextBlobCache.cpp"
	# )

	# set(skia_codec
	# 	"upstream.patched/src/codec/SkCodecImageGenerator.cpp"
	# 	"upstream.patched/src/codec/SkCodec.cpp"
	# 	"upstream.patched/src/codec/SkMasks.cpp"
	# 	"upstream.patched/src/codec/SkBmpCodec.cpp"
	# 	"upstream.patched/src/codec/SkWbmpCodec.cpp"
	# 	"upstream.patched/src/codec/SkSwizzler.cpp"
	# 	"upstream.patched/src/codec/SkBmpMaskCodec.cpp"
	# 	"upstream.patched/src/codec/SkBmpRLECodec.cpp"
	# 	"upstream.patched/src/codec/SkBmpStandardCodec.cpp"
	# 	"upstream.patched/src/codec/SkSampler.cpp"
	# 	"upstream.patched/src/codec/SkBmpBaseCodec.cpp"
	# 	"upstream.patched/src/codec/SkMaskSwizzler.cpp"
	# 	"upstream.patched/src/codec/SkColorTable.cpp"
	# )

	set(target_specific_sources "${target_specific_sources}"
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkDiscardableMemory_none.cpp"
		#"upstream.patched/src/ports/SkGlobalInitialization_default.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		"upstream.patched/src/ports/SkFontMgr_mac_ct_factory.cpp"
		"upstream.patched/src/ports/SkFontMgr_mac_ct.cpp"
		"upstream.patched/src/ports/SkTypeface_mac_ct.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread.cpp"
		"upstream.patched/src/utils/mac/SkCreateCGImageRef.cpp"
		"upstream.patched/src/utils/mac/SkCTFont.cpp"
		"upstream.patched/src/ports/SkScalerContext_mac_ct.cpp"
		"upstream.patched/third_party/skcms/skcms.cc"
		${src_gpu_gl_ios}
		${skia_sksl_sources}
		${skia_glsl_sources}
		${skia_shader_sources}
		${skia_gpu}
		${skia_codec}
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_IOS
		-DSK_SUPPORT_LEGACY_DRAWLOOPER
		-DSK_DISABLE_EFFECT_DESERIALIZATION
	)
	set(target_specific_definitions_private ${target_specific_definitions_private}
		-DSK_USE_POSIX_THREADS
		-DSK_GL
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
		"upstream.patched/src/ports/SkFontMgr_custom_directory_factory.cpp"
		"upstream.patched/src/ports/SkFontMgr_custom_directory.cpp"
		"upstream.patched/src/ports/SkFontMgr_custom.cpp"
		"upstream.patched/src/core/SkFontMgr.cpp"
		"upstream.patched/src/ports/SkDebug_stdio.cpp"
		"upstream.patched/src/ports/SkDiscardableMemory_none.cpp"
		"upstream.patched/src/ports/SkFontConfigInterface_direct.cpp"
		#"upstream.patched/src/ports/SkFontHost_fontconfig.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType_common.cpp"
		"upstream.patched/src/ports/SkFontHost_FreeType.cpp"
		#"upstream.patched/src/ports/SkFontHost_linux.cpp"
		"upstream.patched/src/ports/SkOSFile_posix.cpp"
		# "upstream.patched/src/ports/SkTime_Unix.cpp"
		"upstream.patched/src/ports/SkTLS_pthread.cpp"
		"upstream.patched/src/utils/SkThreadUtils_pthread.cpp"
		# "upstream.patched/src/utils/SkThreadUtils_pthread_linux.cpp"
		"upstream.patched/third_party/skcms/skcms.cc"
		"upstream.patched/src/gpu/mock/GrMockGpu.cpp"
		"upstream.patched/src/gpu/GrCaps.cpp"
		"upstream.patched/src/gpu/mock/GrMockCaps.cpp"
		#"upstream.patched/src/ports/SkGlobalInitialization_default.cpp"
		${src_gpu_gl_glx}
		${skia_sksl_sources}
		${skia_glsl_sources}
		${skia_shader_sources}
		${skia_gpu}
		${skia_codec}
	)
	set(target_specific_definitions_public ${target_specific_definitions_public}
		-DSK_BUILD_FOR_UNIX
		-DSK_SUPPORT_LEGACY_DRAWLOOPER
		-DSK_DISABLE_EFFECT_DESERIALIZATION
		-DSK_BUILD_NO_OPTS
		-DSK_FONTHOST_USES_FONTMGR
		-DUDATA_DEBUG
	)
	set(target_specific_definitions_private ${target_specific_definitions_private}
		-DSK_USE_POSIX_THREADS
		-DSK_GL
		-DSK_BUILD_NO_OPTS
	)
	#set(target_specific_dependencies ${target_specific_dependencies}
	#	freetype2_static
	#)
	find_package(Freetype REQUIRED)
	find_library(PTHREAD_LIBRARY pthread)
	find_library(FONTCONFIG_LIBRARY fontconfig)
	set(target_specific_libraries_public ${target_specific_libraries_public}
		#freetype2_static
		${PTHREAD_LIBRARY}
		${FONTCONFIG_LIBRARY}
		${FREETYPE_LIBRARIES}
	)

	set(target_specific_include_dirs_public ${target_specific_include_dirs_public}
		"upstream.patched/include/third_party/skcms"
		"upstream.patched/include/utils"
		"upstream.patched/third_party/externals/freetype/devel"
		"upstream.patched/third_party/externals/freetype/include"
		# "upstream.patched/externals/freetype/include"
		# "upstream.patched/externals/freetype/src"
		${FREETYPE_INCLUDE_DIRS}
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
		"upstream.patched/src/gpu/mock/GrMockCaps.cpp"
		#"upstream.patched/src/ports/SkGlobalInitialization_default.cpp"
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
#file(GLOB src_images__remove
#	"upstream.patched/src/images/SkImageDecoder_astc.*"
#	"upstream.patched/src/images/SkImageDecoder_ktx.*"
#	"upstream.patched/src/images/SkImageDecoder_libwebp.*"
#	"upstream.patched/src/images/SkImageDecoder_pkm.*"
#	"upstream.patched/src/images/SkImageRef_ashmem.*"
#	"upstream.patched/src/images/SkForceLinking.*")
#list(REMOVE_ITEM src_images ${src_images__remove})
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
#file(GLOB src_gpu_effects "upstream.patched/src/gpu/effects/*")
file(GLOB src_gpu_gl "upstream.patched/src/gpu/gl/*")
#file(GLOB src_gpu_gl__remove
#	"upstream.patched/src/gpu/gl/GrGLCreateNativeInterface_none.cpp"
#	"upstream.patched/src/gpu/gl/GrGLDefaultInterface_none.cpp")
#list(REMOVE_ITEM src_gpu_gl ${src_gpu_gl__remove})
file(GLOB src_gpu_gl_builders "upstream.patched/src/gpu/gl/builders/*")
file(GLOB src_utils "upstream.patched/src/utils/*")
#file(GLOB src_utils__remove
#	"upstream.patched/src/utils/SkLua.*"
#	"upstream.patched/src/utils/SkLuaCanvas.*"
#	"upstream.patched/src/utils/SkThreadUtils_*")
#list(REMOVE_ITEM src_utils ${src_utils__remove})
file(GLOB src_utils_debugger "upstream.patched/src/utils/debugger/*")
if (NOT CMAKE_TARGET_OS STREQUAL "ios")
	file(GLOB src_non_ios__remove "upstream.patched/src/gpu/mtl/*")
	#message(WARNING "src_non_ios__remove=${src_non_ios__remove}")
	list(REMOVE_ITEM src_gpu ${src_non_ios__remove})
	file(GLOB src_non_ios__remove "upstream.patched/src/*/*.mm")
	#message(FATAL_ERROR "src_non_ios__remove=${src_non_ios__remove}")
	list(REMOVE_ITEM src_gpu ${src_non_ios__remove})
	file(GLOB src_image__remove "upstream.patched/src/image/*.mm")
	#message(FATAL_ERROR "src_image__remove=${src_image__remove}")
	list(REMOVE_ITEM src_image ${src_image__remove})
	
endif()

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
		"upstream.patched"
		"upstream.patched/include/gpu"
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
		"upstream.patched"
		"upstream.patched/include/core"
		"upstream.patched/src/shaders"		
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
			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES")

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
