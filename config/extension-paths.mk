# Generic SDK extension headers. Project-specific policy belongs to user projects.
NXSDK_EXTENSION_INCLUDE_REL := \
  lib/extensions/input/include
NXSDK_EXTENSION_INCLUDE_DIRS := $(addprefix $(NXSDK_ROOT)/,$(NXSDK_EXTENSION_INCLUDE_REL))
NXSDK_EXTENSION_INCLUDE_FLAGS := $(addprefix -I,$(NXSDK_EXTENSION_INCLUDE_DIRS))
