package main

import (
	"bufio"
	"crypto/rand"
	"fmt"
	"os"
)

// generateRandomBytes generates cryptographically random bytes
func generateRandomBytes(length int) ([]byte, error) {
	buffer := make([]byte, length)
	_, err := rand.Read(buffer)
	if err != nil {
		return nil, err
	}
	return buffer, nil
}

func main() {
	// Generate 16 random bytes
	targetBytes, err := generateRandomBytes(16)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error generating random bytes: %v\n", err)
		os.Exit(1)
	}

	fmt.Println("=== TARGET PROGRAM ===")
	fmt.Printf("PID: %d\n", os.Getpid())
	
	// Display bytes in hex format
	fmt.Print("Target 16 bytes (hex): ")
	for _, b := range targetBytes {
		fmt.Printf("%02x ", b)
	}
	fmt.Println()

	// Display bytes in decimal format
	fmt.Print("Target 16 bytes (decimal): ")
	for _, b := range targetBytes {
		fmt.Printf("%d ", b)
	}
	fmt.Println()

	// Store bytes in multiple locations to make them easier to find
	// Heap allocation
	heapCopy := make([]byte, 16)
	copy(heapCopy, targetBytes)

	// Another heap allocation
	staticCopy := make([]byte, 16)
	copy(staticCopy, targetBytes)

	// Stack allocation
	var stackCopy [16]byte
	copy(stackCopy[:], targetBytes)

	fmt.Println("Bytes stored in:")
	fmt.Printf("  Heap (slice 1): %p\n", heapCopy)
	fmt.Printf("  Heap (slice 2): %p\n", staticCopy)
	fmt.Printf("  Stack (array):  %p\n", &stackCopy)
	fmt.Println("\nProgram waiting for memory dump...")
	fmt.Println("Press Enter to exit or let memory dumper attach...")

	// Keep the program alive and prevent garbage collection
	reader := bufio.NewReader(os.Stdin)
	reader.ReadString('\n')

	// Use the variables to prevent compiler optimization
	_ = heapCopy
	_ = staticCopy
	_ = stackCopy
}
