#!/usr/bin/env sh
set -eu

REPO_BASE_URL="https://git.pythys.com/taher/multi-repo-manager"
PROJECT_NAME="mrm"
DEFAULT_BIN_DIR="$HOME/.local/bin"

usage() {
  cat <<'EOF'
Usage: install.sh [--version <version>] [--bin-dir <dir>]

Options:
  --version   Release version tag (required)
  --bin-dir   Install directory (default: ~/.local/bin)
  -h, --help  Show this help message
EOF
}

die() {
  printf '%s\n' "error: $*" >&2
  exit 1
}

VERSION=""
BIN_DIR="$DEFAULT_BIN_DIR"

while [ "$#" -gt 0 ]; do
  case "$1" in
    --version)
      shift
      [ "$#" -gt 0 ] || die "missing value for --version"
      VERSION="$1"
      ;;
    --bin-dir)
      shift
      [ "$#" -gt 0 ] || die "missing value for --bin-dir"
      BIN_DIR="$1"
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "unknown argument: $1"
      ;;
  esac
  shift
done

[ -n "$VERSION" ] || die "--version is required"

OS_NAME="$(uname -s | tr '[:upper:]' '[:lower:]')"
case "$OS_NAME" in
  darwin) OS_NAME="darwin" ;;
  linux) OS_NAME="linux" ;;
  *) die "unsupported os: $OS_NAME" ;;
esac

ARCH_NAME="$(uname -m)"
case "$ARCH_NAME" in
  x86_64|amd64) ARCH_NAME="amd64" ;;
  arm64|aarch64) ARCH_NAME="arm64" ;;
  *) die "unsupported arch: $ARCH_NAME" ;;
esac

ASSET_NAME="${PROJECT_NAME}_${VERSION}_${OS_NAME}_${ARCH_NAME}.tar.gz"
RELEASES_URL="${REPO_BASE_URL}/releases"
TAG="$VERSION"
case "$TAG" in
  v*) : ;;
  *) TAG="v$TAG" ;;
esac
DOWNLOAD_URL="${RELEASES_URL}/download/${TAG}/${ASSET_NAME}"

TMP_DIR="$(mktemp -d)"
cleanup() {
  rm -rf "$TMP_DIR"
}
trap cleanup EXIT INT TERM

DOWNLOAD_PATH="$TMP_DIR/$ASSET_NAME"

if command -v curl >/dev/null 2>&1; then
  curl -fsSL "$DOWNLOAD_URL" -o "$DOWNLOAD_PATH"
elif command -v wget >/dev/null 2>&1; then
  wget -qO "$DOWNLOAD_PATH" "$DOWNLOAD_URL"
else
  die "curl or wget is required"
fi

tar -xzf "$DOWNLOAD_PATH" -C "$TMP_DIR"

BIN_PATH="$TMP_DIR/bin/$PROJECT_NAME"
if [ ! -f "$BIN_PATH" ]; then
  die "expected $PROJECT_NAME binary at bin/"
fi

mkdir -p "$BIN_DIR"

if command -v install >/dev/null 2>&1; then
  install -m 755 "$BIN_PATH" "$BIN_DIR/$PROJECT_NAME"
else
  cp "$BIN_PATH" "$BIN_DIR/$PROJECT_NAME"
  chmod 755 "$BIN_DIR/$PROJECT_NAME"
fi

printf '%s\n' "Installed $PROJECT_NAME to $BIN_DIR/$PROJECT_NAME"

case ":$PATH:" in
  *":$BIN_DIR:"*)
    ;;
  *)
    SHELL_NAME="$(basename "${SHELL:-}")"
    PROFILE_FILE="$HOME/.profile"
    case "$SHELL_NAME" in
      zsh) PROFILE_FILE="$HOME/.zshrc" ;;
      bash)
        if [ "$OS_NAME" = "darwin" ]; then
          PROFILE_FILE="$HOME/.bash_profile"
        else
          PROFILE_FILE="$HOME/.bashrc"
        fi
        ;;
    esac

    printf '%s\n' "Add $BIN_DIR to PATH with:"
    printf '%s\n' "echo 'export PATH=\"$BIN_DIR:\$PATH\"' >> \"$PROFILE_FILE\""
    printf '%s\n' "Then restart your shell: exec \$SHELL"
    ;;
esac
