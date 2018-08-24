LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

PROJECT_ROOT_RELATIVE := ../../../../platforms/android/OsmAnd
OSMAND_JPEG_ROOT_RELATIVE := ../../../externals/jpeg
OSMAND_JPEG_ROOT := $(LOCAL_PATH)/$(OSMAND_JPEG_ROOT_RELATIVE)
OSMAND_JPEG_RELATIVE := ../../../externals/skia/upstream.patched/third_party/externals/libjpeg-turbo
OSMAND_JPEG := $(LOCAL_PATH)/$(OSMAND_JPEG_RELATIVE)
	
LOCAL_CFLAGS += -DNO_GETENV -fPIC

LOCAL_C_INCLUDES += \
	$(OSMAND_JPEG_ROOT) 

ifneq ($(filter $(TARGET_ARCH_ABI), armeabi-v7a armeabi-v7a-hard x86),)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -D__ARM_HAVE_NEON
endif

LOCAL_ASMFLAGS += -DELF

ifeq ($(TARGET_ARCH_ABI),x86_64)
SIMD_SOURCES := $(OSMAND_JPEG)/simd/x86_64
LOCAL_SRC_FILES += \
	$(SIMD_SOURCES)/jsimd.c \
	$(SIMD_SOURCES)/jfdctflt-sse-64.asm \
	$(SIMD_SOURCES)/jccolor-sse2-64.asm \
	$(SIMD_SOURCES)/jcgray-sse2-64.asm \
	$(SIMD_SOURCES)/jcsample-sse2-64.asm \
	$(SIMD_SOURCES)/jdcolor-sse2-64.asm \
	$(SIMD_SOURCES)/jdmerge-sse2-64.asm \
	$(SIMD_SOURCES)/jdsample-sse2-64.asm \
	$(SIMD_SOURCES)/jfdctfst-sse2-64.asm \
	$(SIMD_SOURCES)/jfdctint-sse2-64.asm \
	$(SIMD_SOURCES)/jidctflt-sse2-64.asm \
	$(SIMD_SOURCES)/jidctfst-sse2-64.asm \
	$(SIMD_SOURCES)/jidctint-sse2-64.asm \
	$(SIMD_SOURCES)/jidctred-sse2-64.asm \
	$(SIMD_SOURCES)/jquantf-sse2-64.asm \
	$(SIMD_SOURCES)/jquanti-sse2-64.asm \

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=8 \

LOCAL_ASMFLAGS += -D__x86_64__

else ifeq ($(TARGET_ARCH_ABI),x86)
SIMD_SOURCES := $(OSMAND_JPEG)/simd/i386

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=4 \

LOCAL_ASMFLAGS += -DPIC

else ifneq ($(filter $(TARGET_ARCH_ABI), armeabi-v7a armeabi-v7a-hard),)
SIMD_SOURCES := $(OSMAND_JPEG)/simd/arm
LOCAL_SRC_FILES += \
	$(SIMD_SOURCES)/jsimd.c \
	$(SIMD_SOURCES)/jsimd_arm_neon.S \

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=4 \

else ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=4 \

else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
SIMD_SOURCES := $(OSMAND_JPEG)/simd/arm64
LOCAL_SRC_FILES += \
	$(SIMD_SOURCES)/jsimd.c \
	$(SIMD_SOURCES)/jsimd_neon.S \

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=8 \

endif

# libjpeg_la_SOURCES from Makefile.am
LOCAL_SRC_FILES += \
	$(OSMAND_JPEG)/jcapimin.c \
	$(OSMAND_JPEG)/jcapistd.c \
	$(OSMAND_JPEG)/jccoefct.c \
	$(OSMAND_JPEG)/jccolor.c \
	$(OSMAND_JPEG)/jcdctmgr.c \
	$(OSMAND_JPEG)/jchuff.c \
	$(OSMAND_JPEG)/jcinit.c \
	$(OSMAND_JPEG)/jcmainct.c \
	$(OSMAND_JPEG)/jcmarker.c \
	$(OSMAND_JPEG)/jcmaster.c \
	$(OSMAND_JPEG)/jcomapi.c \
	$(OSMAND_JPEG)/jcparam.c \
	$(OSMAND_JPEG)/jcphuff.c \
	$(OSMAND_JPEG)/jcprepct.c \
	$(OSMAND_JPEG)/jcsample.c \
	$(OSMAND_JPEG)/jctrans.c \
	$(OSMAND_JPEG)/jdapimin.c \
	$(OSMAND_JPEG)/jdapistd.c \
	$(OSMAND_JPEG)/jdatadst.c \
	$(OSMAND_JPEG)/jdatasrc.c \
	$(OSMAND_JPEG)/jdcoefct.c \
	$(OSMAND_JPEG)/jdcolor.c \
	$(OSMAND_JPEG)/jddctmgr.c \
	$(OSMAND_JPEG)/jdhuff.c \
	$(OSMAND_JPEG)/jdinput.c \
	$(OSMAND_JPEG)/jdmainct.c \
	$(OSMAND_JPEG)/jdmarker.c \
	$(OSMAND_JPEG)/jdmaster.c \
	$(OSMAND_JPEG)/jdmerge.c \
	$(OSMAND_JPEG)/jdphuff.c \
	$(OSMAND_JPEG)/jdpostct.c \
	$(OSMAND_JPEG)/jdsample.c \
	$(OSMAND_JPEG)/jdtrans.c \
	$(OSMAND_JPEG)/jerror.c \
	$(OSMAND_JPEG)/jfdctflt.c \
	$(OSMAND_JPEG)/jfdctfst.c \
	$(OSMAND_JPEG)/jfdctint.c \
	$(OSMAND_JPEG)/jidctflt.c \
	$(OSMAND_JPEG)/jidctfst.c \
	$(OSMAND_JPEG)/jidctint.c \
	$(OSMAND_JPEG)/jidctred.c \
	$(OSMAND_JPEG)/jmemmgr.c \
	$(OSMAND_JPEG)/jmemnobs.c \
	$(OSMAND_JPEG)/jquant1.c \
	$(OSMAND_JPEG)/jquant2.c \
	$(OSMAND_JPEG)/jutils.c \

# if WITH_ARITH_ENC from Makefile.am
LOCAL_SRC_FILES += \
	$(OSMAND_JPEG)/jaricom.c \
	$(OSMAND_JPEG)/jcarith.c \
	$(OSMAND_JPEG)/jdarith.c \

# libturbojpeg_la_SOURCES from Makefile.am
LOCAL_SRC_FILES += \
	$(OSMAND_JPEG)/turbojpeg.c \
	$(OSMAND_JPEG)/transupp.c \
	$(OSMAND_JPEG)/jdatadst-tj.c \
	$(OSMAND_JPEG)/jdatasrc-tj.c \

LOCAL_C_INCLUDES += \
	$(OSMAND_JPEG)/simd \
	$(OSMAND_JPEG) \

LOCAL_EXPORT_C_INCLUDES := \
	$(OSMAND_JPEG) \

LOCAL_CFLAGS += \
	-DBUILD="" \
	-DC_ARITH_CODING_SUPPORTED=1 \
	-DD_ARITH_CODING_SUPPORTED=1 \
	-DBITS_IN_JSAMPLE=8 \
	-DHAVE_DLFCN_H=1 \
	-DHAVE_INTTYPES_H=1 \
	-DHAVE_LOCALE_H=1 \
	-DHAVE_MEMCPY=1 \
	-DHAVE_MEMORY_H=1 \
	-DHAVE_MEMSET=1 \
	-DHAVE_STDDEF_H=1 \
	-DHAVE_STDINT_H=1 \
	-DHAVE_STDLIB_H=1 \
	-DHAVE_STRINGS_H=1 \
	-DHAVE_STRING_H=1 \
	-DHAVE_SYS_STAT_H=1 \
	-DHAVE_SYS_TYPES_H=1 \
	-DHAVE_UNISTD_H=1 \
	-DHAVE_UNSIGNED_CHAR=1 \
	-DHAVE_UNSIGNED_SHORT=1 \
	-DINLINE="inline __attribute__((always_inline))" \
	-DJPEG_LIB_VERSION=62 \
	-DLIBJPEG_TURBO_VERSION="2.0.0" \
	-DMEM_SRCDST_SUPPORTED=1 \
	-DNEED_SYS_TYPES_H=1 \
	-DSTDC_HEADERS=1 \
	-DWITH_SIMD=1 \

LOCAL_MODULE := osmand_jpeg

ifneq ($(OSMAND_USE_PREBUILT),true)
	include $(BUILD_STATIC_LIBRARY)
else
	LOCAL_SRC_FILES := \
		$(PROJECT_ROOT_RELATIVE)/libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
	include $(PREBUILT_STATIC_LIBRARY)
endif