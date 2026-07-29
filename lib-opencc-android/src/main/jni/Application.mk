APP_ABI := arm64-v8a armeabi-v7a x86_64 x86
APP_STL := c++_static
APP_CPPFLAGS := -fexceptions
APP_OPTIM := release
APP_CFLAGS := -Oz -fvisibility=hidden -ffunction-sections -fdata-sections
APP_CFLAGS += -fno-rtti -fmerge-all-constants -fno-asynchronous-unwind-tables -fno-stack-protector
APP_CFLAGS += -flto=thin
APP_LDFLAGS := -Wl,--gc-sections -Wl,--icf=all -Wl,-s
APP_LDFLAGS += -flto=thin