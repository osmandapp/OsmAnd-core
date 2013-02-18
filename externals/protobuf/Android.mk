LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

PROJECT_ROOT_RELATIVE := ../../../platforms/android/OsmAnd
OSMAND_PROTOBUF_ROOT_RELATIVE := .
OSMAND_PROTOBUF_ROOT := $(LOCAL_PATH)/$(OSMAND_PROTOBUF_ROOT_RELATIVE)
OSMAND_PROTOBUF_RELATIVE := ./upstream.patched
OSMAND_PROTOBUF := $(LOCAL_PATH)/$(OSMAND_PROTOBUF_RELATIVE)

LOCAL_C_INCLUDES += \
	$(OSMAND_PROTOBUF_ROOT) \
	$(OSMAND_PROTOBUF)/src
	
LOCAL_CFLAGS += -DGOOGLE_PROTOBUF_NO_RTTI -fPIC

LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
	$(OSMAND_PROTOBUF_RELATIVE)/src/google/protobuf/extension_set.cc \
	$(OSMAND_PROTOBUF_RELATIVE)/src/google/protobuf/generated_message_util.cc \
	$(OSMAND_PROTOBUF_RELATIVE)/src/google/protobuf/io/coded_stream.cc \
	$(OSMAND_PROTOBUF_RELATIVE)/src/google/protobuf/io/zero_copy_stream.cc \
	$(OSMAND_PROTOBUF_RELATIVE)/src/google/protobuf/io/zero_copy_stream_impl.cc \
	$(OSMAND_PROTOBUF_RELATIVE)/src/google/protobuf/io/zero_copy_stream_impl_lite.cc \
	$(OSMAND_PROTOBUF_RELATIVE)/src/google/protobuf/message_lite.cc \
	$(OSMAND_PROTOBUF_RELATIVE)/src/google/protobuf/repeated_field.cc \
	$(OSMAND_PROTOBUF_RELATIVE)/src/google/protobuf/stubs/common.cc \
	$(OSMAND_PROTOBUF_RELATIVE)/src/google/protobuf/stubs/once.cc \
	$(OSMAND_PROTOBUF_RELATIVE)/src/google/protobuf/wire_format_lite.cc

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
	LOCAL_MODULE := osmand_protobuf
else
	LOCAL_MODULE := osmand_protobuf_neon
	LOCAL_ARM_NEON := true
endif

ifneq ($(OSMAND_USE_PREBUILT),true)
	include $(BUILD_STATIC_LIBRARY)
else
	LOCAL_SRC_FILES := \
		$(PROJECT_ROOT_RELATIVE)/libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
	include $(PREBUILT_STATIC_LIBRARY)
endif