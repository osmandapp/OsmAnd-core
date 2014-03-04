LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
    LOCAL_MODULE := osmand_expat
else
    LOCAL_MODULE := osmand_expat_neon
    LOCAL_ARM_NEON := true
endif

LOCAL_EXPORT_CFLAGS := \
    -DXML_STATIC
LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/upstream.patched/lib

LOCAL_CFLAGS := \
    $(LOCAL_EXPORT_CFLAGS) \
    -DHAVE_EXPAT_CONFIG_H

LOCAL_C_INCLUDES := \
    $(LOCAL_EXPORT_C_INCLUDES) \
    $(LOCAL_PATH)

LOCAL_SRC_FILES := \
    upstream.patched/lib/xmlparse.c \
    upstream.patched/lib/xmlrole.c \
    upstream.patched/lib/xmltok.c \
    upstream.patched/lib/xmltok_impl.c \
    upstream.patched/lib/xmltok_ns.c

include $(BUILD_STATIC_LIBRARY)
