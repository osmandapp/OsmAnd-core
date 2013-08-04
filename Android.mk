LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
    LOCAL_MODULE := OsmAndCore
else
    LOCAL_MODULE := OsmAndCore_neon
    LOCAL_ARM_NEON := true
endif

LOCAL_EXPORT_CFLAGS := \
    -DOSMAND_TARGET_OS_android \
    -DOSMAND_OPENGLES2_RENDERER_SUPPORTED \
    -DOSMAND_CORE_STATIC \
    -DGLM_SWIZZLE

LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/externals/glm/upstream.patched \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/client \
    $(LOCAL_PATH)/protos

ifeq ($(LOCAL_ARM_NEON),true)
    OSMAND_BINARY_SUFFIX := _neon
else
    OSMAND_BINARY_SUFFIX :=
endif

LOCAL_STATIC_LIBRARIES := \
    osmand_protobuf$(OSMAND_BINARY_SUFFIX) \
    osmand_jpeg$(OSMAND_BINARY_SUFFIX) \
    osmand_ft2$(OSMAND_BINARY_SUFFIX) \
    osmand_png$(OSMAND_BINARY_SUFFIX) \
    osmand_gif$(OSMAND_BINARY_SUFFIX) \
    osmand_expat$(OSMAND_BINARY_SUFFIX) \
    osmand_skia$(OSMAND_BINARY_SUFFIX) \
    osmand_gdal$(OSMAND_BINARY_SUFFIX) \
    osmand_glsl-optimizer$(OSMAND_BINARY_SUFFIX) \
    Qt5Core$(OSMAND_BINARY_SUFFIX) \
    Qt5Network$(OSMAND_BINARY_SUFFIX) \
    Qt5Sql$(OSMAND_BINARY_SUFFIX)

ifneq ($(OSMAND_USE_PREBUILT),true)
    LOCAL_CFLAGS := \
        $(LOCAL_EXPORT_CFLAGS)

    ifdef OSMAND_PROFILE_NATIVE_OPERATIONS
        LOCAL_CFLAGS += \
            -DOSMAND_NATIVE_PROFILING
    endif

    LOCAL_C_INCLUDES := \
        $(LOCAL_EXPORT_C_INCLUDES) \
        $(LOCAL_PATH)/include/OsmAndCore \
        $(LOCAL_PATH)/include/OsmAndCore/Data \
        $(LOCAL_PATH)/include/OsmAndCore/Data/Model \
        $(LOCAL_PATH)/include/OsmAndCore/Routing \
        $(LOCAL_PATH)/include/OsmAndCore/Map \
        $(LOCAL_PATH)/src/ \
        $(LOCAL_PATH)/src/Data \
        $(LOCAL_PATH)/src/Data/Model \
        $(LOCAL_PATH)/src/Routing \
        $(LOCAL_PATH)/src/Map

    SRC_FILES := \
        $(wildcard $(LOCAL_PATH)/src/*.c*) \
        $(wildcard $(LOCAL_PATH)/src/Data/*.c*) \
        $(wildcard $(LOCAL_PATH)/src/Data/Model/*.c*) \
        $(wildcard $(LOCAL_PATH)/src/Routing/*.c*) \
        $(wildcard $(LOCAL_PATH)/src/Map/*.c*) \
        $(wildcard $(LOCAL_PATH)/src/Map/OpenGL_Common/*.c*) \
        $(wildcard $(LOCAL_PATH)/src/Map/OpenGLES2/*.c*) \
        $(wildcard $(LOCAL_PATH)/client/*.c*) \
        $(wildcard $(LOCAL_PATH)/protos/*.c*)
    LOCAL_CPP_EXTENSION := .cc .cpp
    LOCAL_SRC_FILES := \
        $(SRC_FILES:$(LOCAL_PATH)/%=%)

    include $(BUILD_STATIC_LIBRARY)
else
    LOCAL_SRC_FILES := \
        $(OSMAND_ANDROID_PREBUILT_ROOT)/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
    include $(PREBUILT_STATIC_LIBRARY)
endif
