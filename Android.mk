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
    -DOSMAND_GLM_AVAILABLE \
    -DGLM_SWIZZLE

LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/externals/glm/upstream.patched \
    $(LOCAL_PATH)/include

LOCAL_EXPORT_LDLIBS := \
    -lEGL \
    -lGLESv2

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
    osmand_icu4c$(OSMAND_BINARY_SUFFIX) \
    Qt5Sql$(OSMAND_BINARY_SUFFIX) \
    Qt5Network$(OSMAND_BINARY_SUFFIX) \
    Qt5Core$(OSMAND_BINARY_SUFFIX) \
    boost_system$(OSMAND_BINARY_SUFFIX) \
    boost_atomic$(OSMAND_BINARY_SUFFIX) \
    boost_thread$(OSMAND_BINARY_SUFFIX)

LOCAL_CFLAGS := \
    $(LOCAL_EXPORT_CFLAGS)

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
    $(LOCAL_PATH)/src/Map \
    $(LOCAL_PATH)/protos

OSMAND_CORE_PROJECT_ROOT := $(LOCAL_PATH)

_SRC_FILES := \
    $(wildcard $(LOCAL_PATH)/src/*.c*) \
    $(wildcard $(LOCAL_PATH)/src/Data/*.c*) \
    $(wildcard $(LOCAL_PATH)/src/Data/Model/*.c*) \
    $(wildcard $(LOCAL_PATH)/src/Routing/*.c*) \
    $(wildcard $(LOCAL_PATH)/src/Map/*.c*) \
    $(wildcard $(LOCAL_PATH)/src/Map/OpenGL/*.c*) \
    $(wildcard $(LOCAL_PATH)/src/Map/OpenGLES2/*.c*)
SRC_FILES := $(_SRC_FILES:$(LOCAL_PATH)/%=%)

_PROTO_FILES := \
    $(wildcard $(LOCAL_PATH)/protos/*.c*)
PROTO_FILES := $(_PROTO_FILES:$(LOCAL_PATH)/%=%)

_HEADER_FILES := \
    $(wildcard $(LOCAL_PATH)/include/*.h) \
    $(wildcard $(LOCAL_PATH)/include/OsmAndCore/*.h) \
    $(wildcard $(LOCAL_PATH)/include/OsmAndCore/Data/*.h) \
    $(wildcard $(LOCAL_PATH)/include/OsmAndCore/Data/Model/*.h) \
    $(wildcard $(LOCAL_PATH)/include/OsmAndCore/Routing/*.h) \
    $(wildcard $(LOCAL_PATH)/include/OsmAndCore/Map/*.h) \
    $(wildcard $(LOCAL_PATH)/src/*.h) \
    $(wildcard $(LOCAL_PATH)/src/Data/*.h) \
    $(wildcard $(LOCAL_PATH)/src/Data/Model/*.h) \
    $(wildcard $(LOCAL_PATH)/src/Routing/*.h) \
    $(wildcard $(LOCAL_PATH)/src/Map/*.h) \
    $(wildcard $(LOCAL_PATH)/src/Map/OpenGL/*.h) \
    $(wildcard $(LOCAL_PATH)/src/Map/OpenGLES2/*.h) \
    $(wildcard $(LOCAL_PATH)/protos/*.h)
HEADER_FILES := $(_HEADER_FILES:$(LOCAL_PATH)/%=%)

# Rule to get moc'ed file from original
OSMAND_CORE_MOC := $(OSMAND_CORE_PROJECT_ROOT)/externals/qtbase-android/upstream.patched.$(TARGET_ARCH_ABI)$(OSMAND_QT_PATH_SUFFIX).static/bin/moc
MOCED_SRC_FILES := $(addsuffix .cpp, $(addprefix moc/, $(SRC_FILES), $(HEADER_FILES)))
$(info $(MOCED_SRC_FILES))
$(OSMAND_CORE_PROJECT_ROOT)/moc/%.cpp: $(OSMAND_CORE_PROJECT_ROOT)/% $(OSMAND_CORE_MOC)
	@mkdir -p $(dir $@)
	@$(OSMAND_CORE_MOC) -o $@ $<

# Embed resources
EMBED_RESOURCES := $(LOCAL_PROJECT_ROOT)/embed-resources.sh
$(LOCAL_PROJECT_ROOT)/gen/EmbeddedResources_bundle.cpp: $(LOCAL_PROJECT_ROOT)/embed-resources.list $(EMBED_RESOURCES)
	$(EMBED_RESOURCES)

LOCAL_SRC_FILES := \
    $(SRC_FILES) \
    $(PROTO_FILES)
    $(MOCED_SRC_FILES) \
    gen/EmbeddedResources_bundle.cpp

include $(BUILD_STATIC_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH)/externals)
