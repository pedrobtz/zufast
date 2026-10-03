# Shared by tools/vendor/{fetch,record,verify}. Sourced, not run.
#
# Layout:
#   tools/vendor/manifest.tsv     one row per upstream source:
#     source repo tag commit version_string archive archive_sha256 license defines patches
#       archive         comma-separated upstream_path:vendor_path pairs
#       archive_sha256  comma-separated SHA-256 of each upstream file, same order
#       defines         comma-separated NAME or NAME=VALUE configured in
#                       inst/include/zufast/detail/vendor_config.h
#       patches         comma-separated file names under tools/patches/<source>/,
#                       applied in order with `patch -p1` inside the vendor dir
#   tools/vendor/checksums.sha256 SHA-256 of every file in the vendor tree, as installed
set -eu

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
MANIFEST="$ROOT/tools/vendor/manifest.tsv"
CHECKSUMS="$ROOT/tools/vendor/checksums.sha256"
VENDOR="$ROOT/inst/include/zufast/vendor"
PATCHES="$ROOT/tools/patches"
CONFIG="$ROOT/inst/include/zufast/detail/vendor_config.h"

sha256() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | cut -d' ' -f1
    else
        shasum -a 256 "$1" | cut -d' ' -f1
    fi
}

# Print the manifest rows without the header, tab-separated.
manifest_rows() {
    tail -n +2 "$MANIFEST" | grep -v '^[[:space:]]*$'
}

# field N of a tab-separated row
field() {
    printf '%s\n' "$1" | cut -f"$2"
}

# Split a comma-separated list onto lines.
split_list() {
    printf '%s\n' "$1" | tr ',' '\n' | grep -v '^$' || true
}

# Write checksums.sha256 for the vendor tree.
record_checksums() {
    (cd "$VENDOR" && find . -type f | sed 's|^\./||' | LC_ALL=C sort | while read -r f; do
        printf '%s  %s\n' "$(sha256 "$f")" "$f"
    done) > "$CHECKSUMS"
}
