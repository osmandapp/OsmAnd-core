LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
	QT := $(LOCAL_PATH)/../externals/qtbase-android/upstream.patched.$(TARGET_ARCH_ABI)-neon
else
	QT := $(LOCAL_PATH)/../externals/qtbase-android/upstream.patched.$(TARGET_ARCH_ABI)
endif
OSMAND_PROTOBUF := $(LOCAL_PATH)/../externals/protobuf/upstream.patched
OSMAND_CORE := $(LOCAL_PATH)/..
OSMAND_COREUTILS_RELATIVE := .
OSMAND_COREUTILS := $(LOCAL_PATH)/$(OSMAND_COREUTILS_RELATIVE)

LOCAL_C_INCLUDES := \
	$(QT)/include \
	$(QT)/include/QtCore \
    $(OSMAND_CORE)/include \
	$(OSMAND_CORE)/protos \
	$(OSMAND_CORE)/native/include \
	$(OSMAND_PROTOBUF)/src

LOCAL_CPP_EXTENSION := .cpp
SRC_FILES := $(wildcard $(OSMAND_COREUTILS)/*.c*)
LOCAL_SRC_FILES := \
	$(SRC_FILES:$(LOCAL_PATH)/%=%)
	
# Name of the local module
ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
	LOCAL_MODULE := OsmAndCoreUtils
else
	LOCAL_MODULE := OsmAndCoreUtils_neon
	LOCAL_ARM_NEON := true
endif

LOCAL_CFLAGS := \
	-DGOOGLE_PROTOBUF_NO_RTTI \
	-DSK_BUILD_FOR_ANDROID \
	-DSK_BUILD_FOR_ANDROID_NDK \
	-DSK_ALLOW_STATIC_GLOBAL_INITIALIZERS=0 \
	-DSK_RELEASE \
	-DSK_CPU_LENDIAN \
	-DGR_RELEASE=1 \
	-DANDROID_BUILD \
	-fPIC
	
ifeq ($(LOCAL_ARM_NEON),true)
	OSMAND_BINARY_SUFFIX := _neon
else
	OSMAND_BINARY_SUFFIX :=
endif

LOCAL_SHARED_LIBRARIES := \
	OsmAndCore$(OSMAND_BINARY_SUFFIX) \
	Qt5Core$(OSMAND_BINARY_SUFFIX)

include $(BUILD_SHARED_LIBRARY)
