LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
    LOCAL_MODULE := osmand_ft2
else
    LOCAL_MODULE := osmand_ft2_neon
    LOCAL_ARM_NEON := true
endif

LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/upstream.patched/include

LOCAL_CFLAGS := \
    -DFT2_BUILD_LIBRARY \
    -DFT_CONFIG_MODULES_H="<ftmodule-override.h>"

LOCAL_C_INCLUDES := \
    $(LOCAL_EXPORT_C_INCLUDES) \
    $(LOCAL_PATH)

LOCAL_SRC_FILES:= \
    upstream.patched/src/base/ftbase.c \
    upstream.patched/src/base/ftbbox.c \
    upstream.patched/src/base/ftbitmap.c \
    upstream.patched/src/base/ftfstype.c \
    upstream.patched/src/base/ftgasp.c \
    upstream.patched/src/base/ftglyph.c \
    upstream.patched/src/base/ftinit.c \
    upstream.patched/src/base/ftlcdfil.c \
    upstream.patched/src/base/ftstroke.c \
    upstream.patched/src/base/ftsystem.c \
    upstream.patched/src/base/fttype1.c \
    upstream.patched/src/base/ftxf86.c \
    upstream.patched/src/autofit/autofit.c \
    upstream.patched/src/bdf/bdf.c \
    upstream.patched/src/cff/cff.c \
    upstream.patched/src/pshinter/pshinter.c \
    upstream.patched/src/psnames/psnames.c \
    upstream.patched/src/raster/raster.c \
    upstream.patched/src/sfnt/sfnt.c \
    upstream.patched/src/smooth/smooth.c \
    upstream.patched/src/truetype/truetype.c

include $(BUILD_STATIC_LIBRARY)
