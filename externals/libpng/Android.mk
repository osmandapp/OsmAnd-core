LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
    LOCAL_MODULE := osmand_png
else
    LOCAL_MODULE := osmand_png_neon
    LOCAL_ARM_NEON := true
endif

LOCAL_EXPORT_CFLAGS := \
    -DPNG_ARM_NEON_OPT=0

LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/upstream.patched

LOCAL_EXPORT_LDLIBS := \
    -lz

LOCAL_CFLAGS := \
    $(LOCAL_EXPORT_CFLAGS) \
    -DPNG_CONFIGURE_LIBPNG

LOCAL_C_INCLUDES := \
    $(LOCAL_EXPORT_C_INCLUDES)

LOCAL_SRC_FILES := \
    upstream.patched/png.c \
    upstream.patched/pngerror.c \
    upstream.patched/pngget.c \
    upstream.patched/pngmem.c \
    upstream.patched/pngpread.c \
    upstream.patched/pngread.c \
    upstream.patched/pngrio.c \
    upstream.patched/pngrtran.c \
    upstream.patched/pngrutil.c \
    upstream.patched/pngset.c \
    upstream.patched/pngtrans.c \
    upstream.patched/pngwio.c \
    upstream.patched/pngwrite.c \
    upstream.patched/pngwtran.c \
    upstream.patched/pngwutil.c

include $(BUILD_STATIC_LIBRARY)
