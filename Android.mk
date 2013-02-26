LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
	QT := $(LOCAL_PATH)/../core/externals/qtbase-android/upstream.patched.$(TARGET_ARCH_ABI)-neon
else
	QT := $(LOCAL_PATH)/../core/externals/qtbase-android/upstream.patched.$(TARGET_ARCH_ABI)
endif
OSMAND_PROTOBUF := $(LOCAL_PATH)/externals/protobuf/upstream.patched
OSMAND_SKIA_ROOT := $(LOCAL_PATH)/externals/skia
OSMAND_SKIA := $(LOCAL_PATH)/externals/skia/upstream.patched
OSMAND_EXPAT := $(LOCAL_PATH)/externals/expat/upstream.patched
OSMAND_CORE_RELATIVE := .
OSMAND_CORE := $(LOCAL_PATH)/$(OSMAND_CORE_RELATIVE)

LOCAL_C_INCLUDES := \
	$(QT)/include/QtCore \
    $(LOCAL_PATH)/src \
	$(OSMAND_PROTOBUF)/src \
	$(OSMAND_SKIA_ROOT) \
	$(OSMAND_SKIA) \
	$(OSMAND_EXPAT)/lib \
	$(OSMAND_SKIA)/include/core \
	$(OSMAND_SKIA)/include/config \
	$(OSMAND_SKIA)/include/effects \
	$(OSMAND_SKIA)/include/images \
	$(OSMAND_SKIA)/include/ports \
	$(OSMAND_SKIA)/include/utils \
	$(OSMAND_SKIA)/include/utils/android \
	$(OSMAND_SKIA)/src/core \
	$(OSMAND_CORE)/include \
	$(OSMAND_CORE)/src \
	$(OSMAND_CORE)/native/include \
	$(OSMAND_CORE)/native/src

LOCAL_CPP_EXTENSION := .cc .cpp
LOCAL_SRC_FILES := \
	$(OSMAND_CORE_RELATIVE)/src/Logging.cpp \
	$(OSMAND_CORE_RELATIVE)/native/src/ElapsedTimer.cpp \
	$(OSMAND_CORE_RELATIVE)/native/src/common.cpp \
	$(OSMAND_CORE_RELATIVE)/native/src/mapObjects.cpp \
	$(OSMAND_CORE_RELATIVE)/native/src/multipolygons.cpp \
	$(OSMAND_CORE_RELATIVE)/native/src/renderRules.cpp \
	$(OSMAND_CORE_RELATIVE)/native/src/rendering.cpp \
	$(OSMAND_CORE_RELATIVE)/native/src/binaryRead.cpp \
	$(OSMAND_CORE_RELATIVE)/native/src/binaryRoutePlanner.cpp \
	$(OSMAND_CORE_RELATIVE)/native/src/proto/osmand_index.pb.cc \
	$(OSMAND_CORE_RELATIVE)/native/src/java_wrap.cpp
	
ifdef OSMAND_PROFILE_NATIVE_OPERATIONS
	LOCAL_CFLAGS += \
		-DOSMAND_NATIVE_PROFILING
endif

# Name of the local module
ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
	LOCAL_MODULE := OsmAndCore
else
	LOCAL_MODULE := OsmAndCore_neon
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

LOCAL_STATIC_LIBRARIES := \
	osmand_protobuf$(OSMAND_BINARY_SUFFIX) \
	osmand_jpeg$(OSMAND_BINARY_SUFFIX) \
	osmand_ft2$(OSMAND_BINARY_SUFFIX) \
	osmand_png$(OSMAND_BINARY_SUFFIX) \
	osmand_gif$(OSMAND_BINARY_SUFFIX) \
	osmand_expat$(OSMAND_BINARY_SUFFIX)
LOCAL_WHOLE_STATIC_LIBRARIES := osmand_skia$(OSMAND_BINARY_SUFFIX)
LOCAL_SHARED_LIBRARIES := \
	Qt5Core$(OSMAND_BINARY_SUFFIX) \
	Qt5Network$(OSMAND_BINARY_SUFFIX) \
	Qt5Xml$(OSMAND_BINARY_SUFFIX) \
	Qt5Sql$(OSMAND_BINARY_SUFFIX) \
	Qt5Concurrent$(OSMAND_BINARY_SUFFIX)

LOCAL_LDLIBS := -lz -llog -ldl

include $(BUILD_SHARED_LIBRARY)
