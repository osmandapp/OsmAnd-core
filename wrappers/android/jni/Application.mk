APP_STL := gnustl_shared
APP_CPPFLAGS := -std=c++11 -fexceptions -frtti
ifeq ($(wildcard $(ANDROID_NDK)/toolchains/*-4.7),)
	NDK_TOOLCHAIN_VERSION := 4.8
else
	NDK_TOOLCHAIN_VERSION := 4.7
endif
APP_ABI := all
APP_PLATFORM := android-8
