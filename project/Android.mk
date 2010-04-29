LOCAL_PATH:= $(LOCAL_PATH)/jni

include $(CLEAR_VARS)

LOCAL_MODULE    := libglmap
LOCAL_CFLAGS    := -Werror
LOCAL_SRC_FILES := gl_code.c
LOCAL_LDLIBS    := -llog -lGLESv2

include $(BUILD_SHARED_LIBRARY)
