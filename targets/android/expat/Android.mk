LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

PROJECT_ROOT_RELATIVE := ../../../../platforms/android/OsmAnd
RELATIVE_OSMAND_EXPAT_ROOT := ../../../externals/skia/upstream.patched/third_party/externals/expat
OSMAND_EXPAT_ROOT := $(LOCAL_PATH)/$(RELATIVE_OSMAND_EXPAT_ROOT)
OSMAND_EXPAT_RELATIVE := ../../../externals/skia/upstream.patched/third_party/externals/expat
OSMAND_EXPAT := $(LOCAL_PATH)/$(OSMAND_EXPAT_RELATIVE)

LOCAL_C_INCLUDES += \
	$(OSMAND_EXPAT_ROOT)
	
LOCAL_CFLAGS += -DHAVE_EXPAT_CONFIG_H -fPIC

LOCAL_SRC_FILES := \
	$(OSMAND_EXPAT_RELATIVE)/lib/xmlparse.c \
	$(OSMAND_EXPAT_RELATIVE)/lib/xmlrole.c \
	$(OSMAND_EXPAT_RELATIVE)/lib/xmltok.c \
	$(OSMAND_EXPAT_RELATIVE)/lib/xmltok_impl.c \
	$(OSMAND_EXPAT_RELATIVE)/lib/xmltok_ns.c

LOCAL_MODULE := osmand_expat

ifneq ($(OSMAND_USE_PREBUILT),true)
	include $(BUILD_STATIC_LIBRARY)
else
	LOCAL_SRC_FILES := \
		$(PROJECT_ROOT_RELATIVE)/libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
	include $(PREBUILT_STATIC_LIBRARY)
endif
