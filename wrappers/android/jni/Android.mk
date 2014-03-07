LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
    LOCAL_MODULE := OsmAndCore_android
else
    LOCAL_MODULE := OsmAndCore_android_neon
    LOCAL_ARM_NEON := true
endif

ifeq ($(LOCAL_ARM_NEON),true)
    OSMAND_BINARY_SUFFIX := _neon
else
    OSMAND_BINARY_SUFFIX :=
endif

LOCAL_STATIC_LIBRARIES := OsmAndCore$(OSMAND_BINARY_SUFFIX)

WRAPPER_JAVA_PROJECT_ROOT := $(LOCAL_PATH)

_HEADER_FILES := \
    $(wildcard $(LOCAL_PATH)/../../../include/*.h) \
    $(wildcard $(LOCAL_PATH)/../../../include/OsmAndCore/*.h) \
    $(wildcard $(LOCAL_PATH)/../../../include/OsmAndCore/Data/*.h) \
    $(wildcard $(LOCAL_PATH)/../../../include/OsmAndCore/Data/Model/*.h) \
    $(wildcard $(LOCAL_PATH)/../../../include/OsmAndCore/Routing/*.h) \
    $(wildcard $(LOCAL_PATH)/../../../include/OsmAndCore/Map/*.h)
HEADER_FILES := $(_HEADER_FILES:$(LOCAL_PATH)/%=%)

SWIG_FILES := \
    $(wildcard $(LOCAL_PATH)/../../../*.swig) \
    $(wildcard $(LOCAL_PATH)/../../../swig/*)
SWIG_FILES := $(_SWIG_FILES:$(LOCAL_PATH)/%=%)

WRAPPER_JAVA_GENERATOR := $(LOCAL_PATH)/../../java/generate.sh
WRAPPER_JAVA_GENERATOR_INPUT := \
    $(_HEADER_FILES) \
    $(_SWIG_FILES)

LOCAL_SRC_FILES := \
    ../../java/gen/cpp/swig.cpp

$(WRAPPER_JAVA_PROJECT_ROOT)/../../java/gen/cpp/swig.cpp: $(WRAPPER_JAVA_GENERATOR_INPUT) $(WRAPPER_JAVA_GENERATOR)
	@echo "Generating..."
	@$(WRAPPER_JAVA_GENERATOR)

include $(BUILD_SHARED_LIBRARY)

_OSMAND_CORE_FOR_ANDROID_MK := $(LOCAL_PATH)/Android.mk
include $(LOCAL_PATH)/../../../Android.mk

ifeq ($(_OSMAND_CORE_FOR_ANDROID_INCLUDED_SELF),)
    _OSMAND_CORE_FOR_ANDROID_INCLUDED_SELF := true

    ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
        OSMAND_BUILDING_NEON_LIBRARY := true
        include $(_OSMAND_CORE_FOR_ANDROID_MK)
        OSMAND_BUILDING_NEON_LIBRARY := false
    endif
endif
