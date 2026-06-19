#!/usr/bin/env bash
# Upload debug symbols (ELF+.debug / PE+DWARF / PDB) to Sentry via sentry-cli.
# Invoked from rpm spec (%check) and deb rules (override_dh_strip).
# Silently skips when SENTRY_AUTH_TOKEN is unset so source builds and forks
# without secrets are not blocked. Network/auth errors never fail the build.

set -eu

TARGET="${1:-.}"

if [ -z "${SENTRY_AUTH_TOKEN:-}" ]; then
  echo "upload-symbols: SENTRY_AUTH_TOKEN unset, skipping"
  exit 0
fi

: "${SENTRY_ORG:?SENTRY_ORG must be set when SENTRY_AUTH_TOKEN is}"
: "${SENTRY_PROJECT:?SENTRY_PROJECT must be set when SENTRY_AUTH_TOKEN is}"

if ! command -v sentry-cli >/dev/null 2>&1; then
  echo "upload-symbols: installing sentry-cli to /tmp/sentry-cli"
  mkdir -p /tmp/sentry-cli
  curl -sL https://sentry.io/get-cli/ | INSTALL_DIR=/tmp/sentry-cli bash
  export PATH="/tmp/sentry-cli:$PATH"
fi

sentry-cli debug-files upload --include-sources --wait "$TARGET" || {
  echo "upload-symbols: upload failed (non-fatal)"
  exit 0
}
