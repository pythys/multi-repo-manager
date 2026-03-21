#!/usr/bin/env sh
set -eu

usage() {
    cat <<'EOF'
Usage: update-version.sh <version>

Example:
    update-version.sh 1.2.3
EOF
}

die() {
    printf '%s\n' "error: $*" >&2
    exit 1
}

if [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

INPUT_VERSION="$1"
case "$INPUT_VERSION" in
    v*) VERSION="${INPUT_VERSION#v}" ;;
    *) VERSION="$INPUT_VERSION" ;;
esac

case "$VERSION" in
    *[!0-9.]*|'')
        die "invalid version: $INPUT_VERSION"
        ;;
esac

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"

write_version_file() {
    printf '%s\n' "$VERSION" > "$ROOT_DIR/VERSION"
}

update_with_awk() {
    file_path="$1"
    awk_program="$2"
    tmp_path="${file_path}.tmp"
    awk -v version="$VERSION" "$awk_program" "$file_path" > "$tmp_path"
    mv "$tmp_path" "$file_path"
}

write_version_file

update_with_awk "$ROOT_DIR/vcpkg.json" \
    '{ if ($0 ~ /"version"[[:space:]]*:/) { sub(/"version"[[:space:]]*:[[:space:]]*"[^"]+"/, "\"version\": \"" version "\"") } print }'

update_with_awk "$ROOT_DIR/Doxyfile" \
    '{ if ($0 ~ /^PROJECT_NUMBER[[:space:]]*=/) { sub(/=.*/, "= \"" version "\"") } print }'

update_with_awk "$ROOT_DIR/docs/install.md" \
  '{ if ($0 ~ /install.sh/ && $0 ~ /--version/) { gsub(/--version[[:space:]]+[^[:space:]]+/, "--version " version) } print }'

update_with_awk "$ROOT_DIR/README.md" \
  '{ if ($0 ~ /install.sh/ && $0 ~ /--version/) { gsub(/--version[[:space:]]+[^[:space:]]+/, "--version " version) } print }'

printf '%s\n' "Updated version to $VERSION"
