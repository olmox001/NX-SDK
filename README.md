# NX-SDK

NX-SDK is the independently versioned NexsOS userland contract and runtime.

It owns:
- public API and object/capability contract headers under `include/`;
- architecture startup/runtime veneers under `arch/`;
- libc/native runtime, graphics helpers and service client extensions under `lib/`;
- portability adapters owned by NexsOS.

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
