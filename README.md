# G - Build the Whole Network Stack

- Name: Your name here
- Email: Your email here
- Class: CS525-001

A TCP/IP stack in C, from the virtio-net driver up to an HTTP server, that runs as
a unikernel in QEMU. The assignment pages on the course site have the details for
each milestone.

## Setup

GitHub Codespaces has everything once the container finishes building. See
[docs/onyx.md](docs/onyx.md) for Onyx.

```bash
make all        # debug, release, test and debug-test builds, plus the kernel
make test       # build the Unity tests
make check      # run them, after make test or make all
make report     # coverage report
make leak-test  # run the unit tests under Address Sanitizer
make run        # boot the kernel in QEMU, Ctrl-a x to leave
make boot-test  # boot the kernel and test it with curl
```

The kernel only builds on 64 bit x86, so a Mac or a Codespace on another
architecture cannot run `make kernel`. Everything else builds anywhere Linux runs.

`tests/main-test.c` has a `MILESTONE` number. Raise it when you start a new milestone,
so the smoke test for that milestone runs too.

## What is yours

| Milestone | Files |
| --------- | ----- |
| G0 | `src/stack/checksum.c` |
| G1 | `src/stack/tcp.c`, the structs in `src/stack/tcp.h` |
| G2 | `src/stack/ip.c`, `src/stack/udp.c`, `src/stack/dns.c` and their structs |
| G3 | `src/stack/eth.c`, `src/stack/stack.c` and the struct in `src/stack/eth.h` |
| G4 | `src/kernel/virtio_net.c`, `src/stack/http.c` and their structs |

Anything marked "Do NOT modify this file!" is the same for everybody. Your tests go
in `tests/`, next to `tests/smoke-test.c`.

## Design

Replace this section with how your stack works and why you built it that way.
