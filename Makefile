# Makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99

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

all: target_program $(MEMORY_DUMPER)

target_program: target_program.c
	$(CC) $(CFLAGS) -o target_program target_program.c

$(MEMORY_DUMPER): $(MEMORY_DUMPER_SRC)
	$(CC) $(CFLAGS) -o $(MEMORY_DUMPER) $(MEMORY_DUMPER_SRC)

clean:
	rm -f target_program memory_dumper dump_*.bin

run: all
	./memory_dumper --launch-target

info:
	@echo "Operating System: $(UNAME_S)"
	@echo "Memory Dumper Source: $(MEMORY_DUMPER_SRC)"
	@echo ""
	@echo "On macOS, you need to run with sudo:"
	@echo "  sudo ./memory_dumper --launch-target"
	@echo "  sudo ./memory_dumper <pid>"

.PHONY: all clean run info