// target_program.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

void generate_random_bytes(unsigned char *buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        buffer[i] = rand() % 256;
    }
}

int main() {
    // Initialize random seed with time and PID for better randomness
    srand(time(NULL) ^ getpid());
    
    // Generate and display our target 16 random bytes
    unsigned char target_bytes[16];
    generate_random_bytes(target_bytes, sizeof(target_bytes));
    
    printf("=== TARGET PROGRAM ===\n");
    printf("PID: %d\n", getpid());
    printf("Target 16 bytes (hex): ");
    for (int i = 0; i < 16; i++) {
        printf("%02x ", target_bytes[i]);
    }
    printf("\n");
    
    printf("Target 16 bytes (decimal): ");
    for (int i = 0; i < 16; i++) {
        printf("%d ", target_bytes[i]);
    }
    printf("\n");
    
    // Store the bytes in various memory locations to make them easier to find
    unsigned char *heap_copy = malloc(16);
    memcpy(heap_copy, target_bytes, 16);
    
    static unsigned char static_copy[16];
    memcpy(static_copy, target_bytes, 16);
    
    unsigned char stack_copy[16];
    memcpy(stack_copy, target_bytes, 16);
    
    printf("Bytes stored in:\n");
    printf("  Heap:   %p\n", (void*)heap_copy);
    printf("  Stack:  %p\n", (void*)stack_copy);
    printf("  Static: %p\n", (void*)static_copy);
    printf("\nProgram waiting for memory dump...\n");
    printf("Press Enter to exit or let memory dumper attach...\n");
    
    // Keep the program alive
    getchar();
    
    free(heap_copy);
    return 0;
}