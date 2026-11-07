SHELL := /bin/bash
APP_NAME ?= myapp
# Default build type (debug, release, test)
BUILD ?= test

# Set the directories for build and source files
TEST_DIR ?= tests
SRC_DIR ?= src
BUILD_BASE_DIR ?= build
INCLUDES := -I$(SRC_DIR)/stack -I$(SRC_DIR)/sim

# Flags for hardening and security
# https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html
CFLAGS ?= -Wall -Wextra -O2 -fPIE -MMD -MP
CFLAGS += -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough
CFLAGS += -fstack-protector-strong
CFLAGS += -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion

# Build configurations
ifeq ($(BUILD),release)
  BUILD_DIR := $(BUILD_BASE_DIR)/release
  TARGET ?= $(BUILD_DIR)/$(APP_NAME)
else ifeq ($(BUILD),debug)
  CFLAGS := -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address,undefined -MMD -MP
  LDFLAGS += -fsanitize=address,undefined
  BUILD_DIR := $(BUILD_BASE_DIR)/debug
  TARGET ?= $(BUILD_DIR)/$(APP_NAME)_d
else ifeq ($(BUILD),test)
  CFLAGS := -g -O0 -DTEST -fprofile-arcs -ftest-coverage -MMD -MP
  LDFLAGS += -fprofile-arcs -ftest-coverage
  BUILD_DIR := $(BUILD_BASE_DIR)/tests
  TEST_TARGET ?= $(BUILD_DIR)/$(APP_NAME)_t
else ifeq ($(BUILD),debug-test)
  CFLAGS := -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address,undefined -MMD -MP
  LDFLAGS += -fsanitize=address,undefined
  BUILD_DIR := $(BUILD_BASE_DIR)/debug-test
  TEST_TARGET ?= $(BUILD_DIR)/$(APP_NAME)_td
else
  $(error Invalid build type: $(BUILD))
endif
CFLAGS += $(INCLUDES)

# The kernel code is the only code that does not run on Linux, so it stays out
# of the normal builds and has its own rules further down.
SRCS := $(shell find $(SRC_DIR) -name '*.c' -not -path '$(SRC_DIR)/kernel/*')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.c.o,$(SRCS))
APP_OBJS := $(OBJS)
LIB_OBJS := $(filter-out $(BUILD_DIR)/main.c.o,$(OBJS))
DEPS := $(OBJS:.o=.d)
TEST_SRCS := $(shell find $(TEST_DIR) -name '*.c')
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%.c.o,$(TEST_SRCS))
TEST_DEPS := $(TEST_OBJS:.o=.d)

.DEFAULT_GOAL := default
default: $(if $(TEST_TARGET),$(TEST_TARGET),$(TARGET))

# Link the object files to create the final executable
$(TARGET): $(APP_OBJS)
	$(CC) $(CFLAGS) $(APP_OBJS) -o $@ $(LDFLAGS)

# Link the object files to create the test executable
$(TEST_TARGET): $(LIB_OBJS) $(TEST_OBJS)
	$(CC) $(CFLAGS) $(LIB_OBJS) $(TEST_OBJS) -o $@ $(LDFLAGS)

# Compile object files from source files
$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile object files from test source files
$(BUILD_DIR)/%.c.o: $(TEST_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ---------------------------------------------------------------------------
# The unikernel. Your stack, the kernel services and your driver in one 32 bit
# ELF file that QEMU boots directly. There is no C library in here, so the stack
# code can only include the freestanding headers.
# ---------------------------------------------------------------------------
KERNEL_DIR := $(BUILD_BASE_DIR)/kernel
KERNEL := $(KERNEL_DIR)/kernel.elf
KCFLAGS := -m32 -march=i686 -std=gnu11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector
KCFLAGS += -mno-sse -mno-mmx -O2 -g -Wall -Wextra -MMD -MP -I$(SRC_DIR)/stack -I$(SRC_DIR)/kernel
KLDFLAGS := -m32 -nostdlib -static -no-pie -T $(SRC_DIR)/kernel/kernel.ld -Wl,--build-id=none -Wl,-z,noexecstack
KERNEL_SRCS := $(wildcard $(SRC_DIR)/kernel/*.c) $(wildcard $(SRC_DIR)/stack/*.c)
KERNEL_OBJS := $(patsubst $(SRC_DIR)/%.c,$(KERNEL_DIR)/%.c.o,$(KERNEL_SRCS)) $(KERNEL_DIR)/kernel/boot.S.o

$(KERNEL): $(KERNEL_OBJS) $(SRC_DIR)/kernel/kernel.ld
	$(CC) $(KCFLAGS) $(KERNEL_OBJS) -o $@ $(KLDFLAGS)

$(KERNEL_DIR)/%.c.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(KCFLAGS) -c $< -o $@

$(KERNEL_DIR)/%.S.o: $(SRC_DIR)/%.S
	mkdir -p $(dir $@)
	$(CC) $(KCFLAGS) -c $< -o $@

# The port on your machine that forwards to port 80 of the unikernel.
PORT ?= 8080
QEMU ?= qemu-system-x86_64
QEMU_FLAGS := -kernel $(KERNEL) -nographic -no-reboot -m 64M
QEMU_FLAGS += -device isa-debug-exit,iobase=0xf4,iosize=0x04
QEMU_FLAGS += -netdev user,id=n0,hostfwd=tcp:127.0.0.1:$(PORT)-:80
QEMU_FLAGS += -device virtio-net-pci,netdev=n0,disable-modern=on
QEMU_FLAGS += -object filter-dump,id=d0,netdev=n0,file=$(BUILD_BASE_DIR)/packets.pcap

# Targets for running tests and cleaning up
.PHONY: default release debug test debug-test all clean print check report leak leak-test kernel run boot-test help
# These targets allow you to build in different modes without changing the BUILD variable
release:
	$(MAKE) BUILD=release
debug:
	$(MAKE) BUILD=debug
test:
	$(MAKE) BUILD=test
debug-test:
	$(MAKE) BUILD=debug-test

all:
	$(MAKE) BUILD=debug
	$(MAKE) BUILD=release
	$(MAKE) BUILD=test
	$(MAKE) BUILD=debug-test
	$(MAKE) kernel
	@echo "Builds completed. You can run the application with: ./build/release/$(APP_NAME)"
	@echo "You can run the test build with: ./build/tests/$(APP_NAME)_t"
	@echo "You can boot the unikernel with: make run"

kernel: $(KERNEL)

# Ctrl-a x leaves QEMU.
run: $(KERNEL)
	$(QEMU) $(QEMU_FLAGS)

boot-test: $(KERNEL)
	PORT=$(PORT) QEMU=$(QEMU) ./scripts/boot-test.sh

leak:
	@if [[ -e ./build/debug/$(APP_NAME)_d ]]; then \
		ASAN_OPTIONS="detect_leaks=1" ./build/debug/$(APP_NAME)_d; \
	else \
		echo "Build the debug target first by running 'make debug'."; \
		exit 1; \
	fi

leak-test:
	@if [[ -e ./build/debug-test/$(APP_NAME)_td ]]; then \
		ASAN_OPTIONS="detect_leaks=1" ./build/debug-test/$(APP_NAME)_td; \
	else \
		echo "Build the debug target first by running 'make debug-test'."; \
		exit 1; \
	fi

check:
	@if [[ -e ./build/tests/$(APP_NAME)_t ]]; then \
		./build/tests/$(APP_NAME)_t; \
	else \
		echo "Build the debug target first by running 'make test'."; \
		exit 1; \
	fi

report:
	@if [[ -e ./build/tests/$(APP_NAME)_t ]]; then \
		./build/tests/$(APP_NAME)_t; \
	else \
		echo "Build the debug target first by running 'make test'."; \
		exit 1; \
	fi
	mkdir -p ./build/report/html
	mkdir -p ./build/report/txt
	gcovr -r . build/tests --html --html-details --object-directory build/tests --exclude 'src/sim/.*' --exclude '.*main\.c$$' --exclude 'tests/.*' -o ./build/report/html/coverage_report.html
	gcovr -r . build/tests --txt                 --object-directory build/tests --exclude 'src/sim/.*' --exclude '.*main\.c$$' --exclude 'tests/.*'

help:
	@echo "Usage: make [target]"
	@echo "Available targets:"
	@echo "  all         - Builds debug, release, test and kernel targets"
	@echo "  release     - Build the simulator in release mode"
	@echo "  debug       - Build the simulator in debug mode"
	@echo "  test        - Build the unit tests"
	@echo "  check       - Run tests and check results"
	@echo "  report      - Generate HTML and TXT coverage report after running tests"
	@echo "  leak        - Run the simulator under Address Sanitizer"
	@echo "  leak-test   - Run the unit tests under Address Sanitizer"
	@echo "  kernel      - Build the unikernel"
	@echo "  run         - Boot the unikernel in QEMU, Ctrl-a x to leave"
	@echo "  boot-test   - Boot the unikernel and test it with curl"
	@echo "  clean       - Remove build artifacts"
	@echo "  print       - Print build variables for MakeFile debugging"
	@echo "  help        - Show this help message"

clean:
	$(RM) -rf $(BUILD_BASE_DIR) submission-report.md

print:
	@echo "Building in $(BUILD) mode, in $(BUILD_DIR)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "App target: $(TARGET)"
	@echo "Test target: $(TEST_TARGET)"
	@echo "Source files: $(SRCS)"
	@echo "Test files: $(TEST_SRCS)"
	@echo "Kernel files: $(KERNEL_SRCS)"

# Include the dependency files if they exist
# This allows for automatic dependency tracking
-include $(DEPS) $(TEST_DEPS) $(KERNEL_OBJS:.o=.d)
