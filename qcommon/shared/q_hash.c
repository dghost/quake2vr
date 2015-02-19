
#include "q_shared.h"
#include "q_hash.h"
#include "../murmur3/murmur3.h"

#if (defined _M_IX86 || defined __i386__)
#define HASHFUNC MurmurHash3_x86_128
#elif defined(_M_X64) || defined(__x86_64__)
#define HASHFUNC MurmurHash3_x64_128
#else
#error Unknown Architecture!
#endif


#ifdef __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

static const uint32_t seed = 42;

hash_t Q_Hash(const char *string, uint32_t len) {
    hash_t result;
    HASHFUNC(string, len, seed, result.v);
    return result;
}

FORCE_INLINE int32_t Q_HashCompare(hash_t hash1, hash_t hash2) {
    return memcmp(hash1.v, hash2.v, sizeof(uint32_t) * 4);
}