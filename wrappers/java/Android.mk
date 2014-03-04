LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
    LOCAL_MODULE := OsmAndCoreJNI
else
    LOCAL_MODULE := OsmAndCoreJNI_neon
    LOCAL_ARM_NEON := true
endif

ifeq ($(LOCAL_ARM_NEON),true)
    OSMAND_BINARY_SUFFIX := _neon
else
    OSMAND_BINARY_SUFFIX :=
endif

LOCAL_STATIC_LIBRARIES := OsmAndCore$(OSMAND_BINARY_SUFFIX)

_HEADER_FILES := \
    $(wildcard $(LOCAL_PATH)/include/*.h) \
    $(wildcard $(LOCAL_PATH)/include/OsmAndCore/*.h) \
    $(wildcard $(LOCAL_PATH)/include/OsmAndCore/Data/*.h) \
    $(wildcard $(LOCAL_PATH)/include/OsmAndCore/Data/Model/*.h) \
    $(wildcard $(LOCAL_PATH)/include/OsmAndCore/Routing/*.h) \
    $(wildcard $(LOCAL_PATH)/include/OsmAndCore/Map/*.h)
HEADER_FILES := $(_HEADER_FILES:$(LOCAL_PATH)/%=%)

SWIG_FILES := \
    $(wildcard $(LOCAL_PATH)/*.swig) \
    $(wildcard $(LOCAL_PATH)/swig/*)
SWIG_FILES := $(_SWIG_FILES:$(LOCAL_PATH)/%=%)

LOCAL_GENERATOR_INPUT := \
    $(_HEADER_FILES) \
    $(_SWIG_FILES)

LOCAL_SRC_FILES := \
    gen/cpp/swig.cpp

WRAPPER_JAVA_PROJECT_ROOT := $(LOCAL_PATH)

$(LOCAL_PATH)/gen/cpp/swig.cpp: $(LOCAL_GENERATOR_INPUT) $(LOCAL_PATH)/generate.sh
	@echo "Generating..."
	@$(WRAPPER_JAVA_PROJECT_ROOT)/generate.sh

include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/../../Android.mk
