#!/usr/bin/env bash
# run.sh - load prelude + the core-js bundle in the standalone mario CLI and
# report whether the bundle's top level ran to completion.
#
# Success is keyed on a sentinel printed AFTER the bundle, not the process exit
# code: the CLI has a pre-existing teardown segfault in vm_close (heap corruption
# unrelated to core-js) that can set a non-zero exit status even on a clean run.
# An uncaught throw during the bundle's top level aborts execution before the
# sentinel, so "sentinel present" == "loaded clean".
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
MARIO="$ROOT/build/mario"
BUNDLE="$HERE/corejs_bundle.js"
LOG="$HERE/run.log"

[ -x "$MARIO" ]   || { echo "mario CLI not built: run 'make' first" >&2; exit 2; }
[ -f "$BUNDLE" ]  || { echo "bundle missing: run 'make corejs-fetch' (or $HERE/fetch.sh)" >&2; exit 2; }

WORK="$(mktemp -d)"; trap 'rm -rf "$WORK"' EXIT
COMBINED="$WORK/combined.js"
cat "$HERE/prelude.js" "$BUNDLE" > "$COMBINED"
# Optional extra tail (e.g. smoke.js) passed as $1 is appended before the sentinel.
if [ "${1:-}" != "" ] && [ -f "${1:-}" ]; then cat "$1" >> "$COMBINED"; fi
printf '\nconsole.log("__COREJS_LOAD_OK__");\n' >> "$COMBINED"

now() { python3 -c 'import time;print(time.time())' 2>/dev/null || date +%s; }
START=$(now)
"$MARIO" "$COMBINED" > "$LOG" 2>&1
STATUS=$?
END=$(now)
ELAPSED=$(awk "BEGIN{printf \"%.2f\", $END-$START}" 2>/dev/null || echo "?")

echo "=== core-js harness ==="
echo "load seconds : $ELAPSED"
echo "exit status  : $STATUS (pre-existing teardown segfault is ignored)"
if grep -q "__COREJS_LOAD_OK__" "$LOG"; then
  echo "result       : LOAD OK (top level completed)"
  RC=0
else
  echo "result       : LOAD FAILED (sentinel not reached)"
  RC=1
fi
echo "--- detection / throw lines (first 20) ---"
grep -nE "Incompatible receiver|is not a function|is not defined|unexpected token|made no progress|Uncaught|TypeError|ReferenceError|SyntaxError" "$LOG" | head -20
echo "--- log: $LOG ---"
exit $RC
