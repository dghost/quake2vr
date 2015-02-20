#include "shared/q_shared.h"
#include "murmur3/murmur3.h"
#include <ctype.h>

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


hash_t Q_HashSanitize(const char *string) {
    int32_t		i = 0, j=0;
    hash_t	hash;
    char	buffer[MAX_OSPATH];

    if (string[0] == '/' || string[0] == '\\') i++;	// skip leading slash
    
    do {
        buffer[j] = tolower(string[i]);
        //	if (letter == '.') break;
        if (buffer[j] == '\\') buffer[j] = '/';	// fix filepaths
        j++;
    }  while (string[i++] != '\0');

    
    HASHFUNC(buffer, j, seed, hash.v);

    return hash;
}

int32_t Q_HashEquals(hash_t hash1, hash_t hash2) {
    return (hash1.v[0] ^ hash2.v[0]) ||
        (hash1.v[1] ^ hash2.v[1]) ||
        (hash1.v[2] ^ hash2.v[2]) ||
        (hash1.v[3] ^ hash2.v[3]);
}

int32_t Q_HashCompare(hash_t hash1, hash_t hash2) {
    return memcmp(&hash1, &hash2, sizeof(hash_t));
}
