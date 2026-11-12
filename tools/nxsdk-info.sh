#!/usr/bin/env bash
set -euo pipefail
SDK_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "$SDK_ROOT/config/sdk-version.env"
printf 'root=%s\nversion=%s\ntools_abi=%s\nheaders_abi=%s\nruntime_abi=%s\n' \
  "$SDK_ROOT" "$NEXS_SDK_VERSION" "$NEXS_SDK_TOOLS_ABI" \
  "$NEXS_SDK_HEADERS_ABI" "$NEXS_SDK_RUNTIME_ABI"
