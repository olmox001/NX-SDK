# NX-SDK integration manifest.
# Consumers set NXSDK_ROOT and BUILD_DIR before including this file.
NXSDK_ROOT ?= system
NXSDK_OUTPUT_NAMESPACE ?= system
NXSDK_BUILD_DIR ?= $(BUILD_DIR)
NXSDK_OBJECT_ROOT := $(NXSDK_BUILD_DIR)/$(NXSDK_OUTPUT_NAMESPACE)

include $(NXSDK_ROOT)/config/extension-paths.mk
include $(NXSDK_ROOT)/config/userkit-paths.mk
NXSDK_INCLUDE_FLAGS := -I$(NXSDK_ROOT)/include/contract -I$(NXSDK_ROOT)/include/api $(NXSDK_EXTENSION_INCLUDE_FLAGS) $(NXSDK_USERKIT_INCLUDE_FLAGS)

NXSDK_CORE_LIB_OBJECTS := \
  $(NXSDK_OBJECT_ROOT)/lib/lib.o \
  $(NXSDK_OBJECT_ROOT)/lib/posix_environment.o \
  $(NXSDK_OBJECT_ROOT)/lib/gfx/alpha.o \
  $(NXSDK_OBJECT_ROOT)/lib/gfx/raster.o \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/LIBGRAPHICS/chrome.o \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/LIBGRAPHICS/decoration.o \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/LIBGRAPHICS/theme.o \
  $(NXSDK_OBJECT_ROOT)/lib/math.o \
  $(NXSDK_OBJECT_ROOT)/lib/font_lib.o \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/USERKIT/nxntfy_srv/client.o \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/object/port_client.o \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/USERKIT/nxexecd/client.o \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/USERKIT/nxtimerdd/client.o \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/input/nxkeyboard_layout.o \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/USERKIT/example/nxexample_echo/client.o \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/graphics/surface_client.o \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/graphics/gfx_buffer.o \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/object/buffer_client.o \
  $(NXSDK_OBJECT_ROOT)/lib/portability/os1_video_platform.o \
  $(NXSDK_OBJECT_ROOT)/lib/portability/d3d9/os1_d3d9_present.o \
  $(NXSDK_OBJECT_ROOT)/lib/portability/opengl/os1_gl_platform.o

NXSDK_MEDIA_OBJECTS := \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/image/image.o \
  $(NXSDK_OBJECT_ROOT)/lib/extensions/video/video.o

NXSDK_LIB_OBJECTS := $(NXSDK_CORE_LIB_OBJECTS) $(NXSDK_MEDIA_OBJECTS)
NXSDK_MALLOC_OBJECT := $(NXSDK_OBJECT_ROOT)/lib/malloc.o
NXSDK_SYSCALL_OBJECT := $(NXSDK_OBJECT_ROOT)/arch/$(ARCH)/syscall.o
NXSDK_SIMD_OBJECT := $(NXSDK_OBJECT_ROOT)/arch/$(ARCH)/simd_probe.o
NXSDK_RUNTIME_OBJECTS := $(NXSDK_LIB_OBJECTS) $(NXSDK_MALLOC_OBJECT) $(NXSDK_SYSCALL_OBJECT)
NXSDK_MODULE_OBJECTS := $(NXSDK_RUNTIME_OBJECTS) $(NXSDK_SIMD_OBJECT)
NXSDK_CORE_OBJECTS := $(NXSDK_CORE_LIB_OBJECTS) $(NXSDK_MALLOC_OBJECT) $(NXSDK_SYSCALL_OBJECT) $(NXSDK_SIMD_OBJECT)
