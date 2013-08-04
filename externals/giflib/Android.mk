LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
    LOCAL_MODULE := osmand_gif
else
    LOCAL_MODULE := osmand_gif_neon
    LOCAL_ARM_NEON := true
endif

LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/upstream.patched/lib

ifneq ($(OSMAND_USE_PREBUILT),true)
    LOCAL_CFLAGS := \
        -DHAVE_CONFIG_H

    LOCAL_SRC_FILES := \
        upstream.patched/lib/dgif_lib.c \
        upstream.patched/lib/gifalloc.c \
        upstream.patched/lib/gif_err.c

    include $(BUILD_STATIC_LIBRARY)
else
    LOCAL_SRC_FILES := \
        $(OSMAND_ANDROID_PREBUILT_ROOT)/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
    include $(PREBUILT_STATIC_LIBRARY)
endif
