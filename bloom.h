#ifndef BLOOM_H
#define BLOOM_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>   // For log and pow
#include <limits.h> // For CHAR_BIT

#ifdef __cplusplus
extern "C" {
#endif

struct bloom
{
  // Fields based on my analysis
  int entries;      // How many items this filter is expected to hold (n)
  double error;     // Desired false positive rate (p)
  long long bits;   // Total bits in the filter (m)
  long long bytes;  // Total bytes (m/8)
  int hashes;       // Number of hash functions to use (k)

  unsigned char *bf; // The actual bitfield
};

// Initialize a new Bloom filter.
// entries: expected number of items.
// error: desired false positive rate (e.g., 0.001 for 0.1%).
// Returns 0 on success, -1 on error (e.g., memory allocation failure).
int bloom_init(struct bloom *bloom, int entries, double error);

// Check if an item is present in the Bloom filter.
// buffer: pointer to the data to check.
// len: length of the data in bytes.
// Returns 1 if the item is possibly present (may be a false positive).
// Returns 0 if the item is definitely not present.
int bloom_check(struct bloom *bloom, const void *buffer, int len);

// Add an item to the Bloom filter.
// buffer: pointer to the data to add.
// len: length of the data in bytes.
// Returns 0 on success, -1 on error.
int bloom_add(struct bloom *bloom, const void *buffer, int len);

// Free the memory used by the Bloom filter.
void bloom_free(struct bloom *bloom);

// Print information about the Bloom filter (for debugging).
void bloom_print(struct bloom *bloom);

#ifdef __cplusplus
}
#endif

#endif // BLOOM_H
