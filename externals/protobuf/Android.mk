LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
    LOCAL_MODULE := osmand_protobuf
else
    LOCAL_MODULE := osmand_protobuf_neon
    LOCAL_ARM_NEON := true
endif

LOCAL_EXPORT_CFLAGS := \
    -DGOOGLE_PROTOBUF_NO_RTTI
LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/upstream.patched/src

LOCAL_CFLAGS := \
    $(LOCAL_EXPORT_CFLAGS)

LOCAL_C_INCLUDES := \
    $(LOCAL_EXPORT_C_INCLUDES) \
    $(LOCAL_PATH) \

LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    upstream.patched/src/google/protobuf/extension_set.cc \
    upstream.patched/src/google/protobuf/generated_message_util.cc \
    upstream.patched/src/google/protobuf/io/coded_stream.cc \
    upstream.patched/src/google/protobuf/io/zero_copy_stream.cc \
    upstream.patched/src/google/protobuf/io/zero_copy_stream_impl.cc \
    upstream.patched/src/google/protobuf/io/zero_copy_stream_impl_lite.cc \
    upstream.patched/src/google/protobuf/message_lite.cc \
    upstream.patched/src/google/protobuf/repeated_field.cc \
    upstream.patched/src/google/protobuf/stubs/common.cc \
    upstream.patched/src/google/protobuf/stubs/once.cc \
    upstream.patched/src/google/protobuf/wire_format_lite.cc

include $(BUILD_STATIC_LIBRARY)
