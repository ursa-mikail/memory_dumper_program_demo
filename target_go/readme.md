# 👻 Memory Dumper

Search for byte patterns in process memory. Works on Linux (ptrace) and macOS (Mach VM API).

## Quick Start

```bash
# Compile
go build target_program.go


# macOS - Auto-launch target with sudo
sudo ./memory_dumper --launch-target

# OR manually in two terminals:
# Terminal 1:
./target_program

# Terminal 2:
sudo ./memory_dumper <PID>    # macOS needs sudo
./memory_dumper <PID>          # Linux
```

### Build
```
# Build everything (C + Go)
make

# Build only C version
make c

# Build only Go version
make go

# Clean all binaries
make clean

# Show help
make info
```

### Run:
```
# Run Go target program  
make run-go
```

## Usage

1. **Target program** generates and displays 16 random bytes
2. **Memory dumper** searches for those bytes across all memory regions
3. Enter the hex bytes when prompted (or use auto-mode)
4. View results showing exact memory addresses

## Example

```
=== TARGET PROGRAM ===
PID: 12345
Target 16 bytes (hex): 4a 7f 32 89 d1 55 aa ff 91 2c 76 b8 03 ee 4d 61
```

Then search:
```bash
sudo ./memory_dumper 12345
# Enter: 4a 7f 32 89 d1 55 aa ff 91 2c 76 b8 03 ee 4d 61
```

## Platform Notes

- **macOS**: Requires `sudo` (uses Mach VM API)
- **Linux**: May need `sudo` or ptrace permissions (uses ptrace API)
- Auto-detects OS and compiles appropriate version

## Features

- Enumerates all readable memory regions
- Searches for 16-byte patterns
- Shows surrounding memory context
- Can dump regions to binary files
- Skips large regions (>100MB) for performance


```
Features:
Memory Region Scanning: Reads /proc/pid/maps to find all memory regions

Pattern Hunting: Searches for specific 16-byte patterns across all readable memory

Memory Dumping: Can dump specific memory regions to files

Context Display: Shows surrounding memory when patterns are found

Multiple Storage: Target stores bytes in heap, stack, and static memory
```

```
# Compile everything:
make

./memory_dumper --launch-target

./memory_dumper <PID>

```

# Manual Approach (Recommended)
Step 1: Run the target program
```bash
# Terminal 1 - Run the target program
./target_program
```

You'll see output like:

```text
=== TARGET PROGRAM ===
PID: 80136
Target 16 bytes (hex): 4a 7f 32 89 d1 55 aa ff 91 2c 76 b8 03 ee 4d 61
Target 16 bytes (decimal): 74 127 50 137 209 85 170 255 145 44 118 184 3 238 77 97
Bytes stored in:
  Heap:   0x55aabbccddee
  Stack:  0x7ffd44556677
  Static: 0x55aabbccddff

Program waiting for memory dump...
Press Enter to exit or let memory dumper attach...
```
Note the PID (80136 in this example) and the 16 bytes in hex format.

Step 2: Open another terminal and run the memory dumper
```bash
# Terminal 2 - Run memory dumper with the PID

sudo ./memory_dumper 80136

Step 3: Enter the pattern when prompted
The memory dumper will ask:

text
Do you want to manually enter the 16-byte pattern? (y/n): y
Enter 16 bytes to search for (hex format, space separated): 
Enter the exact 16 bytes from the target program:

text
4a 7f 32 89 d1 55 aa ff 91 2c 76 b8 03 ee 4d 61
```

```
% sudo ./memory_dumper 80233
Password:
=== macOS Memory Dumper (Mach VM API) ===

Target PID: 80233
Successfully got task port for process

=== Memory Regions ===
Region   0: 0x0000000100708000 - 0x000000010070c000 [r-x] size: 16 KB
Region   1: 0x000000010070c000 - 0x0000000100710000 [r--] size: 16 KB
Region   2: 0x0000000100710000 - 0x0000000100714000 [rw-] size: 16 KB
Region   3: 0x0000000100714000 - 0x0000000100718000 [r--] size: 16 KB
Region   4: 0x0000000100718000 - 0x0000000100720000 [rw-] size: 32 KB
Region   5: 0x0000000100720000 - 0x0000000100728000 [r--] size: 32 KB
Region   6: 0x0000000100728000 - 0x000000010072c000 [r--] size: 16 KB
Region   7: 0x000000010072c000 - 0x0000000100730000 [rw-] size: 16 KB
Region   8: 0x0000000100730000 - 0x0000000100734000 [---] size: 16 KB
Region   9: 0x0000000100734000 - 0x000000010073c000 [rw-] size: 32 KB
Region  10: 0x000000010073c000 - 0x0000000100740000 [---] size: 16 KB
Region  11: 0x0000000100740000 - 0x0000000100744000 [---] size: 16 KB
Region  12: 0x0000000100744000 - 0x000000010074c000 [rw-] size: 32 KB
Region  13: 0x000000010074c000 - 0x0000000100750000 [---] size: 16 KB
Region  14: 0x0000000100750000 - 0x0000000100754000 [---] size: 16 KB
Region  15: 0x0000000100754000 - 0x000000010075c000 [rw-] size: 32 KB
Region  16: 0x000000010075c000 - 0x0000000100760000 [---] size: 16 KB
Region  17: 0x0000000100760000 - 0x0000000100764000 [r--] size: 16 KB
Region  18: 0x0000000100764000 - 0x0000000100768000 [r--] size: 16 KB
Region  19: 0x0000000100768000 - 0x000000010076c000 [rw-] size: 16 KB
Region  20: 0x00000001007f0000 - 0x0000000100850000 [---] size: 384 KB
Region  21: 0x0000000156e00000 - 0x0000000156f00000 [rw-] size: 1024 KB
Region  22: 0x0000000156f00000 - 0x0000000157000000 [rw-] size: 1024 KB
Region  23: 0x0000000157000000 - 0x0000000157800000 [rw-] size: 8192 KB
Region  24: 0x0000000157800000 - 0x0000000157900000 [rw-] size: 1024 KB
Region  25: 0x0000000158000000 - 0x0000000158800000 [rw-] size: 8192 KB
Region  26: 0x000000016b6f8000 - 0x000000016eefc000 [---] size: 57360 KB
Region  27: 0x000000016eefc000 - 0x000000016f6f8000 [rw-] size: 8176 KB
Region  28: 0x0000000180000000 - 0x00000001fe000000 [r--] size: 2064384 KB
Region  29: 0x00000001fe000000 - 0x00000001ffda0000 [r--] size: 30336 KB
Region  30: 0x00000001ffda0000 - 0x00000001ffdc4000 [rw-] size: 144 KB
Region  31: 0x00000001ffdc4000 - 0x0000000200000000 [rw-] size: 2288 KB
Region  32: 0x0000000200000000 - 0x0000000202000000 [r--] size: 32768 KB
Region  33: 0x0000000202000000 - 0x000000020213c000 [r--] size: 1264 KB
Region  34: 0x000000020213c000 - 0x0000000202160000 [r--] size: 144 KB
Region  35: 0x0000000202160000 - 0x0000000203360000 [rw-] size: 18432 KB
Region  36: 0x0000000203360000 - 0x000000020b024000 [r--] size: 127760 KB
Region  37: 0x000000020b024000 - 0x000000020c000000 [r--] size: 16240 KB
Region  38: 0x000000020c000000 - 0x0000000294000000 [r--] size: 2228224 KB
Region  39: 0x0000000294000000 - 0x0000000295840000 [r--] size: 24832 KB
Region  40: 0x0000000295840000 - 0x00000002965d0000 [rw-] size: 13888 KB
Region  41: 0x00000002965d0000 - 0x000000029aba4000 [r--] size: 71504 KB
Region  42: 0x000000029aba4000 - 0x000000029c000000 [r--] size: 20848 KB
Region  43: 0x000000029c000000 - 0x0000000300000000 [r--] size: 1638400 KB
Region  44: 0x0000000fc0000000 - 0x0000001000000000 [---] size: 1048576 KB
Region  45: 0x0000001000000000 - 0x0000007000000000 [---] size: 402653184 KB
Region  46: 0x0000600000000000 - 0x0000600020000000 [rw-] size: 524288 KB

Found 47 memory regions

Do you want to manually enter the 16-byte pattern? (y/n): y
Enter 16 bytes to search for (hex format, space separated): 5a 2a 65 7d ea 64 a1 49 3c 80 93 de a3 01 35 db

Searching for pattern: 5a 2a 65 7d ea 64 a1 49 3c 80 93 de a3 01 35 db 

Searching region: 0x100708000 - 0x10070c000 [r-x] (16384 bytes)

Searching region: 0x10070c000 - 0x100710000 [r--] (16384 bytes)

Searching region: 0x100710000 - 0x100714000 [rw-] (16384 bytes)
*** FOUND PATTERN at address: 0x100710000
    Surrounding memory (hex): [5a][2a][65][7d][ea][64][a1][49][3c][80][93][de][a3][01][35][db] 00  00  00  00  00  00  00  00 

Searching region: 0x100714000 - 0x100718000 [r--] (16384 bytes)

Searching region: 0x100718000 - 0x100720000 [rw-] (32768 bytes)

Searching region: 0x100720000 - 0x100728000 [r--] (32768 bytes)

Searching region: 0x100728000 - 0x10072c000 [r--] (16384 bytes)

Searching region: 0x10072c000 - 0x100730000 [rw-] (16384 bytes)

Searching region: 0x100734000 - 0x10073c000 [rw-] (32768 bytes)

Searching region: 0x100744000 - 0x10074c000 [rw-] (32768 bytes)

Searching region: 0x100754000 - 0x10075c000 [rw-] (32768 bytes)

Searching region: 0x100760000 - 0x100764000 [r--] (16384 bytes)

Searching region: 0x100764000 - 0x100768000 [r--] (16384 bytes)

Searching region: 0x100768000 - 0x10076c000 [rw-] (16384 bytes)

Searching region: 0x156e00000 - 0x156f00000 [rw-] (1048576 bytes)

Searching region: 0x156f00000 - 0x157000000 [rw-] (1048576 bytes)

Searching region: 0x157000000 - 0x157800000 [rw-] (8388608 bytes)

Searching region: 0x157800000 - 0x157900000 [rw-] (1048576 bytes)

Searching region: 0x158000000 - 0x158800000 [rw-] (8388608 bytes)

Searching region: 0x16eefc000 - 0x16f6f8000 [rw-] (8372224 bytes)
*** FOUND PATTERN at address: 0x16f6f6e10
    Surrounding memory (hex):  10  00  00  00  00  00  00  00 [5a][2a][65][7d][ea][64][a1][49][3c][80][93][de][a3][01][35][db] 1d  d7  09  00  ff  ff  ff  0f 
*** FOUND PATTERN at address: 0x16f6f6e28
    Surrounding memory (hex):  1d  d7  09  00  ff  ff  ff  0f [5a][2a][65][7d][ea][64][a1][49][3c][80][93][de][a3][01][35][db] dd  00  37  1a  21  ca  67  43 

Searching region: 0x1fe000000 - 0x1ffda0000 [r--] (31064064 bytes)

Searching region: 0x1ffda0000 - 0x1ffdc4000 [rw-] (147456 bytes)

Searching region: 0x1ffdc4000 - 0x200000000 [rw-] (2342912 bytes)

Searching region: 0x200000000 - 0x202000000 [r--] (33554432 bytes)

Searching region: 0x202000000 - 0x20213c000 [r--] (1294336 bytes)

Searching region: 0x20213c000 - 0x202160000 [r--] (147456 bytes)

Searching region: 0x202160000 - 0x203360000 [rw-] (18874368 bytes)

Searching region: 0x20b024000 - 0x20c000000 [r--] (16629760 bytes)

Searching region: 0x294000000 - 0x295840000 [r--] (25427968 bytes)

Searching region: 0x295840000 - 0x2965d0000 [rw-] (14221312 bytes)

Searching region: 0x2965d0000 - 0x29aba4000 [r--] (73220096 bytes)

Searching region: 0x29aba4000 - 0x29c000000 [r--] (21348352 bytes)

=== SEARCH COMPLETE ===
Total occurrences found: 3

Dump memory regions to files? (y/n): y
Dumping writable memory regions...
Dumping region 0x100710000-0x100714000 to dump_region_2.bin (16384 bytes)
Dump completed: dump_region_2.bin
Dumping region 0x100718000-0x100720000 to dump_region_4.bin (32768 bytes)
Dump completed: dump_region_4.bin
Dumping region 0x10072c000-0x100730000 to dump_region_7.bin (16384 bytes)
Dump completed: dump_region_7.bin
Dumping region 0x100734000-0x10073c000 to dump_region_9.bin (32768 bytes)
Dump completed: dump_region_9.bin
Done!
```

