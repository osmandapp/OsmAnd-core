LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

PROJECT_ROOT_RELATIVE := ../../../../platforms/android/OsmAnd
OSMAND_FREETYPE_ROOT_RELATIVE := ../../../externals/freetype
OSMAND_FREETYPE_ROOT := $(LOCAL_PATH)/$(OSMAND_FREETYPE_ROOT_RELATIVE)
OSMAND_FREETYPE_RELATIVE := ../../../externals/freetype/upstream.patched
OSMAND_FREETYPE := $(LOCAL_PATH)/$(OSMAND_FREETYPE_RELATIVE)

LOCAL_SRC_FILES:= \
	$(OSMAND_FREETYPE_RELATIVE)/src/base/ftbase.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/base/ftbbox.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/base/ftbitmap.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/base/ftfntfmt.c" \
	$(OSMAND_FREETYPE_RELATIVE)/src/base/ftfstype.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/base/ftgasp.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/base/ftglyph.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/base/ftinit.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/base/ftlcdfil.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/base/ftstroke.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/base/ftsystem.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/base/fttype1.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/base/ftmm.c" \
	$(OSMAND_FREETYPE_RELATIVE)/src/gzip/ftgzip.c" \
	$(OSMAND_FREETYPE_RELATIVE)/src/autofit/autofit.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/bdf/bdf.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/cff/cff.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/pshinter/pshinter.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/psnames/psnames.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/raster/raster.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/sfnt/sfnt.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/smooth/smooth.c \
	$(OSMAND_FREETYPE_RELATIVE)/src/truetype/truetype.c

LOCAL_C_INCLUDES += \
	$(OSMAND_FREETYPE_ROOT) \
	$(OSMAND_FREETYPE)/include

LOCAL_CFLAGS += -DFT2_BUILD_LIBRARY -DFT_CONFIG_MODULES_H="<ftmodule-override.h>" -fPIC
LOCAL_ARM_MODE := arm

LOCAL_MODULE := osmand_ft2

ifneq ($(OSMAND_USE_PREBUILT),true)
	include $(BUILD_STATIC_LIBRARY)
else
	LOCAL_SRC_FILES := \
		$(PROJECT_ROOT_RELATIVE)/libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
	include $(PREBUILT_STATIC_LIBRARY)
endif
