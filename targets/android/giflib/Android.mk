LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

PROJECT_ROOT_RELATIVE := ../../../../platforms/android/OsmAnd
OSMAND_GIFLIB_ROOT_RELATIVE := ../../../externals/giflib
OSMAND_GIFLIB_ROOT := $(LOCAL_PATH)/$(OSMAND_GIFLIB_ROOT_RELATIVE)
OSMAND_GIFLIB_RELATIVE := ../../../externals/giflib/upstream.patched
OSMAND_GIFLIB := $(LOCAL_PATH)/$(OSMAND_GIFLIB_RELATIVE)

LOCAL_CFLAGS += -DHAVE_CONFIG_H -fPIC

LOCAL_SRC_FILES := \
	$(OSMAND_GIFLIB_RELATIVE)/lib/dgif_lib.c \
	$(OSMAND_GIFLIB_RELATIVE)/lib/gifalloc.c \
	$(OSMAND_GIFLIB_RELATIVE)/lib/gif_err.c

LOCAL_MODULE := osmand_gif

ifneq ($(OSMAND_USE_PREBUILT),true)
	include $(BUILD_STATIC_LIBRARY)
else
	LOCAL_SRC_FILES := \
		$(PROJECT_ROOT_RELATIVE)/libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
	include $(PREBUILT_STATIC_LIBRARY)
endif