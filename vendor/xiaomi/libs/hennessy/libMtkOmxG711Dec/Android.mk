LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libMtkOmxG711Dec
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libbinder libui libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libMtkOmxG711Dec.so
include $(BUILD_PREBUILT)
