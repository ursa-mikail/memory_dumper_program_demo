// memory_dumper.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>

#ifdef __APPLE__
#include <sys/types.h>
#include <mach/mach.h>
#endif

// If PTRACE_PEEKDATA is still not defined, define it manually
#ifndef PTRACE_PEEKDATA
#define PTRACE_PEEKDATA 2
#endif

#ifndef PTRACE_ATTACH
#define PTRACE_ATTACH 16
#endif

#ifndef PTRACE_DETACH
#define PTRACE_DETACH 17
#endif

#define PATTERN_SIZE 16
#define MAX_MEMORY_REGIONS 1000

typedef struct {
    unsigned long start;
    unsigned long end;
    char permissions[8];
    char pathname[256];
} MemoryRegion;

void read_memory_regions(pid_t pid, MemoryRegion *regions, int *count) {
    char maps_path[256];
    sprintf(maps_path, "/proc/%d/maps", pid);
    
    FILE *maps_file = fopen(maps_path, "r");
    if (!maps_file) {
        perror("fopen maps");
        return;
    }
    
    *count = 0;
    char line[512];
    while (fgets(line, sizeof(line), maps_file)) {
        if (*count >= MAX_MEMORY_REGIONS) break;
        
        // Parse the memory map line
        char perms[8];
        unsigned long start, end;
        char pathname[256] = "";
        
        int parsed = sscanf(line, "%lx-%lx %7s %*s %*s %*s %255s",
                           &start, &end, perms, pathname);
        
        if (parsed >= 3) {
            regions[*count].start = start;
            regions[*count].end = end;
            strncpy(regions[*count].permissions, perms, sizeof(regions[*count].permissions) - 1);
            regions[*count].permissions[sizeof(regions[*count].permissions) - 1] = '\0';
            
            if (parsed >= 4) {
                strncpy(regions[*count].pathname, pathname, sizeof(regions[*count].pathname) - 1);
                regions[*count].pathname[sizeof(regions[*count].pathname) - 1] = '\0';
            } else {
                regions[*count].pathname[0] = '\0';
            }
            (*count)++;
        }
    }
    
    fclose(maps_file);
}

unsigned char *read_process_memory(pid_t pid, unsigned long addr, size_t length) {
    unsigned char *buffer = malloc(length);
    if (!buffer) return NULL;
    
    // Read memory word by word using ptrace
    for (size_t i = 0; i < length; i += sizeof(long)) {
        errno = 0;
        #ifdef __APPLE__
        // macOS ptrace signature: int ptrace(int request, pid_t pid, caddr_t addr, int data)
        long word = ptrace(PTRACE_PEEKDATA, pid, (caddr_t)(addr + i), 0);
        #else
        // Linux ptrace signature
        long word = ptrace(PTRACE_PEEKDATA, pid, addr + i, NULL);
        #endif
        
        if (errno != 0) {
            // If we can't read, fill with zeros and continue
            word = 0;
        }
        
        size_t bytes_to_copy = (length - i < sizeof(long)) ? length - i : sizeof(long);
        memcpy(buffer + i, &word, bytes_to_copy);
    }
    
    return buffer;
}

int search_pattern_in_region(pid_t pid, MemoryRegion *region, 
                           unsigned char *pattern, size_t pattern_size) {
    // Skip non-readable regions and very large regions
    if (!(region->permissions[0] == 'r') || (region->end - region->start) > 100 * 1024 * 1024) {
        return 0;
    }
    
    size_t region_size = region->end - region->start;
    size_t chunk_size = 4096; // Read in 4KB chunks
    int found = 0;
    
    printf("Searching region: %lx-%lx %s %s\n", 
           region->start, region->end, region->permissions, 
           region->pathname[0] ? region->pathname : "[anonymous]");
    
    for (unsigned long offset = 0; offset < region_size; offset += chunk_size) {
        size_t read_size = (offset + chunk_size <= region_size) ? chunk_size : region_size - offset;
        
        // Don't read too much at once
        if (read_size > 64 * 1024) {
            read_size = 64 * 1024;
        }
        
        unsigned char *chunk = read_process_memory(pid, region->start + offset, read_size);
        
        if (!chunk) continue;
        
        // Search for pattern in this chunk
        for (size_t i = 0; i <= read_size - pattern_size; i++) {
            if (memcmp(chunk + i, pattern, pattern_size) == 0) {
                printf("*** FOUND PATTERN at address: 0x%lx\n", region->start + offset + i);
                printf("    Memory region: %s\n", region->pathname[0] ? region->pathname : "[anonymous]");
                
                // Print surrounding memory for context
                printf("    Surrounding memory (hex): ");
                size_t context_start = (i >= 8) ? i - 8 : 0;
                size_t context_end = (i + pattern_size + 8 <= read_size) ? i + pattern_size + 8 : read_size;
                
                for (size_t j = context_start; j < context_end; j++) {
                    if (j >= i && j < i + pattern_size) {
                        printf("[%02x]", chunk[j]); // Highlight the pattern
                    } else {
                        printf(" %02x ", chunk[j]);
                    }
                }
                printf("\n");
                found++;
            }
        }
        
        free(chunk);
    }
    
    return found;
}

void dump_memory_region(pid_t pid, MemoryRegion *region, const char *filename) {
    if (!(region->permissions[0] == 'r')) {
        printf("Region not readable, skipping dump\n");
        return;
    }
    
    size_t region_size = region->end - region->start;
    
    // Don't dump very large regions
    if (region_size > 10 * 1024 * 1024) {
        printf("Region too large (%zu bytes), skipping dump\n", region_size);
        return;
    }
    
    printf("Dumping region %lx-%lx to %s (%zu bytes)\n", 
           region->start, region->end, filename, region_size);
    
    FILE *dump_file = fopen(filename, "wb");
    if (!dump_file) {
        perror("fopen dump file");
        return;
    }
    
    size_t chunk_size = 4096;
    for (unsigned long offset = 0; offset < region_size; offset += chunk_size) {
        size_t read_size = (offset + chunk_size <= region_size) ? chunk_size : region_size - offset;
        unsigned char *chunk = read_process_memory(pid, region->start + offset, read_size);
        
        if (chunk) {
            fwrite(chunk, 1, read_size, dump_file);
            free(chunk);
        }
    }
    
    fclose(dump_file);
    printf("Dump completed: %s\n", filename);
}

int main(int argc, char *argv[]) {
    #ifdef __APPLE__
    printf("WARNING: Running on macOS\n");
    printf("Note: macOS has strict security restrictions on ptrace:\n");
    printf("  - You may need to disable System Integrity Protection (SIP)\n");
    printf("  - You may need to run with sudo\n");
    printf("  - Code signing requirements may prevent attachment\n");
    printf("  - /proc filesystem is not available on macOS\n");
    printf("\nThis tool is designed for Linux. On macOS, use lldb or dtrace instead.\n\n");
    #endif
    
    if (argc < 2) {
        printf("Usage: %s <target_pid>\n", argv[0]);
        printf("Or use: %s --launch-target\n", argv[0]);
        return 1;
    }
    
    pid_t target_pid;
    
    if (strcmp(argv[1], "--launch-target") == 0) {
        // Launch the target program
        printf("Launching target program...\n");
        pid_t child_pid = fork();
        
        if (child_pid == 0) {
            // Child process - execute target program
            char *args[] = {"./target_program", NULL};
            execv(args[0], args);
            perror("execv");
            exit(1);
        } else {
            target_pid = child_pid;
            sleep(2); // Give target time to initialize and print its bytes
        }
    } else {
        target_pid = atoi(argv[1]);
    }
    
    printf("Attaching to PID: %d\n", target_pid);
    
    // Attach to target process
    #ifdef __APPLE__
    if (ptrace(PT_ATTACH, target_pid, 0, 0) == -1) {
    #else
    if (ptrace(PTRACE_ATTACH, target_pid, NULL, NULL) == -1) {
    #endif
        perror("ptrace attach");
        printf("\nIf on macOS, try:\n");
        printf("  1. Run with sudo: sudo %s %d\n", argv[0], target_pid);
        printf("  2. Disable SIP (not recommended for security)\n");
        printf("  3. Use a Linux VM or container\n");
        return 1;
    }
    
    // Wait for the target to stop
    int status;
    waitpid(target_pid, &status, 0);
    printf("Successfully attached to target process\n");
    
    // Read memory regions
    MemoryRegion regions[MAX_MEMORY_REGIONS];
    int region_count;
    read_memory_regions(target_pid, regions, &region_count);
    
    printf("Found %d memory regions\n", region_count);
    
    // Ask user for pattern or use auto-mode
    unsigned char pattern[PATTERN_SIZE];
    char choice;
    printf("Do you want to manually enter the 16-byte pattern? (y/n): ");
    scanf(" %c", &choice);
    
    if (choice == 'y' || choice == 'Y') {
        printf("Enter 16 bytes to search for (hex format, space separated): ");
        for (int i = 0; i < PATTERN_SIZE; i++) {
            unsigned int byte;
            if (scanf("%02x", &byte) != 1) {
                printf("Error reading byte %d\n", i);
                byte = 0;
            }
            pattern[i] = (unsigned char)byte;
        }
    } else {
        // Auto-mode: use a test pattern
        printf("Auto-mode: using test pattern A-Z\n");
        for (int i = 0; i < PATTERN_SIZE; i++) {
            pattern[i] = 0x41 + (i % 26); // A-Z pattern
        }
    }
    
    printf("Searching for pattern: ");
    for (int i = 0; i < PATTERN_SIZE; i++) {
        printf("%02x ", pattern[i]);
    }
    printf("\n");
    
    // Search for pattern in all memory regions
    int total_found = 0;
    for (int i = 0; i < region_count; i++) {
        total_found += search_pattern_in_region(target_pid, &regions[i], pattern, PATTERN_SIZE);
    }
    
    printf("\nTotal occurrences found: %d\n", total_found);
    
    // Optionally dump interesting memory regions
    if (total_found > 0) {
        printf("\nDumping memory regions where pattern was found...\n");
        for (int i = 0; i < region_count; i++) {
            if (strstr(regions[i].pathname, "heap") || 
                strstr(regions[i].pathname, "stack") ||
                regions[i].pathname[0] == '\0') { // anonymous mappings
                char dump_filename[256];
                sprintf(dump_filename, "dump_region_%d.bin", i);
                dump_memory_region(target_pid, &regions[i], dump_filename);
            }
        }
    }
    
    // Detach from target process
    #ifdef __APPLE__
    ptrace(PT_DETACH, target_pid, 0, 0);
    #else
    ptrace(PTRACE_DETACH, target_pid, NULL, NULL);
    #endif
    printf("Detached from target process\n");
    
    return 0;
}