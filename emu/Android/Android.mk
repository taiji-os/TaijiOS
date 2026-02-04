# Android NDK build file for TaijiOS emu/Android platform
# This file is used when building with ndk-build

LOCAL_PATH := $(call my-dir)

# Build the emu library
include $(CLEAR_VARS)
LOCAL_MODULE    := emu-android
LOCAL_SRC_FILES := \
	os.c \
	segflush-arm64.c

LOCAL_CFLAGS    := \
	-Wall -O2 \
	-DANDROID \
	-D__ANDROID__ \
	-DEMUDIR=\"$(LOCAL_PATH)\" \
	-I$(LOCAL_PATH)/../port \
	-I$(LOCAL_PATH)/../../include \
	-I$(LOCAL_PATH)/../../../include \
	-I$(LOCAL_PATH)/../../../libinterp

LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv3 -lm
include $(BUILD_STATIC_LIBRARY)

# Build with pthread support
include $(CLEAR_VARS)
LOCAL_MODULE    := kproc-android
LOCAL_SRC_FILES := ../port/kproc-pthreads.c
LOCAL_CFLAGS    := -Wall -O2 -DANDROID -D__ANDROID__ -DUSE_PTHREADS
LOCAL_LDLIBS    := -lpthread
include $(BUILD_STATIC_LIBRARY)
