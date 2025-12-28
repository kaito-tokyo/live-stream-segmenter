#!/bin/bash
set -euo pipefail

QJSC="$(cd "$(dirname "$0")" && pwd)/../vcpkg_installed/universal-osx-obs/tools/quickjs-ng/qjsc"

pushd "$(cd "$(dirname "$0")" && pwd)/../bundle"

bun install

bun build dayjs.ts --outfile=dayjs.bundle.js
"$QJSC" \
  -o ../src/LiveStreamSegmenter/Scripting/dayjs_bundle.c \
  -n 'builtin:dayjs' \
  -N qjsc_dayjs_bundle \
  -m \
  dayjs.bundle.js
clang-format-19 -i ../src/LiveStreamSegmenter/Scripting/dayjs_bundle.c

bun build ini.ts --outfile=ini.bundle.js
"$QJSC" \
  -o ../src/LiveStreamSegmenter/Scripting/ini_bundle.c \
  -n 'builtin:ini' \
  -N qjsc_ini_bundle \
  -m \
  ini.bundle.js
clang-format-19 -i ../src/LiveStreamSegmenter/Scripting/ini_bundle.c

"$QJSC" \
  -o ../src/LiveStreamSegmenter/Scripting/localstorage_bundle.c \
  -N qjsc_localstorage_bundle \
  -C \
  localstorage.js
clang-format-19 -i ../src/LiveStreamSegmenter/Scripting/localstorage_bundle.c
