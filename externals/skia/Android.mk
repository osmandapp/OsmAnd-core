LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
    LOCAL_MODULE := osmand_skia
else
    LOCAL_MODULE := osmand_skia_neon
    LOCAL_ARM_NEON := true
endif

LOCAL_EXPORT_CFLAGS := \
    -DSK_BUILD_FOR_ANDROID

LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/upstream.patched/include/core \
    $(LOCAL_PATH)/upstream.patched/include/lazy \
    $(LOCAL_PATH)/upstream.patched/include/pathops \
    $(LOCAL_PATH)/upstream.patched/src/core \
    $(LOCAL_PATH)/upstream.patched/include/config \
    $(LOCAL_PATH)/upstream.patched/include/effects \
    $(LOCAL_PATH)/upstream.patched/include/images \
    $(LOCAL_PATH)/upstream.patched/include/pipe \
    $(LOCAL_PATH)/upstream.patched/include/ports \
    $(LOCAL_PATH)/upstream.patched/include/utils \
    $(LOCAL_PATH)/upstream.patched/include/xml \
    $(LOCAL_PATH)/upstream.patched/include/gpu \
    $(LOCAL_PATH)/upstream.patched/include/utils/android

ifeq ($(LOCAL_ARM_NEON),true)
    OSMAND_BINARY_SUFFIX := _neon
else
    OSMAND_BINARY_SUFFIX :=
endif

LOCAL_STATIC_LIBRARIES := \
    osmand_jpeg$(OSMAND_BINARY_SUFFIX) \
    osmand_ft2$(OSMAND_BINARY_SUFFIX) \
    osmand_png$(OSMAND_BINARY_SUFFIX) \
    osmand_gif$(OSMAND_BINARY_SUFFIX) \
    osmand_expat$(OSMAND_BINARY_SUFFIX)

LOCAL_EXPORT_LDLIBS := \
    -llog \
    -lEGL \
    -lGLESv2

ifneq ($(OSMAND_USE_PREBUILT),true)
    LOCAL_CFLAGS := \
        $(LOCAL_EXPORT_CFLAGS)

    LOCAL_C_INCLUDES := \
        $(LOCAL_EXPORT_C_INCLUDES) \
        $(LOCAL_PATH)/upstream.patched/src/core \
        $(LOCAL_PATH)/upstream.patched/src/config \
        $(LOCAL_PATH)/upstream.patched/src/effects \
        $(LOCAL_PATH)/upstream.patched/src/image \
        $(LOCAL_PATH)/upstream.patched/src/images \
        $(LOCAL_PATH)/upstream.patched/src/ports \
        $(LOCAL_PATH)/upstream.patched/src/utils \
        $(LOCAL_PATH)/upstream.patched/src/xml \
        $(LOCAL_PATH)/upstream.patched/src/sfnt \
        $(LOCAL_PATH)/upstream.patched/src/gpu \
        $(LOCAL_PATH)/upstream.patched/src/utils/android

    # need a flag to tell the C side when we're on devices with large memory
    # budgets (i.e. larger than the low-end devices that initially shipped)
    ifeq ($(ARCH_ARM_HAVE_VFP),true)
        LOCAL_CFLAGS += -DANDROID_LARGE_MEMORY_DEVICE
    endif

    ifneq ($(ARCH_ARM_HAVE_VFP),true)
        LOCAL_CFLAGS += -DSK_SOFTWARE_FLOAT
    endif

    ifeq ($(LOCAL_ARM_NEON),true)
        LOCAL_CFLAGS += -D__ARM_HAVE_NEON
    endif

    LOCAL_ARM_MODE := arm
    LOCAL_SRC_FILES := \
        upstream.patched/src/core/Sk64.cpp \
        upstream.patched/src/core/SkAAClip.cpp \
        upstream.patched/src/core/SkAdvancedTypefaceMetrics.cpp \
        upstream.patched/src/core/SkAlphaRuns.cpp \
        upstream.patched/src/core/SkAnnotation.cpp \
        upstream.patched/src/core/SkBBoxHierarchy.cpp \
        upstream.patched/src/core/SkBBoxHierarchyRecord.cpp \
        upstream.patched/src/core/SkBBoxRecord.cpp \
        upstream.patched/src/core/SkBitmap.cpp \
        upstream.patched/src/core/SkBitmapDevice.cpp \
        upstream.patched/src/core/SkBitmapFilter.cpp \
        upstream.patched/src/core/SkBitmapHeap.cpp \
        upstream.patched/src/core/SkBitmapProcShader.cpp \
        upstream.patched/src/core/SkBitmapProcState.cpp \
        upstream.patched/src/core/SkBitmapProcState_matrixProcs.cpp \
        upstream.patched/src/core/SkBitmapScaler.cpp \
        upstream.patched/src/core/SkBitmap_scroll.cpp \
        upstream.patched/src/core/SkBlitMask_D32.cpp \
        upstream.patched/src/core/SkBlitRow_D16.cpp \
        upstream.patched/src/core/SkBlitRow_D32.cpp \
        upstream.patched/src/core/SkBlitter.cpp \
        upstream.patched/src/core/SkBlitter_A1.cpp \
        upstream.patched/src/core/SkBlitter_A8.cpp \
        upstream.patched/src/core/SkBlitter_ARGB32.cpp \
        upstream.patched/src/core/SkBlitter_RGB16.cpp \
        upstream.patched/src/core/SkBlitter_Sprite.cpp \
        upstream.patched/src/core/SkBuffer.cpp \
        upstream.patched/src/core/SkCanvas.cpp \
        upstream.patched/src/core/SkChunkAlloc.cpp \
        upstream.patched/src/core/SkClipStack.cpp \
        upstream.patched/src/core/SkColor.cpp \
        upstream.patched/src/core/SkColorFilter.cpp \
        upstream.patched/src/core/SkColorTable.cpp \
        upstream.patched/src/core/SkComposeShader.cpp \
        upstream.patched/src/core/SkConfig8888.cpp \
        upstream.patched/src/core/SkConvolver.cpp \
        upstream.patched/src/core/SkCordic.cpp \
        upstream.patched/src/core/SkCubicClipper.cpp \
        upstream.patched/src/core/SkData.cpp \
        upstream.patched/src/core/SkDataTable.cpp \
        upstream.patched/src/core/SkDebug.cpp \
        upstream.patched/src/core/SkDeque.cpp \
        upstream.patched/src/core/SkDevice.cpp \
        upstream.patched/src/core/SkDeviceLooper.cpp \
        upstream.patched/src/core/SkDeviceProfile.cpp \
        upstream.patched/src/core/SkDither.cpp \
        upstream.patched/src/core/SkDraw.cpp \
        upstream.patched/src/core/SkDrawLooper.cpp \
        upstream.patched/src/core/SkEdge.cpp \
        upstream.patched/src/core/SkEdgeBuilder.cpp \
        upstream.patched/src/core/SkEdgeClipper.cpp \
        upstream.patched/src/core/SkError.cpp \
        upstream.patched/src/core/SkFilterProc.cpp \
        upstream.patched/src/core/SkFilterShader.cpp \
        upstream.patched/src/core/SkFlate.cpp \
        upstream.patched/src/core/SkFlattenable.cpp \
        upstream.patched/src/core/SkFlattenableBuffers.cpp \
        upstream.patched/src/core/SkFlattenableSerialization.cpp \
        upstream.patched/src/core/SkFloat.cpp \
        upstream.patched/src/core/SkFloatBits.cpp \
        upstream.patched/src/core/SkFontDescriptor.cpp \
        upstream.patched/src/core/SkFontHost.cpp \
        upstream.patched/src/core/SkFontStream.cpp \
        upstream.patched/src/core/SkGeometry.cpp \
        upstream.patched/src/core/SkGlyphCache.cpp \
        upstream.patched/src/core/SkGraphics.cpp \
        upstream.patched/src/core/SkImageFilter.cpp \
        upstream.patched/src/core/SkImageFilterUtils.cpp \
        upstream.patched/src/core/SkInstCnt.cpp \
        upstream.patched/src/core/SkLineClipper.cpp \
        upstream.patched/src/core/SkMallocPixelRef.cpp \
        upstream.patched/src/core/SkMask.cpp \
        upstream.patched/src/core/SkMaskFilter.cpp \
        upstream.patched/src/core/SkMaskGamma.cpp \
        upstream.patched/src/core/SkMath.cpp \
        upstream.patched/src/core/SkMatrix.cpp \
        upstream.patched/src/core/SkMemory_stdlib.cpp \
        upstream.patched/src/core/SkMetaData.cpp \
        upstream.patched/src/core/SkMipMap.cpp \
        upstream.patched/src/core/SkOrderedReadBuffer.cpp \
        upstream.patched/src/core/SkOrderedWriteBuffer.cpp \
        upstream.patched/src/core/SkPackBits.cpp \
        upstream.patched/src/core/SkPaint.cpp \
        upstream.patched/src/core/SkPaintOptionsAndroid.cpp \
        upstream.patched/src/core/SkPaintPriv.cpp \
        upstream.patched/src/core/SkPath.cpp \
        upstream.patched/src/core/SkPathEffect.cpp \
        upstream.patched/src/core/SkPathHeap.cpp \
        upstream.patched/src/core/SkPathMeasure.cpp \
        upstream.patched/src/core/SkPicture.cpp \
        upstream.patched/src/core/SkPictureFlat.cpp \
        upstream.patched/src/core/SkPicturePlayback.cpp \
        upstream.patched/src/core/SkPictureRecord.cpp \
        upstream.patched/src/core/SkPictureStateTree.cpp \
        upstream.patched/src/core/SkPixelRef.cpp \
        upstream.patched/src/core/SkPoint.cpp \
        upstream.patched/src/core/SkProcSpriteBlitter.cpp \
        upstream.patched/src/core/SkPtrRecorder.cpp \
        upstream.patched/src/core/SkQuadClipper.cpp \
        upstream.patched/src/core/SkRRect.cpp \
        upstream.patched/src/core/SkRTree.cpp \
        upstream.patched/src/core/SkRasterClip.cpp \
        upstream.patched/src/core/SkRasterizer.cpp \
        upstream.patched/src/core/SkRect.cpp \
        upstream.patched/src/core/SkRefCnt.cpp \
        upstream.patched/src/core/SkRefDict.cpp \
        upstream.patched/src/core/SkRegion.cpp \
        upstream.patched/src/core/SkRegion_path.cpp \
        upstream.patched/src/core/SkRegion_rects.cpp \
        upstream.patched/src/core/SkScalar.cpp \
        upstream.patched/src/core/SkScaledImageCache.cpp \
        upstream.patched/src/core/SkScalerContext.cpp \
        upstream.patched/src/core/SkScan.cpp \
        upstream.patched/src/core/SkScan_AntiPath.cpp \
        upstream.patched/src/core/SkScan_Antihair.cpp \
        upstream.patched/src/core/SkScan_Hairline.cpp \
        upstream.patched/src/core/SkScan_Path.cpp \
        upstream.patched/src/core/SkShader.cpp \
        upstream.patched/src/core/SkSpriteBlitter_ARGB32.cpp \
        upstream.patched/src/core/SkSpriteBlitter_RGB16.cpp \
        upstream.patched/src/core/SkStream.cpp \
        upstream.patched/src/core/SkString.cpp \
        upstream.patched/src/core/SkStringUtils.cpp \
        upstream.patched/src/core/SkStroke.cpp \
        upstream.patched/src/core/SkStrokeRec.cpp \
        upstream.patched/src/core/SkStrokerPriv.cpp \
        upstream.patched/src/core/SkTLS.cpp \
        upstream.patched/src/core/SkTSearch.cpp \
        upstream.patched/src/core/SkTileGrid.cpp \
        upstream.patched/src/core/SkTileGridPicture.cpp \
        upstream.patched/src/core/SkTypeface.cpp \
        upstream.patched/src/core/SkTypefaceCache.cpp \
        upstream.patched/src/core/SkUnPreMultiply.cpp \
        upstream.patched/src/core/SkUtils.cpp \
        upstream.patched/src/core/SkUtilsArm.cpp \
        upstream.patched/src/core/SkWriter32.cpp \
        upstream.patched/src/core/SkXfermode.cpp \
        upstream.patched/src/effects/Sk1DPathEffect.cpp \
        upstream.patched/src/effects/Sk2DPathEffect.cpp \
        upstream.patched/src/effects/SkArithmeticMode.cpp \
        upstream.patched/src/effects/SkAvoidXfermode.cpp \
        upstream.patched/src/effects/SkBicubicImageFilter.cpp \
        upstream.patched/src/effects/SkBitmapAlphaThresholdShader.cpp \
        upstream.patched/src/effects/SkBitmapSource.cpp \
        upstream.patched/src/effects/SkBlurDrawLooper.cpp \
        upstream.patched/src/effects/SkBlurImageFilter.cpp \
        upstream.patched/src/effects/SkBlurMask.cpp \
        upstream.patched/src/effects/SkBlurMaskFilter.cpp \
        upstream.patched/src/effects/SkColorFilterImageFilter.cpp \
        upstream.patched/src/effects/SkColorFilters.cpp \
        upstream.patched/src/effects/SkColorMatrix.cpp \
        upstream.patched/src/effects/SkColorMatrixFilter.cpp \
        upstream.patched/src/effects/SkComposeImageFilter.cpp \
        upstream.patched/src/effects/SkCornerPathEffect.cpp \
        upstream.patched/src/effects/SkDashPathEffect.cpp \
        upstream.patched/src/effects/SkDiscretePathEffect.cpp \
        upstream.patched/src/effects/SkDisplacementMapEffect.cpp \
        upstream.patched/src/effects/SkDropShadowImageFilter.cpp \
        upstream.patched/src/effects/SkEmbossMask.cpp \
        upstream.patched/src/effects/SkEmbossMaskFilter.cpp \
        upstream.patched/src/effects/SkGpuBlurUtils.cpp \
        upstream.patched/src/effects/SkKernel33MaskFilter.cpp \
        upstream.patched/src/effects/SkLayerDrawLooper.cpp \
        upstream.patched/src/effects/SkLayerRasterizer.cpp \
        upstream.patched/src/effects/SkLerpXfermode.cpp \
        upstream.patched/src/effects/SkLightingImageFilter.cpp \
        upstream.patched/src/effects/SkLumaXfermode.cpp \
        upstream.patched/src/effects/SkMagnifierImageFilter.cpp \
        upstream.patched/src/effects/SkMatrixConvolutionImageFilter.cpp \
        upstream.patched/src/effects/SkMergeImageFilter.cpp \
        upstream.patched/src/effects/SkMorphologyImageFilter.cpp \
        upstream.patched/src/effects/SkOffsetImageFilter.cpp \
        upstream.patched/src/effects/SkPaintFlagsDrawFilter.cpp \
        upstream.patched/src/effects/SkPerlinNoiseShader.cpp \
        upstream.patched/src/effects/SkPixelXorXfermode.cpp \
        upstream.patched/src/effects/SkPorterDuff.cpp \
        upstream.patched/src/effects/SkRectShaderImageFilter.cpp \
        upstream.patched/src/effects/SkStippleMaskFilter.cpp \
        upstream.patched/src/effects/SkTableColorFilter.cpp \
        upstream.patched/src/effects/SkTableMaskFilter.cpp \
        upstream.patched/src/effects/SkTestImageFilters.cpp \
        upstream.patched/src/effects/SkTransparentShader.cpp \
        upstream.patched/src/effects/SkXfermodeImageFilter.cpp \
        upstream.patched/src/effects/gradients/SkBitmapCache.cpp \
        upstream.patched/src/effects/gradients/SkClampRange.cpp \
        upstream.patched/src/effects/gradients/SkGradientShader.cpp \
        upstream.patched/src/effects/gradients/SkLinearGradient.cpp \
        upstream.patched/src/effects/gradients/SkRadialGradient.cpp \
        upstream.patched/src/effects/gradients/SkSweepGradient.cpp \
        upstream.patched/src/effects/gradients/SkTwoPointConicalGradient.cpp \
        upstream.patched/src/effects/gradients/SkTwoPointRadialGradient.cpp \
        upstream.patched/src/image/SkDataPixelRef.cpp \
        upstream.patched/src/image/SkImage.cpp \
        upstream.patched/src/image/SkImagePriv.cpp \
        upstream.patched/src/image/SkImage_Codec.cpp \
        upstream.patched/src/image/SkImage_Gpu.cpp \
        upstream.patched/src/image/SkImage_Picture.cpp \
        upstream.patched/src/image/SkImage_Raster.cpp \
        upstream.patched/src/image/SkSurface.cpp \
        upstream.patched/src/image/SkSurface_Gpu.cpp \
        upstream.patched/src/image/SkSurface_Picture.cpp \
        upstream.patched/src/image/SkSurface_Raster.cpp \
        upstream.patched/src/images/SkForceLinking.cpp \
        upstream.patched/src/images/SkImageDecoder.cpp \
        upstream.patched/src/images/SkImageDecoder_FactoryDefault.cpp \
        upstream.patched/src/images/SkImageDecoder_FactoryRegistrar.cpp \
        upstream.patched/src/images/SkImageDecoder_libbmp.cpp \
        upstream.patched/src/images/SkImageDecoder_libgif.cpp \
        upstream.patched/src/images/SkImageDecoder_libico.cpp \
        upstream.patched/src/images/SkImageDecoder_libjpeg.cpp \
        upstream.patched/src/images/SkImageDecoder_libpng.cpp \
        upstream.patched/src/images/SkImageDecoder_wbmp.cpp \
        upstream.patched/src/images/SkImageEncoder.cpp \
        upstream.patched/src/images/SkImageEncoder_Factory.cpp \
        upstream.patched/src/images/SkImageEncoder_argb.cpp \
        upstream.patched/src/images/SkImageRef.cpp \
        upstream.patched/src/images/SkImageRefPool.cpp \
        upstream.patched/src/images/SkImageRef_GlobalPool.cpp \
        upstream.patched/src/images/SkImages.cpp \
        upstream.patched/src/images/SkJpegUtility.cpp \
        upstream.patched/src/images/SkMovie.cpp \
        upstream.patched/src/images/SkMovie_gif.cpp \
        upstream.patched/src/images/SkPageFlipper.cpp \
        upstream.patched/src/images/SkScaledBitmapSampler.cpp \
        upstream.patched/src/images/SkStreamHelpers.cpp \
        upstream.patched/src/images/bmpdecoderhelper.cpp \
        upstream.patched/src/sfnt/SkOTTable_name.cpp \
        upstream.patched/src/sfnt/SkOTUtils.cpp \
        upstream.patched/src/pathops/SkAddIntersections.cpp \
        upstream.patched/src/pathops/SkDCubicIntersection.cpp \
        upstream.patched/src/pathops/SkDCubicLineIntersection.cpp \
        upstream.patched/src/pathops/SkDCubicToQuads.cpp \
        upstream.patched/src/pathops/SkDLineIntersection.cpp \
        upstream.patched/src/pathops/SkDQuadImplicit.cpp \
        upstream.patched/src/pathops/SkDQuadIntersection.cpp \
        upstream.patched/src/pathops/SkDQuadLineIntersection.cpp \
        upstream.patched/src/pathops/SkIntersections.cpp \
        upstream.patched/src/pathops/SkOpAngle.cpp \
        upstream.patched/src/pathops/SkOpContour.cpp \
        upstream.patched/src/pathops/SkOpEdgeBuilder.cpp \
        upstream.patched/src/pathops/SkOpSegment.cpp \
        upstream.patched/src/pathops/SkPathOpsBounds.cpp \
        upstream.patched/src/pathops/SkPathOpsCommon.cpp \
        upstream.patched/src/pathops/SkPathOpsCubic.cpp \
        upstream.patched/src/pathops/SkPathOpsDebug.cpp \
        upstream.patched/src/pathops/SkPathOpsLine.cpp \
        upstream.patched/src/pathops/SkPathOpsOp.cpp \
        upstream.patched/src/pathops/SkPathOpsPoint.cpp \
        upstream.patched/src/pathops/SkPathOpsQuad.cpp \
        upstream.patched/src/pathops/SkPathOpsRect.cpp \
        upstream.patched/src/pathops/SkPathOpsSimplify.cpp \
        upstream.patched/src/pathops/SkPathOpsTriangle.cpp \
        upstream.patched/src/pathops/SkPathOpsTypes.cpp \
        upstream.patched/src/pathops/SkPathWriter.cpp \
        upstream.patched/src/pathops/SkQuarticRoot.cpp \
        upstream.patched/src/pathops/SkReduceOrder.cpp \
        upstream.patched/src/gpu/GrAAConvexPathRenderer.cpp \
        upstream.patched/src/gpu/GrAAHairLinePathRenderer.cpp \
        upstream.patched/src/gpu/GrAARectRenderer.cpp \
        upstream.patched/src/gpu/GrAddPathRenderers_default.cpp \
        upstream.patched/src/gpu/GrAllocPool.cpp \
        upstream.patched/src/gpu/GrAtlas.cpp \
        upstream.patched/src/gpu/GrBlend.cpp \
        upstream.patched/src/gpu/GrBufferAllocPool.cpp \
        upstream.patched/src/gpu/GrCacheID.cpp \
        upstream.patched/src/gpu/GrClipData.cpp \
        upstream.patched/src/gpu/GrClipMaskCache.cpp \
        upstream.patched/src/gpu/GrClipMaskManager.cpp \
        upstream.patched/src/gpu/GrContext.cpp \
        upstream.patched/src/gpu/GrDefaultPathRenderer.cpp \
        upstream.patched/src/gpu/GrDrawState.cpp \
        upstream.patched/src/gpu/GrDrawTarget.cpp \
        upstream.patched/src/gpu/GrEffect.cpp \
        upstream.patched/src/gpu/GrGeometryBuffer.cpp \
        upstream.patched/src/gpu/GrGpu.cpp \
        upstream.patched/src/gpu/GrGpuFactory.cpp \
        upstream.patched/src/gpu/GrInOrderDrawBuffer.cpp \
        upstream.patched/src/gpu/GrMemory.cpp \
        upstream.patched/src/gpu/GrMemoryPool.cpp \
        upstream.patched/src/gpu/GrOvalRenderer.cpp \
        upstream.patched/src/gpu/GrPaint.cpp \
        upstream.patched/src/gpu/GrPath.cpp \
        upstream.patched/src/gpu/GrPathRenderer.cpp \
        upstream.patched/src/gpu/GrPathRendererChain.cpp \
        upstream.patched/src/gpu/GrPathUtils.cpp \
        upstream.patched/src/gpu/GrRectanizer.cpp \
        upstream.patched/src/gpu/GrReducedClip.cpp \
        upstream.patched/src/gpu/GrRenderTarget.cpp \
        upstream.patched/src/gpu/GrResource.cpp \
        upstream.patched/src/gpu/GrResourceCache.cpp \
        upstream.patched/src/gpu/GrSWMaskHelper.cpp \
        upstream.patched/src/gpu/GrSoftwarePathRenderer.cpp \
        upstream.patched/src/gpu/GrStencil.cpp \
        upstream.patched/src/gpu/GrStencilAndCoverPathRenderer.cpp \
        upstream.patched/src/gpu/GrStencilBuffer.cpp \
        upstream.patched/src/gpu/GrSurface.cpp \
        upstream.patched/src/gpu/GrTest.cpp \
        upstream.patched/src/gpu/GrTextContext.cpp \
        upstream.patched/src/gpu/GrTextStrike.cpp \
        upstream.patched/src/gpu/GrTexture.cpp \
        upstream.patched/src/gpu/GrTextureAccess.cpp \
        upstream.patched/src/gpu/SkGpuDevice.cpp \
        upstream.patched/src/gpu/SkGr.cpp \
        upstream.patched/src/gpu/SkGrFontScaler.cpp \
        upstream.patched/src/gpu/SkGrPixelRef.cpp \
        upstream.patched/src/gpu/SkGrTexturePixelRef.cpp \
        upstream.patched/src/gpu/effects/GrBezierEffect.cpp \
        upstream.patched/src/gpu/effects/GrBicubicEffect.cpp \
        upstream.patched/src/gpu/effects/GrConfigConversionEffect.cpp \
        upstream.patched/src/gpu/effects/GrConvolutionEffect.cpp \
        upstream.patched/src/gpu/effects/GrSimpleTextureEffect.cpp \
        upstream.patched/src/gpu/effects/GrSingleTextureEffect.cpp \
        upstream.patched/src/gpu/effects/GrTextureDomainEffect.cpp \
        upstream.patched/src/gpu/effects/GrTextureStripAtlas.cpp \
        upstream.patched/src/gpu/gl/GrGLBufferImpl.cpp \
        upstream.patched/src/gpu/gl/GrGLCaps.cpp \
        upstream.patched/src/gpu/gl/GrGLContext.cpp \
        upstream.patched/src/gpu/gl/GrGLCreateNullInterface.cpp \
        upstream.patched/src/gpu/gl/GrGLDefaultInterface_native.cpp \
        upstream.patched/src/gpu/gl/GrGLEffect.cpp \
        upstream.patched/src/gpu/gl/GrGLEffectMatrix.cpp \
        upstream.patched/src/gpu/gl/GrGLExtensions.cpp \
        upstream.patched/src/gpu/gl/GrGLIndexBuffer.cpp \
        upstream.patched/src/gpu/gl/GrGLInterface.cpp \
        upstream.patched/src/gpu/gl/GrGLNoOpInterface.cpp \
        upstream.patched/src/gpu/gl/GrGLPath.cpp \
        upstream.patched/src/gpu/gl/GrGLProgram.cpp \
        upstream.patched/src/gpu/gl/GrGLProgramDesc.cpp \
        upstream.patched/src/gpu/gl/GrGLRenderTarget.cpp \
        upstream.patched/src/gpu/gl/GrGLSL.cpp \
        upstream.patched/src/gpu/gl/GrGLShaderBuilder.cpp \
        upstream.patched/src/gpu/gl/GrGLStencilBuffer.cpp \
        upstream.patched/src/gpu/gl/GrGLTexture.cpp \
        upstream.patched/src/gpu/gl/GrGLUniformManager.cpp \
        upstream.patched/src/gpu/gl/GrGLUtil.cpp \
        upstream.patched/src/gpu/gl/GrGLVertexArray.cpp \
        upstream.patched/src/gpu/gl/GrGLVertexBuffer.cpp \
        upstream.patched/src/gpu/gl/GrGpuGL.cpp \
        upstream.patched/src/gpu/gl/GrGpuGL_program.cpp \
        upstream.patched/src/gpu/gl/SkGLContextHelper.cpp \
        upstream.patched/src/gpu/gl/SkNullGLContext.cpp \
        upstream.patched/src/utils/SkBase64.cpp \
        upstream.patched/src/utils/SkBitSet.cpp \
        upstream.patched/src/utils/SkBoundaryPatch.cpp \
        upstream.patched/src/utils/SkCamera.cpp \
        upstream.patched/src/utils/SkCondVar.cpp \
        upstream.patched/src/utils/SkCountdown.cpp \
        upstream.patched/src/utils/SkCubicInterval.cpp \
        upstream.patched/src/utils/SkCullPoints.cpp \
        upstream.patched/src/utils/SkDeferredCanvas.cpp \
        upstream.patched/src/utils/SkDumpCanvas.cpp \
        upstream.patched/src/utils/SkInterpolator.cpp \
        upstream.patched/src/utils/SkJSON.cpp \
        upstream.patched/src/utils/SkLayer.cpp \
        upstream.patched/src/utils/SkMatrix44.cpp \
        upstream.patched/src/utils/SkMeshUtils.cpp \
        upstream.patched/src/utils/SkNinePatch.cpp \
        upstream.patched/src/utils/SkNullCanvas.cpp \
        upstream.patched/src/utils/SkNWayCanvas.cpp \
        upstream.patched/src/utils/SkOSFile.cpp \
        upstream.patched/src/utils/SkParse.cpp \
        upstream.patched/src/utils/SkParseColor.cpp \
        upstream.patched/src/utils/SkParsePath.cpp \
        upstream.patched/src/utils/SkPictureUtils.cpp \
        upstream.patched/src/utils/SkProxyCanvas.cpp \
        upstream.patched/src/utils/SkThreadPool.cpp \
        upstream.patched/src/utils/SkUnitMappers.cpp \
        upstream.patched/src/pipe/SkGPipeRead.cpp \
        upstream.patched/src/pipe/SkGPipeWrite.cpp

    # Android specific files
    LOCAL_SRC_FILES += \
        upstream.patched/src/ports/SkDebug_android.cpp \
        upstream.patched/src/ports/SkGlobalInitialization_default.cpp \
        upstream.patched/src/ports/SkFontHost_sandbox_none.cpp \
        upstream.patched/src/ports/SkThread_pthread.cpp \
        upstream.patched/src/ports/SkTime_Unix.cpp \
        upstream.patched/src/utils/SkThreadUtils_pthread.cpp \
        upstream.patched/src/ports/SkFontConfigParser_android.cpp \
        upstream.patched/src/ports/SkFontConfigInterface_android.cpp \
        upstream.patched/src/ports/SkFontHost_fontconfig.cpp \
        upstream.patched/src/ports/SkFontHost_FreeType.cpp \
        upstream.patched/src/ports/SkFontHost_FreeType_common.cpp \
        upstream.patched/src/ports/SkOSFile_posix.cpp \
        upstream.patched/src/ports/SkOSFile_stdio.cpp \
        upstream.patched/src/ports/SkTLS_pthread.cpp \
        upstream.patched/src/gpu/gl/android/GrGLCreateNativeInterface_android.cpp \
        upstream.patched/src/gpu/gl/android/SkNativeGLContext_android.cpp

    ifeq ($(TARGET_ARCH),arm)
        ifeq ($(LOCAL_ARM_NEON),true)
            LOCAL_SRC_FILES += \
                upstream.patched/src/opts/memset16_neon.S \
                upstream.patched/src/opts/memset32_neon.S \
                upstream.patched/src/opts/SkBitmapProcState_arm_neon.cpp \
                upstream.patched/src/opts/SkBitmapProcState_matrixProcs_neon.cpp \
                upstream.patched/src/opts/SkBlitRow_opts_arm_neon.cpp
        endif

        LOCAL_SRC_FILES += \
            upstream.patched/src/opts/opts_check_arm.cpp \
            upstream.patched/src/opts/memset.arm.S \
            upstream.patched/src/opts/SkBitmapProcState_opts_arm.cpp \
            upstream.patched/src/opts/SkBlitRow_opts_arm.cpp \
            upstream.patched/src/opts/SkBlitMask_opts_arm.cpp

    else

        LOCAL_SRC_FILES += \
            upstream.patched/src/opts/SkBlitRow_opts_none.cpp \
            upstream.patched/src/opts/SkBitmapProcState_opts_none.cpp \
            upstream.patched/src/opts/SkUtils_opts_none.cpp \
            upstream.patched/src/opts/SkBlitMask_opts_none.cpp

    endif

    include $(BUILD_STATIC_LIBRARY)
else
    LOCAL_SRC_FILES := \
        $(OSMAND_ANDROID_PREBUILT_ROOT)/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
    include $(PREBUILT_STATIC_LIBRARY)
endif
