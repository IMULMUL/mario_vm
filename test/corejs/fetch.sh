#!/usr/bin/env bash
# fetch.sh - obtain the single-file global core-js 3 bundle used by the harness.
#
# The bundle is a ~910KB webpack IIFE that pollutes the global object with
# polyfills; it needs no CommonJS `require` (which mario lacks), unlike
# core-js-pure. It is fetched on demand and kept gitignored (not committed).
set -eu
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
BUNDLE="$HERE/corejs_bundle.js"
URL="https://cdn.jsdelivr.net/npm/core-js-bundle@3/index.js"

if [ -f "$BUNDLE" ]; then
  echo "bundle already present: $BUNDLE ($(wc -c < "$BUNDLE") bytes)"
  exit 0
fi

# Fast path: reuse a copy already downloaded during investigation.
if [ -f "$ROOT/_scratch/corejs_bundle.js" ]; then
  cp "$ROOT/_scratch/corejs_bundle.js" "$BUNDLE"
  echo "copied from _scratch: $BUNDLE ($(wc -c < "$BUNDLE") bytes)"
  exit 0
fi

echo "fetching $URL"
curl -fsSL "$URL" -o "$BUNDLE"
echo "saved: $BUNDLE ($(wc -c < "$BUNDLE") bytes)"
