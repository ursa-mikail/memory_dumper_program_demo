// memory_dumper_macos.c - Native macOS implementation using Mach VM API
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>

#define PATTERN_SIZE 16

typedef struct {
    mach_vm_address_t start;
    mach_vm_size_t size;
    vm_region_basic_info_data_64_t info;
    char permissions[8];
} MemoryRegion;

const char* protection_to_string(vm_prot_t prot, char *buf) {
    buf[0] = (prot & VM_PROT_READ) ? 'r' : '-';
    buf[1] = (prot & VM_PROT_WRITE) ? 'w' : '-';
    buf[2] = (prot & VM_PROT_EXECUTE) ? 'x' : '-';
    buf[3] = '\0';
    return buf;
}

int read_memory_regions(task_t task, MemoryRegion **regions_out, int *count) {
    MemoryRegion *regions = malloc(sizeof(MemoryRegion) * 1000);
    if (!regions) return -1;
    
    *count = 0;
    mach_vm_address_t address = 0;
    mach_vm_size_t size = 0;
    
    printf("\n=== Memory Regions ===\n");
    
    while (1) {
        mach_msg_type_number_t info_count = VM_REGION_BASIC_INFO_COUNT_64;
        vm_region_basic_info_data_64_t info;
        mach_port_t object_name;
        
        kern_return_t kr = mach_vm_region(
            task,
            &address,
            &size,
            VM_REGION_BASIC_INFO_64,
            (vm_region_info_t)&info,
            &info_count,
            &object_name
        );
        
        if (kr != KERN_SUCCESS) {
            break;
        }
        
        char perm[8];
        protection_to_string(info.protection, perm);
        
        if (*count < 1000) {
            regions[*count].start = address;
            regions[*count].size = size;
            regions[*count].info = info;
            strcpy(regions[*count].permissions, perm);
            (*count)++;
        }
        
        printf("Region %3d: 0x%016llx - 0x%016llx [%s] size: %llu KB\n",
               *count - 1, address, address + size, perm, size / 1024);
        
        address += size;
    }
    
    *regions_out = regions;
    return 0;
}

unsigned char *read_process_memory(task_t task, mach_vm_address_t addr, mach_vm_size_t length) {
    unsigned char *buffer = malloc(length);
    if (!buffer) return NULL;
    
    vm_offset_t data;
    mach_msg_type_number_t data_count;
    
    kern_return_t kr = mach_vm_read(task, addr, length, &data, &data_count);
    
    if (kr != KERN_SUCCESS) {
        free(buffer);
        return NULL;
    }
    
    memcpy(buffer, (void*)data, data_count);
    vm_deallocate(mach_task_self(), data, data_count);
    
    return buffer;
}

int search_pattern_in_region(task_t task, MemoryRegion *region,
                           unsigned char *pattern, size_t pattern_size) {
    // Skip non-readable regions and very large regions
    if (!(region->info.protection & VM_PROT_READ) || region->size > 100 * 1024 * 1024) {
        return 0;
    }
    
    size_t chunk_size = 4096;
    int found = 0;
    
    printf("\nSearching region: 0x%llx - 0x%llx [%s] (%llu bytes)\n",
           region->start, region->start + region->size,
           region->permissions, region->size);
    
    for (mach_vm_address_t offset = 0; offset < region->size; offset += chunk_size) {
        size_t read_size = (offset + chunk_size <= region->size) ? chunk_size : region->size - offset;
        
        if (read_size > 64 * 1024) {
            read_size = 64 * 1024;
        }
        
        unsigned char *chunk = read_process_memory(task, region->start + offset, read_size);
        
        if (!chunk) continue;
        
        // Search for pattern in this chunk
        for (size_t i = 0; i <= read_size - pattern_size; i++) {
            if (memcmp(chunk + i, pattern, pattern_size) == 0) {
                printf("*** FOUND PATTERN at address: 0x%llx\n", region->start + offset + i);
                
                // Print surrounding memory for context
                printf("    Surrounding memory (hex): ");
                size_t context_start = (i >= 8) ? i - 8 : 0;
                size_t context_end = (i + pattern_size + 8 <= read_size) ? i + pattern_size + 8 : read_size;
                
                for (size_t j = context_start; j < context_end; j++) {
                    if (j >= i && j < i + pattern_size) {
                        printf("[%02x]", chunk[j]);
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

void dump_memory_region(task_t task, MemoryRegion *region, const char *filename) {
    if (!(region->info.protection & VM_PROT_READ)) {
        printf("Region not readable, skipping dump\n");
        return;
    }
    
    if (region->size > 10 * 1024 * 1024) {
        printf("Region too large (%llu bytes), skipping dump\n", region->size);
        return;
    }
    
    printf("Dumping region 0x%llx-0x%llx to %s (%llu bytes)\n",
           region->start, region->start + region->size, filename, region->size);
    
    FILE *dump_file = fopen(filename, "wb");
    if (!dump_file) {
        perror("fopen dump file");
        return;
    }
    
    size_t chunk_size = 4096;
    for (mach_vm_address_t offset = 0; offset < region->size; offset += chunk_size) {
        size_t read_size = (offset + chunk_size <= region->size) ? chunk_size : region->size - offset;
        unsigned char *chunk = read_process_memory(task, region->start + offset, read_size);
        
        if (chunk) {
            fwrite(chunk, 1, read_size, dump_file);
            free(chunk);
        }
    }
    
    fclose(dump_file);
    printf("Dump completed: %s\n", filename);
}

int main(int argc, char *argv[]) {
    printf("=== macOS Memory Dumper (Mach VM API) ===\n\n");
    
    if (argc < 2) {
        printf("Usage: %s <target_pid>\n", argv[0]);
        printf("Or use: %s --launch-target\n", argv[0]);
        printf("\nNote: You may need to run with sudo on macOS\n");
        return 1;
    }
    
    pid_t target_pid;
    int launched = 0;
    
    if (strcmp(argv[1], "--launch-target") == 0) {
        printf("Launching target program...\n");
        pid_t child_pid = fork();
        
        if (child_pid == 0) {
            char *args[] = {"./target_program", NULL};
            execv(args[0], args);
            perror("execv");
            exit(1);
        } else {
            target_pid = child_pid;
            launched = 1;
            sleep(2);
            printf("Launched target with PID: %d\n", target_pid);
        }
    } else {
        target_pid = atoi(argv[1]);
    }
    
    printf("Target PID: %d\n", target_pid);
    
    // Get task port for the target process
    task_t task;
    kern_return_t kr = task_for_pid(mach_task_self(), target_pid, &task);
    
    if (kr != KERN_SUCCESS) {
        printf("\nERROR: Failed to get task port for PID %d\n", target_pid);
        printf("Mach error: %s (0x%x)\n", mach_error_string(kr), kr);
        printf("\nPossible solutions:\n");
        printf("  1. Run with sudo: sudo %s %d\n", argv[0], target_pid);
        printf("  2. If launching target: sudo %s --launch-target\n", argv[0]);
        printf("  3. Code sign the binary with proper entitlements\n");
        printf("  4. Add get-task-allow entitlement\n");
        
        if (launched) {
            kill(target_pid, SIGTERM);
        }
        return 1;
    }
    
    printf("Successfully got task port for process\n");
    
    // Read memory regions
    MemoryRegion *regions;
    int region_count;
    
    if (read_memory_regions(task, &regions, &region_count) != 0) {
        printf("Failed to read memory regions\n");
        return 1;
    }
    
    printf("\nFound %d memory regions\n", region_count);
    
    // Ask user for pattern
    unsigned char pattern[PATTERN_SIZE];
    char choice;
    printf("\nDo you want to manually enter the 16-byte pattern? (y/n): ");
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
        printf("Auto-mode: using test pattern A-Z\n");
        for (int i = 0; i < PATTERN_SIZE; i++) {
            pattern[i] = 0x41 + (i % 26);
        }
    }
    
    printf("\nSearching for pattern: ");
    for (int i = 0; i < PATTERN_SIZE; i++) {
        printf("%02x ", pattern[i]);
    }
    printf("\n");
    
    // Search for pattern
    int total_found = 0;
    for (int i = 0; i < region_count; i++) {
        total_found += search_pattern_in_region(task, &regions[i], pattern, PATTERN_SIZE);
    }
    
    printf("\n=== SEARCH COMPLETE ===\n");
    printf("Total occurrences found: %d\n", total_found);
    
    // Optionally dump memory regions
    if (total_found > 0) {
        char dump_choice;
        printf("\nDump memory regions to files? (y/n): ");
        scanf(" %c", &dump_choice);
        
        if (dump_choice == 'y' || dump_choice == 'Y') {
            printf("Dumping writable memory regions...\n");
            for (int i = 0; i < region_count && i < 10; i++) {
                if (regions[i].info.protection & VM_PROT_WRITE) {
                    char dump_filename[256];
                    sprintf(dump_filename, "dump_region_%d.bin", i);
                    dump_memory_region(task, &regions[i], dump_filename);
                }
            }
        }
    }
    
    free(regions);
    
    if (launched) {
        printf("\nTerminating launched target process...\n");
        kill(target_pid, SIGTERM);
    }
    
    printf("Done!\n");
    return 0;
}