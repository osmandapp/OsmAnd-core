LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

PROJECT_ROOT_RELATIVE := ../../../../platforms/android/OsmAnd
OSMAND_JPEG_ROOT_RELATIVE := ../../../externals/jpeg
OSMAND_JPEG_ROOT := $(LOCAL_PATH)/$(OSMAND_JPEG_ROOT_RELATIVE)
OSMAND_JPEG_RELATIVE := ../../../externals/skia/upstream.patched/third_party/externals/libjpeg-turbo
OSMAND_JPEG := $(LOCAL_PATH)/$(OSMAND_JPEG_RELATIVE)
	
LOCAL_CFLAGS += -DNO_GETENV -fPIC

LOCAL_C_INCLUDES += \
	$(OSMAND_JPEG_ROOT) \
	$(OSMAND_JPEG) \
	$(OSMAND_JPEG)/simd \

ifneq ($(filter $(TARGET_ARCH_ABI), armeabi-v7a armeabi-v7a-hard x86),)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -D__ARM_HAVE_NEON
endif

LOCAL_ASMFLAGS += -DELF

ifeq ($(TARGET_ARCH_ABI),x86_64)
SIMD_SOURCES := $(OSMAND_JPEG_RELATIVE)/simd/x86_64
LOCAL_SRC_FILES += \
	$(OSMAND_JPEG_RELATIVE)/jsimd_none.c \

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=8 \
	-DWITH_SIMD=0 \

LOCAL_ASMFLAGS += -D__x86_64__

else ifeq ($(TARGET_ARCH_ABI),x86)
SIMD_SOURCES := $(OSMAND_JPEG_RELATIVE)/simd/i386
LOCAL_SRC_FILES += \
	$(OSMAND_JPEG_RELATIVE)/jsimd_none.c \

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=4 \
	-DWITH_SIMD=0 \

LOCAL_ASMFLAGS += -DPIC

else ifneq ($(filter $(TARGET_ARCH_ABI), armeabi-v7a armeabi-v7a-hard),)
SIMD_SOURCES := $(OSMAND_JPEG_RELATIVE)/simd/arm
LOCAL_SRC_FILES += \
	$(SIMD_SOURCES)/jsimd.c \
	$(SIMD_SOURCES)/jsimd_neon.S \

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=4 \
	-DWITH_SIMD=1 \

else ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=4 \

else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
SIMD_SOURCES := $(OSMAND_JPEG_RELATIVE)/simd/arm64
LOCAL_SRC_FILES += \
	$(SIMD_SOURCES)/jsimd.c \
	$(SIMD_SOURCES)/jsimd_neon.S \

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=8 \
	-DWITH_SIMD=1 \

endif

# libjpeg_la_SOURCES from Makefile.am
LOCAL_SRC_FILES += \
	$(OSMAND_JPEG_RELATIVE)/jcapimin.c \
	$(OSMAND_JPEG_RELATIVE)/jcapistd.c \
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
	$(OSMAND_JPEG_RELATIVE)/jcphuff.c \
	$(OSMAND_JPEG_RELATIVE)/jcprepct.c \
	$(OSMAND_JPEG_RELATIVE)/jcsample.c \
	$(OSMAND_JPEG_RELATIVE)/jctrans.c \
	$(OSMAND_JPEG_RELATIVE)/jdapimin.c \
	$(OSMAND_JPEG_RELATIVE)/jdapistd.c \
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
	$(OSMAND_JPEG_RELATIVE)/jdphuff.c \
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
	$(OSMAND_JPEG_RELATIVE)/jidctred.c \
	$(OSMAND_JPEG_RELATIVE)/jmemmgr.c \
	$(OSMAND_JPEG_RELATIVE)/jmemnobs.c \
	$(OSMAND_JPEG_RELATIVE)/jquant1.c \
	$(OSMAND_JPEG_RELATIVE)/jquant2.c \
	$(OSMAND_JPEG_RELATIVE)/jutils.c \

# if WITH_ARITH_ENC from Makefile.am
LOCAL_SRC_FILES += \
	$(OSMAND_JPEG_RELATIVE)/jaricom.c \
	$(OSMAND_JPEG_RELATIVE)/jcarith.c \
	$(OSMAND_JPEG_RELATIVE)/jdarith.c \

# libturbojpeg_la_SOURCES from Makefile.am
LOCAL_SRC_FILES += \
	$(OSMAND_JPEG_RELATIVE)/turbojpeg.c \
	$(OSMAND_JPEG_RELATIVE)/transupp.c \
	$(OSMAND_JPEG_RELATIVE)/jdatadst-tj.c \
	$(OSMAND_JPEG_RELATIVE)/jdatasrc-tj.c \

LOCAL_EXPORT_C_INCLUDES := \
	$(OSMAND_JPEG_ROOT) \
	$(OSMAND_JPEG) \

LOCAL_MODULE := osmand_jpeg

ifneq ($(OSMAND_USE_PREBUILT),true)
	include $(BUILD_STATIC_LIBRARY)
else
	LOCAL_SRC_FILES := \
		$(PROJECT_ROOT_RELATIVE)/libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
	include $(PREBUILT_STATIC_LIBRARY)
endif