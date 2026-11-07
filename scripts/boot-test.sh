#!/usr/bin/env bash
# Boot the unikernel in QEMU and talk to it with curl from outside. This is the
# test that proves the whole stack works, since curl has no idea it is not talking
# to a real machine.
#
#   PORT=8080 ./scripts/boot-test.sh
set -u

PORT=${PORT:-8080}
QEMU=${QEMU:-qemu-system-x86_64}
BYTES=${BYTES:-5000000}
BUILD=${BUILD_BASE_DIR:-build}
KERNEL=$BUILD/kernel/kernel.elf
LOG=$BUILD/boot-test.log
PCAP=$BUILD/packets.pcap
WORK=$BUILD/boot-test
FAILED=0

if [[ ! -e $KERNEL ]]; then
  echo "Build the kernel first by running 'make kernel'."
  exit 1
fi
mkdir -p "$WORK"
rm -f "$LOG" "$PCAP"

"$QEMU" -kernel "$KERNEL" -display none -serial "file:$LOG" -no-reboot -m 64M \
  -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
  -netdev "user,id=n0,hostfwd=tcp:127.0.0.1:$PORT-:80" \
  -device virtio-net-pci,netdev=n0,disable-modern=on \
  -object "filter-dump,id=d0,netdev=n0,file=$PCAP" &
QEMU_PID=$!
trap 'kill "$QEMU_PID" 2>/dev/null; wait "$QEMU_PID" 2>/dev/null' EXIT

check() {
  if [[ $2 == "$3" ]]; then
    echo "ok: $1"
  else
    echo "FAILED: $1"
    echo "  wanted: $3"
    echo "  got:    $2"
    FAILED=1
  fi
}

# It takes a few seconds, because the boot checks wait on the network.
for _ in $(seq 1 120); do
  grep -q "http listening" "$LOG" 2>/dev/null && break
  if ! kill -0 "$QEMU_PID" 2>/dev/null; then
    echo "FAILED: QEMU quit before the stack came up"
    cat "$LOG"
    exit 1
  fi
  sleep 0.5
done
if ! grep -q "http listening" "$LOG"; then
  echo "FAILED: the kernel never started listening"
  cat "$LOG"
  exit 1
fi

# QEMU's firmware puts escape codes on the serial port first, so only show ours.
grep -a -E "hello|stack up|ping|dns|http" "$LOG"

URL=http://127.0.0.1:$PORT
CURL=(curl --silent --show-error --max-time 120 --noproxy '*')

page=$("${CURL[@]}" "$URL/")
check "GET / returns the page" "$(grep -c "Hello from the unikernel" <<<"$page")" "1"

status=$("${CURL[@]}" -o /dev/null -w '%{http_code}' "$URL/nope")
check "a missing page is a 404" "$status" "404"

status=$("${CURL[@]}" -o /dev/null -w '%{http_code}' -X DELETE "$URL/")
check "a bad method is a 405" "$status" "405"

# The whole file has to match byte for byte, so build it here and compare.
python3 - "$BYTES" "$WORK/expected.bin" <<'EOF'
import sys
n, path = int(sys.argv[1]), sys.argv[2]
block = bytes(((i * 31) + (i >> 8)) & 0xFF for i in range(n))
open(path, "wb").write(block)
EOF
"${CURL[@]}" -o "$WORK/got.bin" "$URL/bytes/$BYTES"
if cmp -s "$WORK/expected.bin" "$WORK/got.bin"; then
  echo "ok: GET /bytes/$BYTES is identical"
else
  echo "FAILED: GET /bytes/$BYTES is not identical"
  FAILED=1
fi

# Same again the other way, which is what tests the receive window.
want=$(python3 - "$WORK/expected.bin" <<'EOF'
import sys
h = 2166136261
data = open(sys.argv[1], "rb").read()
for b in data:
    h = ((h ^ b) * 16777619) & 0xFFFFFFFF
print(f"length={len(data)} fnv1a={h}")
EOF
)
got=$("${CURL[@]}" -H 'Expect:' -H 'Content-Type: application/octet-stream' --data-binary "@$WORK/expected.bin" "$URL/count")
check "POST /count hashes the upload" "$got" "$want"

# Many requests in a row have to get past 8 connection slots full of TIME_WAIT.
for i in $(seq 1 30); do
  status=$("${CURL[@]}" -o /dev/null -w '%{http_code}' "$URL/bytes/1000")
  [[ $status == 200 ]] || break
done
check "30 requests in a row" "$status" "200"

if [[ $FAILED -ne 0 ]]; then
  echo "boot test FAILED, and the packets are in $PCAP"
  exit 1
fi
echo "boot test passed, and the packets are in $PCAP"
