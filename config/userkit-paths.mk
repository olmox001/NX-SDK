# NX-USERKIT SDK-side public interfaces. Server entrypoints never live here.
NXSDK_USERKIT_INCLUDE_REL := \
  lib/extensions/USERKIT/manager/include \
  lib/extensions/USERKIT/nxexecd/include \
  lib/extensions/USERKIT/nxntfy_srv/include \
  lib/extensions/USERKIT/nxpaxd/include \
  lib/extensions/USERKIT/nxtimerdd/include \
  lib/extensions/USERKIT/nxttyd/include \
  lib/extensions/USERKIT/sbin/common/include \
  lib/extensions/USERKIT/sbin/nxinit/include \
  lib/extensions/USERKIT/sbin/nxdemo1init/include \
  lib/extensions/USERKIT/example/nxexample_echo/include
NXSDK_USERKIT_INCLUDE_DIRS := $(addprefix $(NXSDK_ROOT)/,$(NXSDK_USERKIT_INCLUDE_REL))
NXSDK_USERKIT_INCLUDE_FLAGS := $(addprefix -I,$(NXSDK_USERKIT_INCLUDE_DIRS))
