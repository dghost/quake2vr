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

static const uint32_t seed = 42;

hash128_t Q_Hash128(const char *string, uint32_t len) {
    hash128_t result;
    HASHFUNC(string, len, seed, result.v);
    return result;
}

hash32_t Q_Hash32(const char *string, uint32_t len) {
    hash32_t result;
    MurmurHash3_x86_32(string, len, seed, &result.h);
    return result;
}

hash128_t Q_HashSanitized128(const char *string) {
    int32_t		i = 0, j=0;
    char	buffer[MAX_OSPATH];

    if (string[0] == '/' || string[0] == '\\') i++;	// skip leading slash
    
    do {
        buffer[j] = tolower(string[i]);
        //	if (letter == '.') break;
        if (buffer[j] == '\\') buffer[j] = '/';	// fix filepaths
        j++;
    }  while (string[i++] != '\0');

    return Q_Hash128(buffer, j);
}

hash32_t Q_HashSanitized32(const char *string) {
    int32_t		i = 0, j=0;
    char	buffer[MAX_OSPATH];
    
    if (string[0] == '/' || string[0] == '\\') i++;	// skip leading slash
    
    do {
        buffer[j] = tolower(string[i]);
        //	if (letter == '.') break;
        if (buffer[j] == '\\') buffer[j] = '/';	// fix filepaths
        j++;
    }  while (string[i++] != '\0');
    
    return Q_Hash32(buffer, j);
}


int32_t Q_HashEquals128(hash128_t hash1, hash128_t hash2) {
    return (hash1.v[0] ^ hash2.v[0]) ||
    (hash1.v[1] ^ hash2.v[1]) ||
    (hash1.v[2] ^ hash2.v[2]) ||
    (hash1.v[3] ^ hash2.v[3]);
}

int32_t Q_HashCompare128(hash128_t hash1, hash128_t hash2) {
    return memcmp(&hash1, &hash2, sizeof(hash128_t));
}


int32_t Q_HashEquals32(hash32_t hash1, hash32_t hash2) {
    return (hash1.h ^ hash2.h);
}

int32_t Q_HashCompare32(hash32_t hash1, hash32_t hash2) {
    return memcmp(&hash1, &hash2, sizeof(hash32_t));
}
