# Makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99
GO = go
GOBUILD = $(GO) build

# Detect OS
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    # macOS
    MEMORY_DUMPER_SRC = memory_dumper_macos.c
    MEMORY_DUMPER = memory_dumper
else
    # Linux
    MEMORY_DUMPER_SRC = memory_dumper.c
    MEMORY_DUMPER = memory_dumper
endif

# Check which source files exist
TARGET_C_EXISTS := $(wildcard target_program.c)
TARGET_GO_EXISTS := $(wildcard target_program.go)
DUMPER_EXISTS := $(wildcard $(MEMORY_DUMPER_SRC))

# Build targets based on what exists
TARGETS :=
ifdef TARGET_GO_EXISTS
    TARGETS += target_program_go
endif
ifdef TARGET_C_EXISTS
    TARGETS += target_program
endif
ifdef DUMPER_EXISTS
    TARGETS += $(MEMORY_DUMPER)
endif

# Default target builds everything available
all: $(TARGETS)

# C version of target program
target_program: target_program.c
	$(CC) $(CFLAGS) -o target_program target_program.c

# Go version of target program
target_program_go: target_program.go
	$(GOBUILD) -o target_program_go target_program.go

# Memory dumper (C only)
$(MEMORY_DUMPER): $(MEMORY_DUMPER_SRC)
	$(CC) $(CFLAGS) -o $(MEMORY_DUMPER) $(MEMORY_DUMPER_SRC)

# Build only C version (if it exists)
c: 
ifdef TARGET_C_EXISTS
	$(MAKE) target_program
else
	@echo "Error: target_program.c not found"
	@exit 1
endif
ifdef DUMPER_EXISTS
	$(MAKE) $(MEMORY_DUMPER)
endif

# Build only Go version
go: target_program_go

clean:
	rm -f target_program target_program_go memory_dumper dump_*.bin

# Run with C target
run-c: target_program $(MEMORY_DUMPER)
	./target_program

# Run with Go target
run-go: target_program_go
	./target_program_go

# Launch memory dumper with C target
dump-c: all
	@echo "Run this with sudo on macOS:"
	@echo "sudo ./memory_dumper --launch-target"

# Get PID helper for Go target
run-dump-go: target_program_go
	@echo "Starting Go target program..."
	@echo "In another terminal, run:"
	@echo "  sudo ./memory_dumper \`pgrep target_program_go\`"
	@echo ""
	./target_program_go

info:
	@echo "Operating System: $(UNAME_S)"
	@echo "Memory Dumper Source: $(MEMORY_DUMPER_SRC)"
	@echo ""
	@echo "Targets available:"
	@echo "  make          - Build everything (C + Go)"
	@echo "  make c        - Build C version only"
	@echo "  make go       - Build Go version only"
	@echo "  make run-c    - Run C target program"
	@echo "  make run-go   - Run Go target program"
	@echo ""
	@echo "On macOS, you need to run with sudo:"
	@echo "  sudo ./memory_dumper --launch-target"
	@echo "  sudo ./memory_dumper <pid>"

.PHONY: all c go clean run-c run-go dump-c run-dump-go info