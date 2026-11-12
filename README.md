# NX-SDK

NX-SDK is the independently versioned NexsOS userland contract and runtime.

It owns:
- public API and object/capability contract headers under `include/`;
- architecture startup/runtime veneers under `arch/`;
- libc/native runtime, graphics helpers and service client extensions under `lib/`;
- portability adapters owned by NexsOS;
- versioned development tooling under `tools/` and the canonical toolchain lock under `config/`.

It does not own userland daemons/applications or third-party upstream source trees.

## Build

`make ARCH=amd64 core` and `make ARCH=aarch64 core` build the SDK-owned core without optional media decoder dependencies.

The full integration module also builds the existing image/video extensions. Their upstream headers remain external and must be supplied explicitly:

```sh
make ARCH=amd64 module STB_INCLUDE=/path/to/stb PL_MPEG_INCLUDE=/path/to/pl_mpeg
make ARCH=aarch64 module STB_INCLUDE=/path/to/stb PL_MPEG_INCLUDE=/path/to/pl_mpeg
```

The NexsOS superproject passes its pinned dependency paths. NX-SDK never reaches into a parent checkout implicitly.

`make check ARCH=<arch> ...` verifies the selected cross toolchain and builds the full module.

## Integration contract

`sdk.mk` is the Make integration manifest. Consumers set `NXSDK_ROOT`, `BUILD_DIR` and `ARCH`, include the manifest, and use the exported object sets. The superproject remains responsible for rootfs, release media, QEMU/nxadb acceptance and pinning the exact NX-SDK gitlink revision.

## Development-tool and installation model

NX-SDK is usable directly from its checkout or as a versioned installation.
The install surface is deliberately split into independent components:

- `make install-tools PREFIX=...` installs the cross-toolchain/build tooling,
  toolchain lock, SDK metadata and development-only headers;
- `make install-headers PREFIX=...` installs the public SDK headers;
- `make ARCH=<arch> install-runtime PREFIX=... ...` installs the full target
  runtime for one architecture; `install-runtime-core` installs the core
  runtime profile.

The default prefix follows `NEXS_SDK_INSTALL_PREFIX` or XDG data-directory
conventions and is versioned by `config/sdk-version.env`; `current` selects
the active installation. Updating tools, headers or one target runtime does
not require reinstalling the other components.

NX-CORE consumes NX-SDK only on the development-tool plane. The SDK's libc,
POSIX/userland API headers and runtime archives are not kernel-image inputs.
The separate NX-CORE machine extension is a user-mode artifact and may link
the SDK runtime without changing that kernel boundary.

## USERKIT extension/tooling plane

NX-SDK owns USERKIT public interfaces and reusable clients under `lib/extensions/USERKIT`, plus reusable USERKIT catalog/Power12 tooling under `tools/userkit`. It does not contain USERKIT server entrypoints. Generic `icon` and `input` extensions remain generic SDK extensions rather than being duplicated inside USERKIT.
