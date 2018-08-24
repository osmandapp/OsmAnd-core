LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

PROJECT_ROOT_RELATIVE := ../../../../platforms/android/OsmAnd
OSMAND_SKIA_ROOT_RELATIVE := ../../../externals/skia
OSMAND_SKIA_ROOT := $(LOCAL_PATH)/$(OSMAND_SKIA_ROOT_RELATIVE)
OSMAND_SKIA_RELATIVE := ../../../externals/skia/upstream.patched
OSMAND_SKIA := $(LOCAL_PATH)/$(OSMAND_SKIA_RELATIVE)

OSMAND_FREETYPE_RELATIVE := ../../../externals/skia/upstream.patched/third_party/externals/freetype
OSMAND_FREETYPE := $(LOCAL_PATH)/$(OSMAND_FREETYPE_RELATIVE)

OSMAND_LIBPNG_RELATIVE := ../../../externals/skia/upstream.patched/third_party/libpng
OSMAND_LIBPNG := $(LOCAL_PATH)/$(OSMAND_LIBPNG_RELATIVE)

OSMAND_JPEG_RELATIVE := ../../../externals/skia/upstream.patched/third_party/externals/libjpeg-turbo
OSMAND_JPEG := $(LOCAL_PATH)/$(OSMAND_JPEG_RELATIVE)

OSMAND_EXPAT_RELATIVE := ../../../externals/skia/upstream.patched/third_party/externals/expat
OSMAND_EXPAT := $(LOCAL_PATH)/$(OSMAND_EXPAT_RELATIVE)

LOCAL_C_INCLUDES += \
	${OSMAND_SKIA}/src/c \
	${OSMAND_SKIA}/src/core \
	${OSMAND_SKIA}/src/codec \
	${OSMAND_SKIA}/src/config \
	${OSMAND_SKIA}/src/effects \
    ${OSMAND_SKIA}/src/effects/gradients \
	${OSMAND_SKIA}/src/fonts \
	${OSMAND_SKIA}/src/image \
	${OSMAND_SKIA}/src/images \
	${OSMAND_SKIA}/src/lazy \
	${OSMAND_SKIA}/src/ports \
    ${OSMAND_SKIA}/src/sksl \
	${OSMAND_SKIA}/src/utils \
	${OSMAND_SKIA}/src/xml \
	${OSMAND_SKIA}/src/sfnt \
	${OSMAND_SKIA}/src/gpu \
	${OSMAND_SKIA}/src/opts \
    ${OSMAND_SKIA}/src/pathops \
    ${OSMAND_SKIA}/include/android \
	${OSMAND_SKIA}/include/c \
	${OSMAND_SKIA}/include/core \
	${OSMAND_SKIA}/include/codec \
	${OSMAND_SKIA}/include/lazy \
	${OSMAND_SKIA}/include/utils/mac \
	${OSMAND_SKIA}/include/utils/win \
	${OSMAND_SKIA}/include/pathops \
	${OSMAND_SKIA}/include/private \
	${OSMAND_SKIA}/include/config \
	${OSMAND_SKIA}/include/effects \
	${OSMAND_SKIA}/include/images \
	${OSMAND_SKIA}/include/pipe \
	${OSMAND_SKIA}/include/ports \
	${OSMAND_SKIA}/include/utils \
	${OSMAND_SKIA}/include/xml \
	${OSMAND_SKIA}/include/gpu \
	${OSMAND_SKIA}/include/gpu/gl \
	$(OSMAND_EXPAT)/lib \
	$(OSMAND_FREETYPE)/include \
	$(OSMAND_GIF) \
	$(OSMAND_JPEG) \
	$(OSMAND_LIBPNG)

# core
LOCAL_SRC_FILES += \
	$(OSMAND_SKIA_RELATIVE)/src/c/sk_surface.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/c/sk_types_priv.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/Sk4px.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkAAClip.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkAnnotation.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkAdvancedTypefaceMetrics.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkAlphaRuns.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkAntiRun.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkATrace.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkATrace.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkAutoKern.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkAutoPixmapStorage.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkAutoPixmapStorage.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBBHFactory.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBBoxHierarchy.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBigPicture.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmap.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapCache.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapController.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapDevice.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapDevice.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapFilter.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcShader.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcState.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcState.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcState_filter.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcState_matrix.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcState_matrix_template.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcState_matrixProcs.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcState_procs.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcState_sample.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcState_shaderproc.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcState_utils.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProvider.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProvider.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapScaler.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapScaler.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitBWMaskTemplate.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitMask.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitMask_D32.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitRow.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitRow_D16.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitRow_D32.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter_A8.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter_ARGB32.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter_PM4f.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter_RGB16.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter_Sprite.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlurImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBuffer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkCachedData.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkCanvas.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkCanvasPriv.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkClipStack.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkClipStack.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkClipStackDevice.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkClipStackDevice.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColor.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorFilterShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorLookUpTable.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorLookUpTable.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorMatrixFilterRowMajor255.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorMatrixFilterRowMajor255.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorShader.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorSpace.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorSpace_A2B.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorSpace_A2B.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorSpace_XYZ.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorSpace_XYZ.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorSpace_ICC.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorSpaceXform.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorSpaceXformCanvas.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorSpaceXformer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorSpaceXformer.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorSpaceXform_A2B.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorSpaceXform_A2B.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorTable.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkComposeShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkConvertPixels.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkConvertPixels.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkConvolver.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkConvolver.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkCoreBlitters.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkCpu.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkCpu.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkCubicClipper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkCubicClipper.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkData.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDataTable.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDebug.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDeque.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDescriptor.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDevice.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDevice.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDeviceLooper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDeviceProfile.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDiscardableMemory.h \
	$(OSMAND_SKIA_RELATIVE)/src/lazy/SkDiscardableMemoryPool.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDistanceFieldGen.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDistanceFieldGen.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDither.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDither.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDocument.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDraw.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDraw.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDrawable.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDrawLooper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDrawProcs.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkEdgeBuilder.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkEdgeBuilder.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkEdgeClipper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkEdgeClipper.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkEmptyShader.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkEndian.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkExecutor.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkAnalyticEdge.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFDot6Constants.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkEdge.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkEdge.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFDot6.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFilterProc.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFilterProc.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFindAndPlaceGlyph.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkArenaAlloc.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkArenaAlloc.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFlattenable.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFlattenableSerialization.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFont.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFontLCDConfig.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFontMgr.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFontStyle.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFontDescriptor.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFontDescriptor.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFontStream.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFontStream.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFuzzLogging.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkGeometry.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkGeometry.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkGlobalInitialization_core.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkGlyph.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkGlyphCache.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkGlyphCache.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkGlyphCache_Globals.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkGpuBlurUtils.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkGpuBlurUtils.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkGraphics.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkHalf.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkHalf.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkICC.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkImageFilterCache.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkImageFilterCache.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkImageInfo.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkImageCacherator.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkImageCacherator.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkImageGenerator.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLightingShader.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLightingShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLights.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLinearBitmapPipeline.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLinearBitmapPipeline.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLinearBitmapPipeline_core.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLinearBitmapPipeline_matrix.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLinearBitmapPipeline_tile.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLinearBitmapPipeline_sample.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLineClipper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLiteDL.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLiteRecorder.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLocalMatrixImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLocalMatrixImageFilter.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLocalMatrixShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMD5.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMD5.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMallocPixelRef.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMask.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMaskCache.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMaskFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMaskGamma.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMaskGamma.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMath.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMathPriv.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMatrix.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMatrix44.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMatrixImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMatrixImageFilter.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMatrixUtils.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMetaData.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMipMap.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMipMap.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMiniRecorder.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkModeColorFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMultiPictureDraw.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkNextID.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLatticeIter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLatticeIter.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkNormalBevelSource.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkNormalBevelSource.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkNormalMapSource.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkNormalMapSource.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkNormalFlatSource.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkNormalFlatSource.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkNormalSource.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkNormalSource.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkNormalSourcePriv.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkNx.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkOpts.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkOpts.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkOrderedReadBuffer.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkOSFile.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkOverdrawCanvas.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkOverdrawCanvas.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPaint.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPaintDefaults.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPaintPriv.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPaintPriv.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPath.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPathEffect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPathMeasure.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPathPriv.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPathRef.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPerspIter.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPicture.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureAnalyzer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureCommon.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureContentInfo.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureContentInfo.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureData.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureData.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureFlat.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureFlat.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureImageGenerator.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPicturePlayback.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPicturePlayback.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureRecord.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureRecord.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureRecorder.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureShader.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPixelRef.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPixmap.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPoint.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPoint3.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPtrRecorder.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkQuadClipper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkQuadClipper.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRadialShadowMapShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRadialShadowMapShader.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRasterClip.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRasterPipeline.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRasterPipelineBlitter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRasterizer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkReadBuffer.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkReadBuffer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkReader32.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRecord.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRecords.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRecordDraw.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRecordOpts.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRecordOpts.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRecordPattern.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRecordedDrawable.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRecorder.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRefDict.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRegion.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRegionPriv.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRegion_path.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkResourceCache.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRRect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRTree.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRTree.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRWBuffer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScalar.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScalerContext.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScalerContext.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScan.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScan.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScanPriv.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScan_AAAPath.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScan_AntiPath.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScan_Antihair.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScan_Hairline.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScan_Path.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSemaphore.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSharedMutex.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSharedMutex.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSinglyLinkedList.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSpanProcs.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSpecialImage.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSpecialImage.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSpecialSurface.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSpecialSurface.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSpinlock.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSpriteBlitter_ARGB32.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSpriteBlitter_RGB16.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSpriteBlitter.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSpriteBlitterTemplate.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSpriteBlitter4f.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkStream.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkStreamPriv.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkString.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkStringUtils.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkStroke.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkStroke.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkStrokeRec.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkStrokerPriv.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkStrokerPriv.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSurfacePriv.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSwizzle.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSRGB.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTaskGroup.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTaskGroup.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTDPQueue.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTDynamicHash.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTInternalLList.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTextBlob.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTextFormatParams.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTextMapStateProc.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTextToPathIter.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTime.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTDPQueue.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkThreadID.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTLList.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTLS.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTMultiMap.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTraceEvent.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTraceEventCommon.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTSearch.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTSort.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTTopoSort.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTypeface.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTypefaceCache.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTypefaceCache.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTypefacePriv.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkUnPreMultiply.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkUtils.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkUtils.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkValidatingReadBuffer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkValidatingReadBuffer.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkValidationUtils.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkVarAlloc.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkVertices.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkVertState.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkWriteBuffer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkWriter32.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkXfermode.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkXfermode4f.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkXfermodeF16.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkXfermode_proccoeff.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkXfermodeInterpretation.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkXfermodeInterpretation.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkYUVPlanesCache.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkYUVPlanesCache.h \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkShadowShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkShadowShader.h \
	$(OSMAND_SKIA_RELATIVE)/src/image/SkImage.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/image/SkImage_Generator.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/image/SkImage_Raster.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/image/SkImageShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/image/SkImageShader.h \
	$(OSMAND_SKIA_RELATIVE)/src/image/SkSurface.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/image/SkSurface_Base.h \
	$(OSMAND_SKIA_RELATIVE)/src/image/SkSurface_Raster.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pipe/SkPipeCanvas.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pipe/SkPipeReader.cpp \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkBBHFactory.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkBitmap.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkCanvas.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkColor.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkColorFilter.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkColorPriv.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkCrossContextImageData.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkData.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkDeque.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkDrawable.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkDrawFilter.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkDrawLooper.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkFlattenable.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkFlattenableSerialization.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkFontArguments.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkFontLCDConfig.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkFontStyle.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkGraphics.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkImage.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkImageEncoder.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkImageFilter.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkImageInfo.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkLights.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkMallocPixelRef.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkMask.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkMaskFilter.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkMath.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkMatrix.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkMatrix44.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkMetaData.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkMultiPictureDraw.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkPaint.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkPath.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkPathEffect.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkPathMeasure.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkPathRef.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkPicture.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkPictureAnalyzer.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkPictureRecorder.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkPixelRef.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkPoint.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkPoint3.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkPreConfig.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkRasterizer.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkRect.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkRefCnt.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkRegion.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkRRect.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkScalar.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkShader.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkStream.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkString.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkStrokeRec.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkSurface.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkSwizzle.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkTextBlob.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkTime.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkTLazy.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkTypeface.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkTypes.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkUnPreMultiply.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkVertices.h \
	$(OSMAND_SKIA_RELATIVE)/include/core/SkWriter32.h \
	# private
	$(OSMAND_SKIA_RELATIVE)/include/private/SkAtomics.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkChecksum.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkFixed.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkFloatBits.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkFloatingPoint.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkMalloc.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkMessageBus.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkMiniRecorder.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkMutex.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkOnce.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkRecords.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkSemaphore.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkShadowFlags.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkShadowParams.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkSpinlock.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkTemplates.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkTArray.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkTDArray.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkTFitsIn.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkTHash.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkThreadID.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkTSearch.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkTLogic.h \
	$(OSMAND_SKIA_RELATIVE)/include/private/SkWeakRefCnt.h \
	# pathops
	$(OSMAND_SKIA_RELATIVE)/include/pathops/SkPathOps.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkAddIntersections.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkDConicLineIntersection.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkDCubicLineIntersection.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkDCubicToQuads.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkDLineIntersection.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkDQuadLineIntersection.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkIntersections.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpAngle.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpBuilder.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpCoincidence.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpContour.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpCubicHull.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpEdgeBuilder.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpSegment.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpSpan.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsCommon.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsConic.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsCubic.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsCurve.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsDebug.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsLine.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsOp.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsPoint.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsQuad.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsRect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsSimplify.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsTSect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsTightBounds.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsTypes.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsWinding.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathWriter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkReduceOrder.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkAddIntersections.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkIntersectionHelper.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkIntersections.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkLineParameters.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpAngle.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpCoincidence.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpContour.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpEdgeBuilder.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpSegment.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpSpan.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkOpTAllocator.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsBounds.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsCommon.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsConic.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsCubic.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsCurve.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsDebug.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsLine.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsPoint.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsQuad.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsRect.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsTSect.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathOpsTypes.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkPathWriter.h \
	$(OSMAND_SKIA_RELATIVE)/src/pathops/SkReduceOrder.h 

# utils
LOCAL_SRC_FILES += \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkFrontBufferedStream.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkCamera.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkCanvasStateUtils.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkDumpCanvas.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkEventTracer.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkInterpolator.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkNoDrawCanvas.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkNWayCanvas.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkNullCanvas.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkPaintFilterCanvas.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkParse.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkParsePath.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkRandom.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkShadowUtils.h \
  $(OSMAND_SKIA_RELATIVE)/include/utils/SkTextBox.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkBase64.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkBase64.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkBitmapSourceDeserializer.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkBitmapSourceDeserializer.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkBitSet.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkFrontBufferedStream.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkCamera.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkCanvasStack.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkCanvasStack.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkCanvasStateUtils.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkCurveMeasure.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkCurveMeasure.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkDashPath.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkDashPathPriv.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkDeferredCanvas.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkDumpCanvas.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkEventTracer.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkFloatUtils.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkInsetConvexPolygon.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkInsetConvexPolygon.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkInterpolator.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkMatrix22.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkMatrix22.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkMultiPictureDocument.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkNWayCanvas.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkNullCanvas.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkOSPath.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkOSPath.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkPaintFilterCanvas.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkParse.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkParseColor.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkParsePath.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkPatchUtils.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkPatchUtils.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkRGBAToYUV.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkRGBAToYUV.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkShadowPaintFilterCanvas.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkShadowPaintFilterCanvas.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkShadowTessellator.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkShadowTessellator.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkShadowUtils.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkTextBox.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkThreadUtils.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkThreadUtils_pthread.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkThreadUtils_pthread.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkThreadUtils_win.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkThreadUtils_win.h \
  $(OSMAND_SKIA_RELATIVE)/src/utils/SkWhitelistTypefaces.cpp 

# effects
LOCAL_SRC_FILES += \
  $(OSMAND_SKIA_RELATIVE)/src/c/sk_effects.cpp \

  $(OSMAND_SKIA_RELATIVE)/src/effects/GrCircleBlurFragmentProcessor.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/GrCircleBlurFragmentProcessor.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/GrAlphaThresholdFragmentProcessor.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/GrAlphaThresholdFragmentProcessor.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/Sk1DPathEffect.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/Sk2DPathEffect.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkAlphaThresholdFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkArcToPathEffect.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkArithmeticImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkArithmeticMode.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkBlurMask.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkBlurMask.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkBlurMaskFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkColorFilterImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkColorMatrix.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkColorMatrixFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkComposeImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkCornerPathEffect.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkDashPathEffect.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkDiscretePathEffect.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkDisplacementMapEffect.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkDropShadowImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkEmbossMask.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkEmbossMask.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkEmbossMask_Table.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkEmbossMaskFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkImageSource.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkGaussianEdgeShader.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkHighContrastFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkLayerDrawLooper.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkLayerRasterizer.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkLightingImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkLumaColorFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkMagnifierImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkMatrixConvolutionImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkMergeImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkMorphologyImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkOffsetImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkOverdrawColorFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkOverdrawColorFilter.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkPackBits.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkPackBits.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkPaintFlagsDrawFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkPaintImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkPerlinNoiseShader.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkPictureImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkRRectsGaussianEdgeMaskFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkTableColorFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkTableMaskFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkTileImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/SkXfermodeImageFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/Sk4fGradientBase.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/Sk4fGradientBase.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/Sk4fGradientPriv.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/Sk4fLinearGradient.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/Sk4fLinearGradient.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkClampRange.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkClampRange.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkGradientBitmapCache.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkGradientBitmapCache.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkGradientShader.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkGradientShaderPriv.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkLinearGradient.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkLinearGradient.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkRadialGradient.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkRadialGradient.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkTwoPointConicalGradient.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkTwoPointConicalGradient.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkTwoPointConicalGradient_gpu.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkTwoPointConicalGradient_gpu.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkSweepGradient.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkSweepGradient.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/shadows/SkAmbientShadowMaskFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/shadows/SkAmbientShadowMaskFilter.h \
  $(OSMAND_SKIA_RELATIVE)/src/effects/shadows/SkSpotShadowMaskFilter.cpp \
  $(OSMAND_SKIA_RELATIVE)/src/effects/shadows/SkSpotShadowMaskFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/Sk1DPathEffect.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/Sk2DPathEffect.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkAlphaThresholdFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkArithmeticImageFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkBlurDrawLooper.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkBlurImageFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkBlurMaskFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkColorFilterImageFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkColorMatrix.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkColorMatrixFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkCornerPathEffect.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkDashPathEffect.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkDiscretePathEffect.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkDisplacementMapEffect.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkDropShadowImageFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkGaussianEdgeShader.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkGradientShader.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkImageSource.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkLayerDrawLooper.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkLayerRasterizer.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkLightingImageFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkLumaColorFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkMagnifierImageFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkMorphologyImageFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkOffsetImageFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkPaintFlagsDrawFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkPaintImageFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkPerlinNoiseShader.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkRRectsGaussianEdgeMaskFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkTableColorFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkTableMaskFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkTileImageFilter.h \
  $(OSMAND_SKIA_RELATIVE)/include/effects/SkXfermodeImageFilter.h 

LOCAL_SRC_FILES += \
    $(OSMAND_SKIA_RELATIVE)/src/android/SkBitmapRegionCodec.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/android/SkBitmapRegionDecoder.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkAndroidCodec.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkBmpCodec.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkBmpMaskCodec.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkBmpRLECodec.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkBmpStandardCodec.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkCodec.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkCodecImageGenerator.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkGifCodec.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkMaskSwizzler.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkMasks.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkSampledCodec.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkSampler.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkStreamBuffer.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkSwizzler.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkWbmpCodec.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/images/SkImageEncoder.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/ports/SkDiscardableMemory_none.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/ports/SkImageGenerator_skia.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/ports/SkMemory_malloc.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/ports/SkOSFile_stdio.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/sfnt/SkOTTable_name.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/sfnt/SkOTUtils.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/utils/mac/SkStream_mac.cpp \
    $(OSMAND_SKIA_RELATIVE)/third_party/etc1/etc1.cpp \
    $(OSMAND_SKIA_RELATIVE)/third_party/gif/SkGifImageReader.cpp \

    $(OSMAND_SKIA_RELATIVE)/src/codec/SkIcoCodec.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkPngCodec.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/images/SkPNGImageEncoder.cpp \

    $(OSMAND_SKIA_RELATIVE)/src/codec/SkJpegCodec.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkJpegDecoderMgr.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/codec/SkJpegUtility.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/images/SkJPEGImageEncoder.cpp \
    $(OSMAND_SKIA_RELATIVE)/src/images/SkJPEGWriteUtility.cpp \

    $(OSMAND_SKIA_RELATIVE)/src/ports/SkGlobalInitialization_none.cpp

# Android specific files
LOCAL_SRC_FILES += \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkDebug_android.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkDiscardableMemory_none.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkFontConfigParser_android.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkFontHost_FreeType_common.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkFontHost_FreeType.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkFontMgr_android.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkOSFile_posix.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkTLS_pthread.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkThreadUtils_pthread.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkThreadUtils_pthread_other.cpp

	
LOCAL_CFLAGS += \
	-DSK_BUILD_FOR_ANDROID \
	-DANDROID_LARGE_MEMORY_DEVICE \
	-DSK_USE_POSIX_THREADS \
	-DSK_IGNORE_ETC1_SUPPORT \
	-DSKRELEASE \
	-DSK_HAS_JPEG_LIBRARY \
	-DSK_HAS_WEBP_LIBRARY \
	-DSK_HAS_PNG_LIBRARY \
	-DSK_SUPPORT_GPU=0

LOCAL_CPPFLAGS := \
	-fno-rtti \
	-fno-exceptions
	
LOCAL_MODULE := osmand_skia

ifneq ($(OSMAND_USE_PREBUILT),true)

LOCAL_ARM_MODE := arm

ifeq ($(TARGET_ARCH),arm64)
	#LOCAL_CFLAGS += \
	#	-DSK_ARM_HAS_OPTIONAL_NEON
	# ARM
	LOCAL_SRC_FILES += \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBitmapProcState_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlitMask_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlitRow_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlurImage_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkMorphology_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkTextureCompression_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkUtils_opts_none.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkXfermode_opts_arm.cpp

	# NEON
	LOCAL_SRC_FILES += \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBitmapProcState_arm_neon.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBitmapProcState_matrixProcs_neon.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlitMask_opts_arm_neon.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlitRow_opts_arm_neon.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlurImage_opts_neon.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkMorphology_opts_neon.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkTextureCompression_opts_neon.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkXfermode_opts_arm_neon.cpp

else ifeq ($(TARGET_ARCH),arm)
	#LOCAL_CFLAGS += \
	#	-DSK_ARM_HAS_OPTIONAL_NEON

	# ARM
	LOCAL_SRC_FILES += \
		$(OSMAND_SKIA_RELATIVE)/src/opts/memset.arm.S \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBitmapProcState_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlitMask_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlitRow_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlurImage_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkMorphology_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkTextureCompression_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkUtils_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkXfermode_opts_arm.cpp

	# NEON
	#LOCAL_SRC_FILES += \
	#	$(OSMAND_SKIA_RELATIVE)/src/opts/memset16_neon.S \
	#	$(OSMAND_SKIA_RELATIVE)/src/opts/memset32_neon.S \
	#	$(OSMAND_SKIA_RELATIVE)/src/opts/SkBitmapProcState_arm_neon.cpp \
	#	$(OSMAND_SKIA_RELATIVE)/src/opts/SkBitmapProcState_matrixProcs_neon.cpp \
	#	$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlitMask_opts_arm_neon.cpp \
	#	$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlitRow_opts_arm_neon.cpp \
	#	$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlurImage_opts_neon.cpp \
	#	$(OSMAND_SKIA_RELATIVE)/src/opts/SkMorphology_opts_neon.cpp \
	#	$(OSMAND_SKIA_RELATIVE)/src/opts/SkTextureCompression_opts_neon.cpp \
	#	$(OSMAND_SKIA_RELATIVE)/src/opts/SkXfermode_opts_arm_neon.cpp
else

	LOCAL_SRC_FILES += \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBitmapProcState_opts_none.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlitMask_opts_none.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlitRow_opts_none.cpp \
		
endif

LOCAL_STATIC_LIBRARIES += \
	osmand_jpeg \
	osmand_ft2 \
	osmand_png \
	osmand_expat \
	cpufeatures

LOCAL_LDLIBS += -lz -llog

include $(BUILD_STATIC_LIBRARY)

$(call import-module,android/cpufeatures)

else
	LOCAL_SRC_FILES := \
		$(PROJECT_ROOT_RELATIVE)/libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
	include $(PREBUILT_STATIC_LIBRARY)
endif
