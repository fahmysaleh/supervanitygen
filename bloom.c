#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h> // For CHAR_BIT
#include <stdint.h> // For uint64_t

#include "bloom.h"

#define SETBIT(bf, n) (bf[((n) / CHAR_BIT)] |= (1 << ((n) % CHAR_BIT)))
#define GETBIT(bf, n) (bf[((n) / CHAR_BIT)] & (1 << ((n) % CHAR_BIT)))

// Hash function 1: djb2
static uint64_t djb2_hash(const void *buffer, int len)
{
  uint64_t hash = 5381;
  const unsigned char *str = (const unsigned char *)buffer;
  int c;
  int i = 0;

  while (i < len) {
    c = str[i++];
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }

  return hash;
}

// Hash function 2: sdbm
static uint64_t sdbm_hash(const void *buffer, int len)
{
  uint64_t hash = 0;
  const unsigned char *str = (const unsigned char *)buffer;
  int c;
  int i = 0;

  while (i < len) {
    c = str[i++];
    hash = c + (hash << 6) + (hash << 16) - hash;
  }

  return hash;
}

// Generate the nth hash value (0 <= n < k)
static uint64_t nth_hash(int n, uint64_t hash_a, uint64_t hash_b, uint64_t filter_bits)
{
  // Simple linear combination
  return (hash_a + (uint64_t)n * hash_b) % filter_bits;
}

int bloom_init(struct bloom *bloom, int entries, double error)
{
  if (!bloom || entries <= 0 || error <= 0.0 || error >= 1.0) {
    // Handle edge case of 0 entries or invalid error rate
    // One might choose to initialize to a minimal safe state or return error
    if (bloom && entries == 0) { // Allow init with 0 entries to a minimal filter
        bloom->entries = 0;
        bloom->error = error > 0.0 ? error : 0.001; // Default error if not specified
        bloom->bits = 8; // Minimal bits
        bloom->bytes = 1;
        bloom->hashes = 1; // Minimal hashes
    } else if (bloom) {
        bloom->bf = NULL; // Ensure bf is NULL on error if bloom itself is not
        return -1; // Invalid parameters
    } else {
        return -1; // Bloom itself is NULL
    }
  } else {
    bloom->entries = entries;
    bloom->error = error;

    // Calculate optimal filter size (m bits) and number of hash functions (k)
    // m = - (n * ln(p)) / (ln(2)^2)
    // k = (m / n) * ln(2)
    double num = -1.0 * entries * log(error);
    double den = log(2.0) * log(2.0);
    bloom->bits = (long long)ceil(num / den);

    if (bloom->bits == 0) bloom->bits = 1; // Ensure at least 1 bit

    bloom->hashes = (int)ceil((((double)bloom->bits / entries)) * log(2.0));
    
    if (bloom->hashes == 0) bloom->hashes = 1; // Ensure at least 1 hash function
  }


  bloom->bytes = (bloom->bits + CHAR_BIT - 1) / CHAR_BIT; // Ceiling division

  bloom->bf = (unsigned char *)calloc(bloom->bytes, sizeof(unsigned char));
  if (bloom->bf == NULL) {
    return -1; // Memory allocation failed
  }

  return 0;
}

int bloom_add(struct bloom *bloom, const void *buffer, int len)
{
  if (!bloom || !bloom->bf || !buffer || len <= 0) {
    return -1;
  }

  uint64_t hash_a = djb2_hash(buffer, len);
  uint64_t hash_b = sdbm_hash(buffer, len);
  int i;

  for (i = 0; i < bloom->hashes; i++) {
    SETBIT(bloom->bf, nth_hash(i, hash_a, hash_b, bloom->bits));
  }

  return 0;
}

int bloom_check(struct bloom *bloom, const void *buffer, int len)
{
  if (!bloom || !bloom->bf || !buffer || len <= 0) {
    return -1; // Or some other indicator of invalid args / uninitialized filter
  }

  uint64_t hash_a = djb2_hash(buffer, len);
  uint64_t hash_b = sdbm_hash(buffer, len);
  int i;

  for (i = 0; i < bloom->hashes; i++) {
    if (!GETBIT(bloom->bf, nth_hash(i, hash_a, hash_b, bloom->bits))) {
      return 0; // Definitely not in set
    }
  }

  return 1; // Possibly in set
}

void bloom_free(struct bloom *bloom)
{
  if (bloom) {
    if (bloom->bf) {
      free(bloom->bf);
    }
    bloom->bf = NULL;
    bloom->entries = 0;
    bloom->error = 0.0;
    bloom->bits = 0;
    bloom->bytes = 0;
    bloom->hashes = 0;
  }
}

void bloom_print(struct bloom *bloom)
{
  if (!bloom) return;

  printf("Bloom Filter Parameters:
");
  printf("  Entries (n):         %d
", bloom->entries);
  printf("  Desired Error (p):   %f
", bloom->error);
  printf("  Bits (m):            %lld
", bloom->bits);
  printf("  Bytes:               %lld
", bloom->bytes);
  printf("  Hash Functions (k):  %d
", bloom->hashes);

  // Calculate theoretical actual false positive rate with integer k and m
  // P_false_positive = (1 - e^(-k*n/m))^k
  if (bloom->entries > 0 && bloom->bits > 0) {
    double k_actual = (double)bloom->hashes;
    double n_actual = (double)bloom->entries;
    double m_actual = (double)bloom->bits;
    double exponent = -k_actual * n_actual / m_actual;
    double p_effective = pow(1.0 - exp(exponent), k_actual);
    printf("  Effective Error:     ~%f
", p_effective);
  }
}

#endif // BLOOM_C Not standard, but useful for self-reference if file was named bloom.c. Remove if confusing.
// Standard practice is just to end the file.
