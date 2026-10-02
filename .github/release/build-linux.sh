#!/bin/bash
# Builds ClaFi applications as Linux release executables, then checks what each one asks of the
# machine it lands on. Runs inside the image the Containerfile beside it describes.
#
#   build-linux.sh <source dir> <work dir> <target>...
#
# The build tree goes to <work dir>/build and the executables to <work dir>/bin.

set -euo pipefail

src=$1
work=$2
shift 2
[ "$#" -gt 0 ] || { echo "no target given" >&2; exit 1; }

toolchain="$(dirname "$(readlink -f "$0")")/clafi-static.cmake"

# A toolchain file is read into the cache once, so a changed one never reaches an existing build
# tree. Its hash is kept beside the tree, and the tree is dropped when the hash moves.
mkdir -p "$work"
stamp="$work/toolchain.sha"
hash=$(sha256sum "$toolchain" | cut -d' ' -f1)
if [ ! -f "$stamp" ] || [ "$(cat "$stamp")" != "$hash" ]; then
    rm -rf "$work/build"
    printf '%s\n' "$hash" > "$stamp"
fi

cmake -S "$src" -B "$work/build" -G Ninja -Wno-experimental \
    -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
    -DCLAFI_PLATFORM=wayland \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$work/bin"
cmake --build "$work/build" --target "$@" -- -k 0

# The oldest glibc an executable starts on is the newest symbol version it asks for.
status=0
for target in "$@"; do
    executable="$work/bin/$target"
    echo "--- $target"
    needed=$(objdump -p "$executable" | sed -n 's/^ *NEEDED *//p')
    printf '%s\n' "$needed" | sed 's/^/      /'
    for unwanted in libc++.so.1 libc++abi.so.1 libunibreak.so; do
        if printf '%s\n' "$needed" | grep -qF "$unwanted"; then
            echo "      STOP  $unwanted is a shared dependency"
            status=1
        fi
    done
    floor=$(objdump -T "$executable" | grep -o 'GLIBC_2\.[0-9]*' | sort -uV | tail -n 1)
    echo "      glibc floor: ${floor:-none named}"
    if [ "$(printf '%s\n%s\n' "$floor" GLIBC_2.35 | sort -V | tail -n 1)" != GLIBC_2.35 ]; then
        echo "      STOP  newer than GLIBC_2.35"
        status=1
    fi
done
exit $status
