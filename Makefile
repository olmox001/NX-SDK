# NX-SDK standalone build entrypoint.
.DEFAULT_GOAL := core

ARCH ?= aarch64
BUILD_ROOT ?= build
NXSDK_ROOT := .
NXSDK_OUTPUT_NAMESPACE ?= system
BUILD_DIR := $(BUILD_ROOT)/$(ARCH)
HOST_OS ?= $(shell uname -s)

ifeq ($(HOST_OS),Darwin)
  AMD64_CROSS_COMPILE ?= x86_64-elf-
  AARCH64_CROSS_COMPILE ?= aarch64-none-elf-
else ifeq ($(HOST_OS),Linux)
  AMD64_CROSS_COMPILE ?= x86_64-elf-
  AARCH64_CROSS_COMPILE ?= aarch64-none-elf-
else
  $(error Unsupported host OS "$(HOST_OS)")
endif

include config/toolchain-versions.env

ifeq ($(findstring amd64,$(ARCH)),amd64)
  override ARCH := amd64
  CROSS_COMPILE ?= $(AMD64_CROSS_COMPILE)
  ARCH_CFLAGS := -DARCH_AMD64 -mno-red-zone -mcmodel=large
else ifeq ($(findstring aarch64,$(ARCH)),aarch64)
  override ARCH := aarch64
  CROSS_COMPILE ?= $(AARCH64_CROSS_COMPILE)
  ARCH_CFLAGS := -DARCH_AARCH64 -mcpu=cortex-a57
else
  $(error ARCH must be amd64 or aarch64)
endif

CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
LD := $(CROSS_COMPILE)ld

NX_WARNINGS_AS_ERRORS ?= 0
NX_STRICT ?= 1
COMMON_FLAGS := -Wall -Wextra -Wpedantic -Wshadow -Wwrite-strings \
  -Wmissing-prototypes -Wstrict-prototypes -Wconversion -Wsign-conversion \
  -ffreestanding -fno-builtin -nostdlib -nostartfiles -fno-common \
  -fstack-protector-strong -fno-pic -fno-pie -fno-omit-frame-pointer -O2 -g
ifeq ($(NX_WARNINGS_AS_ERRORS),1)
COMMON_FLAGS += -Werror
endif
ifeq ($(NX_STRICT),1)
COMMON_FLAGS += -DNX_STRICT
endif

STB_INCLUDE ?=
PL_MPEG_INCLUDE ?=
SDK_EXTERNAL_FLAGS := $(if $(STB_INCLUDE),-I$(STB_INCLUDE)) \
                      $(if $(PL_MPEG_INCLUDE),-I$(PL_MPEG_INCLUDE))

include sdk.mk

SDK_CFLAGS := $(COMMON_FLAGS) $(ARCH_CFLAGS) $(NXSDK_INCLUDE_FLAGS) \
              $(SDK_EXTERNAL_FLAGS) -ffunction-sections -fdata-sections
NXSDK_ARCHIVE := $(BUILD_DIR)/libnxsdk.a

.PHONY: all core module archive check check-contracts check-toolchain check-external clean help
all: core

core: $(NXSDK_CORE_OBJECTS)
	@echo "NX-SDK core built for $(ARCH)"

module: check-external $(NXSDK_MODULE_OBJECTS)
	@echo "NX-SDK full module built for $(ARCH)"

archive: module $(NXSDK_ARCHIVE)
	@echo "NX-SDK archive: $(NXSDK_ARCHIVE)"

$(NXSDK_ARCHIVE): $(NXSDK_LIB_OBJECTS) $(NXSDK_MALLOC_OBJECT)
	@mkdir -p $(dir $@)
	@rm -f $@
	@$(AR) rcs $@ $^

check: check-contracts check-toolchain module
	@echo "NX-SDK $(ARCH) check passed"

check-contracts:
	@python3 tools/check-contract-snapshot.py

check-external:
	@test -n "$(STB_INCLUDE)" && test -f "$(STB_INCLUDE)/stb_image.h" || { \
	  echo "NX-SDK: STB_INCLUDE must point to a directory containing stb_image.h" >&2; exit 1; }
	@test -n "$(PL_MPEG_INCLUDE)" && test -f "$(PL_MPEG_INCLUDE)/pl_mpeg.h" || { \
	  echo "NX-SDK: PL_MPEG_INCLUDE must point to a directory containing pl_mpeg.h" >&2; exit 1; }

check-toolchain:
	@set -eu; \
	for tool in "$(CC)" "$(AR)" "$(LD)"; do \
	  command -v "$$tool" >/dev/null || { echo "missing tool: $$tool" >&2; exit 1; }; \
	done; \
	case "$(ARCH):$$($(CC) -dumpmachine)" in \
	  amd64:x86_64-elf|aarch64:aarch64-none-elf) ;; \
	  *) echo "compiler target mismatch for $(ARCH): $$($(CC) -dumpmachine)" >&2; exit 1;; \
	esac; \
	expected_gcc="$(if $(filter amd64,$(ARCH)),$(NEXS_X86_64_GCC_VERSION),$(NEXS_AARCH64_GCC_VERSION))"; \
	actual_gcc="$$($(CC) -dumpfullversion -dumpversion)"; \
	test "$$actual_gcc" = "$$expected_gcc" || { \
	  echo "GCC $$actual_gcc; expected $$expected_gcc" >&2; exit 1; }; \
	expected_binutils="$(if $(filter amd64,$(ARCH)),$(NEXS_X86_64_BINUTILS_VERSION),$(NEXS_AARCH64_BINUTILS_VERSION))"; \
	actual_binutils="$$($(LD) --version | awk 'NR==1 {print $$NF}')"; \
	test "$$actual_binutils" = "$$expected_binutils" || { \
	  echo "binutils $$actual_binutils; expected $$expected_binutils" >&2; exit 1; }

$(NXSDK_OBJECT_ROOT)/lib/gfx/alpha.o: SDK_CFLAGS += -mgeneral-regs-only

$(NXSDK_OBJECT_ROOT)/lib/%.o: lib/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(SDK_CFLAGS) -MMD -MP -c $< -o $@

$(NXSDK_OBJECT_ROOT)/arch/$(ARCH)/%.o: arch/$(ARCH)/%.S
	@mkdir -p $(dir $@)
	@$(CC) $(SDK_CFLAGS) -c $< -o $@

DEPS := $(shell find "$(NXSDK_OBJECT_ROOT)" -type f -name '*.d' 2>/dev/null)
-include $(DEPS)

clean:
	@rm -rf "$(BUILD_ROOT)"

help:
	@echo "NX-SDK targets:"
	@echo "  core     build SDK-owned core without optional media decoder headers"
	@echo "  module   build the full integration object set"
	@echo "  archive  build the full runtime archive after module"
	@echo "  check    verify toolchain versions and build the full module"
	@echo "Variables: ARCH=amd64|aarch64 BUILD_ROOT=... STB_INCLUDE=... PL_MPEG_INCLUDE=..."
