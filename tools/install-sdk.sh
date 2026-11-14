#!/usr/bin/env bash
set -euo pipefail
SDK_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "$SDK_ROOT/config/sdk-version.env"
DATA_HOME="${XDG_DATA_HOME:-$HOME/.local/share}"
PREFIX="${NEXS_SDK_INSTALL_PREFIX:-$DATA_HOME/nexsos/sdk}"
COMPONENT=all
ARCH=""
VARIANT=full
BUILD_ROOT="$SDK_ROOT/build"
usage() {
  cat <<USAGE
usage: $0 [--prefix PATH] [--component tools|headers|runtime|all]
          [--arch amd64|aarch64] [--variant core|full] [--build-root PATH]
USAGE
}
while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix) PREFIX="$2"; shift 2 ;;
    --component) COMPONENT="$2"; shift 2 ;;
    --arch) ARCH="$2"; shift 2 ;;
    --variant) VARIANT="$2"; shift 2 ;;
    --build-root) BUILD_ROOT="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "NX-SDK install: unknown argument: $1" >&2; usage >&2; exit 2 ;;
  esac
done
case "$COMPONENT" in tools|headers|runtime|all) ;; *) echo "invalid component: $COMPONENT" >&2; exit 2 ;; esac
case "$VARIANT" in core|full) ;; *) echo "invalid runtime variant: $VARIANT" >&2; exit 2 ;; esac
if [ "$COMPONENT" = runtime ] || [ "$COMPONENT" = all ]; then
  case "$ARCH" in amd64|aarch64) ;; *) echo "runtime install requires --arch amd64|aarch64" >&2; exit 2 ;; esac
fi
DEST="$PREFIX/$NEXS_SDK_VERSION"
mkdir -p "$DEST"
install_tools() {
  mkdir -p "$DEST/tools" "$DEST/config"
  cp "$SDK_ROOT/config/toolchain-versions.env" "$DEST/config/"
  cp "$SDK_ROOT/config/sdk-version.env" "$DEST/config/"
  cp "$SDK_ROOT/config/extension-paths.mk" "$DEST/config/"
  cp "$SDK_ROOT/config/userkit-paths.mk" "$DEST/config/"
  cp "$SDK_ROOT/sdk.mk" "$DEST/"
  cp "$SDK_ROOT/tools/setup-toolchain-linux.sh" "$DEST/tools/"
  cp "$SDK_ROOT/tools/setup-toolchain-macos.sh" "$DEST/tools/"
  cp "$SDK_ROOT/tools/check-contract-snapshot.py" "$DEST/tools/"
  rm -rf "$DEST/external/stb" "$DEST/tools/userkit"
  mkdir -p "$DEST/external"
  cp -R "$SDK_ROOT/external/stb" "$DEST/external/stb"
  cp -R "$SDK_ROOT/tools/userkit" "$DEST/tools/userkit"
  chmod +x "$DEST/tools/"*.sh "$DEST/tools/check-contract-snapshot.py"
  find "$DEST/tools/userkit" -type f -name '*.py' -exec chmod +x {} +
}
install_headers() {
  rm -rf "$DEST/include.tmp" "$DEST/lib/extensions"
  mkdir -p "$DEST/include.tmp"
  cp -R "$SDK_ROOT/include/." "$DEST/include.tmp/"
  rm -rf "$DEST/include"
  mv "$DEST/include.tmp" "$DEST/include"
  while IFS= read -r rel; do
    [ -n "$rel" ] || continue
    src="$SDK_ROOT/$rel"
    dst="$DEST/$rel"
    mkdir -p "$(dirname "$dst")"
    cp -R "$src" "$dst"
  done <<'HDRS'
lib/extensions/input/include
lib/extensions/USERKIT/manager/include
lib/extensions/USERKIT/nxexecd/include
lib/extensions/USERKIT/nxntfy_srv/include
lib/extensions/USERKIT/nxpaxd/include
lib/extensions/USERKIT/nxtimerdd/include
lib/extensions/USERKIT/nxttyd/include
lib/extensions/USERKIT/sbin/common/include
lib/extensions/USERKIT/sbin/nxinit/include
lib/extensions/USERKIT/sbin/nxdemo1init/include
lib/extensions/USERKIT/example/nxexample_echo/include
HDRS
  mkdir -p "$DEST/lib/extensions/USERKIT"
  cp "$SDK_ROOT/lib/extensions/USERKIT/catalog.mk" "$DEST/lib/extensions/USERKIT/"
  cp "$SDK_ROOT/lib/extensions/USERKIT/README.md" "$DEST/lib/extensions/USERKIT/"
  while IFS= read -r project; do
    [ -n "$project" ] || continue
    mkdir -p "$DEST/lib/extensions/USERKIT/$(dirname "$project")"
    cp "$SDK_ROOT/lib/extensions/USERKIT/$project/project.mk"        "$DEST/lib/extensions/USERKIT/$project/project.mk"
  done <<'PROJECTS'
manager
nxexecd
nxntfy_srv
nxtimerdd
nxpaxd
nxcompositor
nxkeyboardd
nxmoused
nxttyd
sbin/common
sbin/nxinit
sbin/nxdemo1init
example/nxnotify_registry
example/nxexample_echo
PROJECTS
}
install_runtime() {
  local archive object_root archive_name
  object_root="$BUILD_ROOT/$ARCH/system"
  if [ "$VARIANT" = core ]; then
    archive="$BUILD_ROOT/$ARCH/libnxsdk-core.a"
    archive_name=libnxsdk-core.a
  else
    archive="$BUILD_ROOT/$ARCH/libnxsdk.a"
    archive_name=libnxsdk.a
  fi
  [ -s "$archive" ] || { echo "NX-SDK install: missing archive: $archive" >&2; exit 1; }
  [ -s "$object_root/arch/$ARCH/syscall.o" ] || { echo "NX-SDK install: missing startup object" >&2; exit 1; }
  mkdir -p "$DEST/runtime/$ARCH/lib" "$DEST/runtime/$ARCH/crt"
  cp "$archive" "$DEST/runtime/$ARCH/lib/$archive_name"
  cp "$object_root/arch/$ARCH/syscall.o" "$DEST/runtime/$ARCH/crt/"
  if [ -s "$object_root/arch/$ARCH/simd_probe.o" ]; then
    cp "$object_root/arch/$ARCH/simd_probe.o" "$DEST/runtime/$ARCH/crt/"
  fi
  cat > "$DEST/runtime/$ARCH/nxsdk-config.mk" <<CFG
NEXS_SDK_VERSION := $NEXS_SDK_VERSION
NEXS_SDK_ROOT := $DEST
NEXS_SDK_INCLUDE := $DEST/include
NEXS_SDK_LIBDIR := $DEST/runtime/$ARCH/lib
NEXS_SDK_CRTDIR := $DEST/runtime/$ARCH/crt
NEXS_SDK_RUNTIME_VARIANT := $VARIANT
CFG
}
case "$COMPONENT" in
  tools) install_tools ;;
  headers) install_headers ;;
  runtime) install_runtime ;;
  all) install_tools; install_headers; install_runtime ;;
esac
ln -sfn "$NEXS_SDK_VERSION" "$PREFIX/current"
printf 'NX-SDK %s: installed %s%s at %s\n' "$NEXS_SDK_VERSION" "$COMPONENT" "${ARCH:+/$ARCH}" "$DEST"
