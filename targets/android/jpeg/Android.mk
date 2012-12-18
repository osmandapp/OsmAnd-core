LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

PROJECT_ROOT_RELATIVE := ../../../../platforms/android/OsmAnd
OSMAND_JPEG_ROOT_RELATIVE := ../../../externals/jpeg
OSMAND_JPEG_ROOT := $(LOCAL_PATH)/$(OSMAND_JPEG_ROOT_RELATIVE)
OSMAND_JPEG_RELATIVE := ../../../externals/jpeg/upstream.patched
OSMAND_JPEG := $(LOCAL_PATH)/$(OSMAND_JPEG_RELATIVE)

LOCAL_C_INCLUDES += \
	$(OSMAND_JPEG_ROOT)
	
LOCAL_CFLAGS += -DNO_GETENV -fPIC

LOCAL_SRC_FILES := \
	$(OSMAND_JPEG_RELATIVE)/jaricom.c \
	$(OSMAND_JPEG_RELATIVE)/jcapimin.c \
	$(OSMAND_JPEG_RELATIVE)/jcapistd.c \
	$(OSMAND_JPEG_RELATIVE)/jcarith.c \
	$(OSMAND_JPEG_RELATIVE)/jccoefct.c \
	$(OSMAND_JPEG_RELATIVE)/jccolor.c \
	$(OSMAND_JPEG_RELATIVE)/jcdctmgr.c \
	$(OSMAND_JPEG_RELATIVE)/jchuff.c \
	$(OSMAND_JPEG_RELATIVE)/jcinit.c \
	$(OSMAND_JPEG_RELATIVE)/jcmainct.c \
	$(OSMAND_JPEG_RELATIVE)/jcmarker.c \
	$(OSMAND_JPEG_RELATIVE)/jcmaster.c \
	$(OSMAND_JPEG_RELATIVE)/jcomapi.c \
	$(OSMAND_JPEG_RELATIVE)/jcparam.c \
	$(OSMAND_JPEG_RELATIVE)/jcprepct.c \
	$(OSMAND_JPEG_RELATIVE)/jcsample.c \
	$(OSMAND_JPEG_RELATIVE)/jctrans.c \
	$(OSMAND_JPEG_RELATIVE)/jdapimin.c \
	$(OSMAND_JPEG_RELATIVE)/jdapistd.c \
	$(OSMAND_JPEG_RELATIVE)/jdarith.c \
	$(OSMAND_JPEG_RELATIVE)/jdatadst.c \
	$(OSMAND_JPEG_RELATIVE)/jdatasrc.c \
	$(OSMAND_JPEG_RELATIVE)/jdcoefct.c \
	$(OSMAND_JPEG_RELATIVE)/jdcolor.c \
	$(OSMAND_JPEG_RELATIVE)/jddctmgr.c \
	$(OSMAND_JPEG_RELATIVE)/jdhuff.c \
	$(OSMAND_JPEG_RELATIVE)/jdinput.c \
	$(OSMAND_JPEG_RELATIVE)/jdmainct.c \
	$(OSMAND_JPEG_RELATIVE)/jdmarker.c \
	$(OSMAND_JPEG_RELATIVE)/jdmaster.c \
	$(OSMAND_JPEG_RELATIVE)/jdmerge.c \
	$(OSMAND_JPEG_RELATIVE)/jdpostct.c \
	$(OSMAND_JPEG_RELATIVE)/jdsample.c \
	$(OSMAND_JPEG_RELATIVE)/jdtrans.c \
	$(OSMAND_JPEG_RELATIVE)/jerror.c \
	$(OSMAND_JPEG_RELATIVE)/jfdctflt.c \
	$(OSMAND_JPEG_RELATIVE)/jfdctfst.c \
	$(OSMAND_JPEG_RELATIVE)/jfdctint.c \
	$(OSMAND_JPEG_RELATIVE)/jidctflt.c \
	$(OSMAND_JPEG_RELATIVE)/jidctfst.c \
	$(OSMAND_JPEG_RELATIVE)/jidctint.c \
	$(OSMAND_JPEG_RELATIVE)/jmemansi.c \
	$(OSMAND_JPEG_RELATIVE)/jmemmgr.c \
	$(OSMAND_JPEG_RELATIVE)/jquant1.c \
	$(OSMAND_JPEG_RELATIVE)/jquant2.c \
	$(OSMAND_JPEG_RELATIVE)/jutils.c

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
	LOCAL_MODULE := osmand_jpeg
else
	LOCAL_MODULE := osmand_jpeg_neon
	LOCAL_ARM_NEON := true
endif

ifneq ($(OSMAND_USE_PREBUILT),true)
	include $(BUILD_STATIC_LIBRARY)
else
	LOCAL_SRC_FILES := \
		$(PROJECT_ROOT_RELATIVE)/libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
	include $(PREBUILT_STATIC_LIBRARY)
endif