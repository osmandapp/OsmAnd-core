LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
    LOCAL_MODULE := osmand_glsl-optimizer
else
    LOCAL_MODULE := osmand_glsl-optimizer_neon
    LOCAL_ARM_NEON := true
endif

LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/public_include

ifneq ($(OSMAND_USE_PREBUILT),true)
    LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/upstream.patched/src/mesa \
        $(LOCAL_PATH)/upstream.patched/src/mapi \
        $(LOCAL_PATH)/upstream.patched/src/glsl \
        $(LOCAL_PATH)/upstream.patched/include

    libglcpp_SRC_FILES := $(wildcard $(LOCAL_PATH)/upstream.patched/src/glsl/glcpp/*.c)
    mesa_SRC_FILES := $(wildcard $(LOCAL_PATH)/upstream.patched/src/mesa/*.c)
    _glsl_SRC_FILES := $(wildcard $(LOCAL_PATH)/upstream.patched/src/glsl/*.c*)
    glsl_SRC_FILES := $(filter-out %/upstream.patched/src/glsl/main.cpp,$(_glsl_SRC_FILES))

    LOCAL_SRC_FILES := \
        $(libglcpp_SRC_FILES:$(LOCAL_PATH)/%=%) \
        $(mesa_SRC_FILES:$(LOCAL_PATH)/%=%) \
        $(glsl_SRC_FILES:$(LOCAL_PATH)/%=%)

    include $(BUILD_STATIC_LIBRARY)
else
    LOCAL_SRC_FILES := \
        $(OSMAND_ANDROID_PREBUILT_ROOT)/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
    include $(PREBUILT_STATIC_LIBRARY)
endif
