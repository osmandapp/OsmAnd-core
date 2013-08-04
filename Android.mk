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
    OSMAND_QT_PATH_SUFFIX := -neon
else
    OSMAND_BINARY_SUFFIX :=
    OSMAND_QT_PATH_SUFFIX :=
endif

LOCAL_STATIC_LIBRARIES := \
    osmand_protobuf$(OSMAND_BINARY_SUFFIX) \
    osmand_skia$(OSMAND_BINARY_SUFFIX) \
    osmand_gdal$(OSMAND_BINARY_SUFFIX) \
    osmand_glsl-optimizer$(OSMAND_BINARY_SUFFIX) \
    Qt5Sql$(OSMAND_BINARY_SUFFIX) \
    Qt5Network$(OSMAND_BINARY_SUFFIX) \
    Qt5Core$(OSMAND_BINARY_SUFFIX)

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

    HEADER_FILES := \
        $(wildcard $(LOCAL_PATH)/include/*.h) \
        $(wildcard $(LOCAL_PATH)/src/*.h) \
        $(wildcard $(LOCAL_PATH)/src/Data/*.h) \
        $(wildcard $(LOCAL_PATH)/src/Data/Model/*.h) \
        $(wildcard $(LOCAL_PATH)/src/Routing/*.h) \
        $(wildcard $(LOCAL_PATH)/src/Map/*.h) \
        $(wildcard $(LOCAL_PATH)/src/Map/OpenGL_Common/*.h) \
        $(wildcard $(LOCAL_PATH)/src/Map/OpenGLES2/*.h) \
        $(wildcard $(LOCAL_PATH)/client/*.h) \
        $(wildcard $(LOCAL_PATH)/protos/*.h)
    mkdirp_ = \
        $(info $(shell ( \
            mkdir -p `dirname $(1)` \
        )))
    mkdirp = \
        $(call mkdirp_,$(1))
    run_moc = \
        $(call mkdirp,$(1:$(LOCAL_PATH)/%=$(LOCAL_PATH)/moc/%)) \
        $(info $(shell ( \
            $(LOCAL_PATH)/externals/qtbase-android/upstream.patched.$(TARGET_ARCH_ABI)$(OSMAND_QT_PATH_SUFFIX)/bin/moc \
                $(1) \
                -f \
                -o $(1:$(LOCAL_PATH)/%.h=$(LOCAL_PATH)/moc/%.cpp) \
            )))
    $(info $(shell (rm -rf $(LOCAL_PATH)/moc)))
    $(foreach header_file,$(HEADER_FILES),$(call run_moc,$(header_file)))
    MOC_FILES := \
        $(shell (find $(LOCAL_PATH)/moc -type f))

    LOCAL_SRC_FILES := \
        $(SRC_FILES:$(LOCAL_PATH)/%=%) \
        $(MOC_FILES:$(LOCAL_PATH)/%=%)

    include $(BUILD_STATIC_LIBRARY)
else
    LOCAL_SRC_FILES := \
        $(OSMAND_ANDROID_PREBUILT_ROOT)/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
    include $(PREBUILT_STATIC_LIBRARY)
endif
