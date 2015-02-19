typedef struct hash_t { uint32_t v[4]; } hash_t;

hash_t Q_Hash(const char *string, uint32_t len);

int32_t Q_HashCompare(hash_t hash1, hash_t hash2);