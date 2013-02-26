LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

PROJECT_ROOT_RELATIVE := ../../../platforms/android/OsmAnd
OSMAND_SKIA_ROOT_RELATIVE := .
OSMAND_SKIA_ROOT := $(LOCAL_PATH)/$(OSMAND_SKIA_ROOT_RELATIVE)
OSMAND_SKIA_RELATIVE := ./upstream.patched
OSMAND_SKIA := $(LOCAL_PATH)/$(OSMAND_SKIA_RELATIVE)

OSMAND_FREETYPE_RELATIVE := ../freetype/upstream.patched
OSMAND_FREETYPE := $(LOCAL_PATH)/$(OSMAND_FREETYPE_RELATIVE)

OSMAND_LIBPNG_RELATIVE := ../libpng/upstream.patched
OSMAND_LIBPNG := $(LOCAL_PATH)/$(OSMAND_LIBPNG_RELATIVE)

OSMAND_GIFLIB_RELATIVE := ../giflib/upstream.patched
OSMAND_GIFLIB := $(LOCAL_PATH)/$(OSMAND_GIFLIB_RELATIVE)

OSMAND_JPEG_RELATIVE := ../jpeg/upstream.patched
OSMAND_JPEG := $(LOCAL_PATH)/$(OSMAND_JPEG_RELATIVE)

OSMAND_EXPAT_RELATIVE := ../expat/upstream.patched
OSMAND_EXPAT := $(LOCAL_PATH)/$(OSMAND_EXPAT_RELATIVE)

OSMAND_HARFBUZZ_RELATIVE := ../harfbuzz/upstream.patched
OSMAND_HARFBUZZ := $(LOCAL_PATH)/$(OSMAND_HARFBUZZ_RELATIVE)

LOCAL_SRC_FILES := \
	$(OSMAND_SKIA_RELATIVE)/src/core/Sk64.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkAAClip.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkAdvancedTypefaceMetrics.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkAlphaRuns.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkAnnotation.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBBoxHierarchy.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBBoxHierarchyRecord.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBBoxRecord.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmap.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapHeap.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcState.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapProcState_matrixProcs.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmapSampler.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBitmap_scroll.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitMask_D32.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitRow_D16.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitRow_D32.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitRow_D4444.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter_4444.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter_A1.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter_A8.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter_ARGB32.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter_RGB16.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBlitter_Sprite.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkBuffer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkCanvas.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkChunkAlloc.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkClipStack.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColor.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkColorTable.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkComposeShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkConfig8888.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkCordic.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkCubicClipper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkData.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDebug.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDeque.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDevice.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDeviceProfile.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDither.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkDraw.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkEdge.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkEdgeBuilder.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkEdgeClipper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFilterProc.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFlate.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFlattenable.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFlattenableBuffers.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFloat.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFloatBits.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFontDescriptor.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkFontHost.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkGeometry.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkGlyphCache.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkGraphics.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkInstCnt.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkLineClipper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMallocPixelRef.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMask.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMaskFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMaskGamma.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMath.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMatrix.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMetaData.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkOrderedReadBuffer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkOrderedWriteBuffer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPackBits.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPaint.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPaintPriv.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPath.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPathEffect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPathHeap.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPathMeasure.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPicture.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureFlat.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPicturePlayback.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureRecord.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPictureStateTree.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPixelRef.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPoint.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkProcSpriteBlitter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkPtrRecorder.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkQuadClipper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRasterClip.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRasterizer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRefCnt.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRefDict.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRegion.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRegion_path.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRegion_rects.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRRect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkRTree.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScalar.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScalerContext.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScan.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScan_Antihair.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScan_AntiPath.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScan_Hairline.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkScan_Path.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSpriteBlitter_ARGB32.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkSpriteBlitter_RGB16.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkStream.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkString.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkStroke.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkStrokeRec.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkStrokerPriv.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTileGrid.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTLS.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTSearch.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTypeface.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkTypefaceCache.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkUnPreMultiply.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkUtils.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkUtilsArm.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkWriter32.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkXfermode.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/Sk1DPathEffect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/Sk2DPathEffect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkArithmeticMode.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkAvoidXfermode.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkBitmapSource.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkBicubicImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkBlurDrawLooper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkBlurImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkBlurMask.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkBlurMaskFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkColorFilters.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkColorMatrix.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkColorMatrixFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkCornerPathEffect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkDashPathEffect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkDiscretePathEffect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkDisplacementMapEffect.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkEmbossMask.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkEmbossMaskFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkKernel33MaskFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkLayerDrawLooper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkLayerRasterizer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkMorphologyImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkOffsetImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkPaintFlagsDrawFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkPixelXorXfermode.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkPorterDuff.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkStippleMaskFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkTableColorFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkTableMaskFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkTestImageFilters.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkTransparentShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkLightingImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkBlendImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkMagnifierImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkMergeImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkColorFilterImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/SkMatrixConvolutionImageFilter.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkBitmapCache.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkGradientShader.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkTwoPointConicalGradient.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkTwoPointRadialGradient.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkSweepGradient.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkRadialGradient.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkLinearGradient.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/effects/gradients/SkClampRange.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/image/SkSurface.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/image/SkImage.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/bmpdecoderhelper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkBitmapFactory.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkImageDecoder.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkImageDecoder_Factory.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkImageDecoder_libgif.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkImageDecoder_libjpeg.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkImageDecoder_libpng.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkImageEncoder.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkImageEncoder_Factory.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkImageRef.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkImageRefPool.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkImageRef_GlobalPool.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkImages.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkJpegUtility.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkMovie.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkMovie_gif.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkPageFlipper.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/images/SkScaledBitmapSampler.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/sfnt/SkOTUtils.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkBase64.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkBitSet.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkBoundaryPatch.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkCamera.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkCondVar.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkCountdown.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkCubicInterval.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkCullPoints.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkDeferredCanvas.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkDumpCanvas.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkInterpolator.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkJSON.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkLayer.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkMatrix44.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkMeshUtils.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkNinePatch.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkNullCanvas.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkNWayCanvas.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkOSFile.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkParse.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkParseColor.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkParsePath.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkPictureUtils.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkProxyCanvas.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkUnitMappers.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pipe/SkGPipeRead.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/pipe/SkGPipeWrite.cpp

LOCAL_C_INCLUDES += \
	$(OSMAND_SKIA)/src/core \
	$(OSMAND_SKIA)/include/core \
	$(OSMAND_SKIA)/src/config \
	$(OSMAND_SKIA)/include/config \
	$(OSMAND_SKIA)/src/effects \
	$(OSMAND_SKIA)/include/effects \
	$(OSMAND_SKIA)/src/image \
	$(OSMAND_SKIA)/src/images \
	$(OSMAND_SKIA)/include/images \
	$(OSMAND_SKIA)/include/pipe \
	$(OSMAND_SKIA)/src/ports \
	$(OSMAND_SKIA)/include/ports \
	$(OSMAND_SKIA)/src/utils \
	$(OSMAND_SKIA)/include/utils \
	$(OSMAND_SKIA)/src/utils/android \
	$(OSMAND_SKIA)/include/utils/android \
	$(OSMAND_SKIA)/src/xml \
	$(OSMAND_SKIA)/include/xml \
	$(OSMAND_SKIA)/src/sfnt \
	$(OSMAND_SKIA)/src/gpu \
	$(OSMAND_SKIA)/include/gpu \
	$(OSMAND_EXPAT)/lib \
	$(OSMAND_FREETYPE)/include \
	$(OSMAND_GIFLIB)/lib \
	$(OSMAND_JPEG) \
	$(OSMAND_LIBPNG) \
	$(OSMAND_HARFBUZZ)/src
	
LOCAL_CFLAGS += \
	-DSK_BUILD_FOR_ANDROID \
	-DSK_BUILD_FOR_ANDROID_NDK \
	-DSK_ALLOW_STATIC_GLOBAL_INITIALIZERS=0 \
	-DSK_RELEASE \
	-DGR_RELEASE=1 \
	-DNDEBUG \
	-DSK_SUPPORT_GPU=0
	
ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
	LOCAL_MODULE := osmand_skia
else
	LOCAL_MODULE := osmand_skia_neon
	LOCAL_ARM_NEON := true
endif

ifneq ($(OSMAND_USE_PREBUILT),true)

LOCAL_ARM_MODE := arm

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

# Android specific files
LOCAL_SRC_FILES += \
	$(OSMAND_SKIA_RELATIVE)/src/core/SkMMapStream.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkDebug_android.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkGlobalInitialization_default.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkFontHost_FreeType.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkFontHost_FreeType_common.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkFontHost_sandbox_none.cpp	\
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkFontHost_android.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkFontHost_tables.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkMemory_malloc.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkOSFile_stdio.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkThread_pthread.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/SkTime_Unix.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/ports/FontHostConfiguration_android.cpp \
	$(OSMAND_SKIA_RELATIVE)/src/utils/SkThreadUtils_pthread.cpp
	
ifeq ($(TARGET_ARCH),arm)
	ifeq ($(LOCAL_ARM_NEON),true)
		LOCAL_SRC_FILES += \
			$(OSMAND_SKIA_RELATIVE)/src/opts/memset16_neon.S \
			$(OSMAND_SKIA_RELATIVE)/src/opts/memset32_neon.S \
			$(OSMAND_SKIA_RELATIVE)/src/opts/SkBitmapProcState_arm_neon.cpp \
			$(OSMAND_SKIA_RELATIVE)/src/opts/SkBitmapProcState_matrixProcs_neon.cpp \
			$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlitRow_opts_arm_neon.cpp
	endif

	LOCAL_SRC_FILES += \
		$(OSMAND_SKIA_RELATIVE)/src/opts/opts_check_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/memset.arm.S \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBitmapProcState_opts_arm.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlitRow_opts_arm.cpp
		
else

	LOCAL_SRC_FILES += \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBlitRow_opts_none.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkBitmapProcState_opts_none.cpp \
		$(OSMAND_SKIA_RELATIVE)/src/opts/SkUtils_opts_none.cpp
		
endif

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libz
	
ifeq ($(LOCAL_ARM_NEON),true)
	LOCAL_STATIC_LIBRARIES += \
		osmand_jpeg_neon \
		osmand_ft2_neon \
		osmand_png_neon \
		osmand_gif_neon \
		osmand_expat_neon
else
	LOCAL_STATIC_LIBRARIES += \
		osmand_jpeg \
		osmand_ft2 \
		osmand_png \
		osmand_gif \
		osmand_expat
endif

LOCAL_LDLIBS += -lz -llog

include $(BUILD_STATIC_LIBRARY)

else
	LOCAL_SRC_FILES := \
		$(PROJECT_ROOT_RELATIVE)/libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
	include $(PREBUILT_STATIC_LIBRARY)
endif
