LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
    LOCAL_MODULE := osmand_jpeg
else
    LOCAL_MODULE := osmand_jpeg_neon
    LOCAL_ARM_NEON := true
endif

LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/upstream.patched

LOCAL_CFLAGS := \
    -DNO_GETENV

LOCAL_C_INCLUDES := \
    $(LOCAL_EXPORT_C_INCLUDES) \
    $(LOCAL_PATH)

LOCAL_SRC_FILES := \
    upstream.patched/jaricom.c \
    upstream.patched/jcapimin.c \
    upstream.patched/jcapistd.c \
    upstream.patched/jcarith.c \
    upstream.patched/jccoefct.c \
    upstream.patched/jccolor.c \
    upstream.patched/jcdctmgr.c \
    upstream.patched/jchuff.c \
    upstream.patched/jcinit.c \
    upstream.patched/jcmainct.c \
    upstream.patched/jcmarker.c \
    upstream.patched/jcmaster.c \
    upstream.patched/jcomapi.c \
    upstream.patched/jcparam.c \
    upstream.patched/jcprepct.c \
    upstream.patched/jcsample.c \
    upstream.patched/jctrans.c \
    upstream.patched/jdapimin.c \
    upstream.patched/jdapistd.c \
    upstream.patched/jdarith.c \
    upstream.patched/jdatadst.c \
    upstream.patched/jdatasrc.c \
    upstream.patched/jdcoefct.c \
    upstream.patched/jdcolor.c \
    upstream.patched/jddctmgr.c \
    upstream.patched/jdhuff.c \
    upstream.patched/jdinput.c \
    upstream.patched/jdmainct.c \
    upstream.patched/jdmarker.c \
    upstream.patched/jdmaster.c \
    upstream.patched/jdmerge.c \
    upstream.patched/jdpostct.c \
    upstream.patched/jdsample.c \
    upstream.patched/jdtrans.c \
    upstream.patched/jerror.c \
    upstream.patched/jfdctflt.c \
    upstream.patched/jfdctfst.c \
    upstream.patched/jfdctint.c \
    upstream.patched/jidctflt.c \
    upstream.patched/jidctfst.c \
    upstream.patched/jidctint.c \
    upstream.patched/jmemansi.c \
    upstream.patched/jmemmgr.c \
    upstream.patched/jquant1.c \
    upstream.patched/jquant2.c \
    upstream.patched/jutils.c

include $(BUILD_STATIC_LIBRARY)
