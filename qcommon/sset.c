#include "qcommon.h"

qboolean Q_SSetInit(sset_t *ss,uint32_t maxSize, int32_t avgLength, int16_t memoryTag) {
    assert(ss != NULL);
    ss->tag = memoryTag;
    ss->maxSize = maxSize;
    ss->currentSize = 0;
    if (Q_STInit(&ss->table, avgLength * maxSize, avgLength, ss->tag))
    {
        ss->tokens = (int32_t *) Z_TagMalloc(maxSize * sizeof(int32_t), ss->tag);
        if (ss->tokens)
            return true;
        else {
            Q_STFree(&ss->table);
        }
    }
    return false;
}

void Q_SSetFree(sset_t *ss) {
    assert(ss != NULL);
    Q_STFree(&ss->table);
    ss->currentSize = 0;
    Z_Free(ss->tokens);
    ss->tokens = NULL;
}

qboolean Q_SSetGrow(sset_t *ss, uint32_t newSize) {
    assert(ss != NULL);
    
    if (newSize > ss->maxSize) {
        int32_t *newTokens = Z_Realloc(ss->tokens, newSize * sizeof(int32_t));
        if (newTokens) {
            ss->tokens = newTokens;
            ss->maxSize = newSize;
            return true;
        }
    }
    return false;
}

qboolean Q_SSetContains(const sset_t *ss, const char *string) {
    assert(ss != NULL && string != NULL);
    return (Q_STLookup(&ss->table, string) >= 0);
}

qboolean Q_SSetInsert(sset_t *ss, const char *string) {
    int32_t handle = -1;
    assert(ss != NULL && string != NULL);
    
    if (Q_STLookup(&ss->table, string) >= 0)
        return false;
    
    if (ss->currentSize == ss->maxSize) {
        if (!Q_SSetGrow(ss, ss->maxSize * 2))
            return false;
    }
    
    handle = Q_STRegister(&ss->table, string);
    if (handle < 0) {
        Q_STGrow(&ss->table, ss->table.size * 2);
        handle = Q_STRegister(&ss->table, string);
    }
    
    ss->tokens[ss->currentSize] = handle;
    ss->currentSize++;
    return true;
}

qboolean Q_SSetDuplicate(sset_t *source, sset_t *dest) {
    assert(source != NULL && dest != NULL);

    if (Q_STInit(&dest->table, source->table.size, source->table.size/source->currentSize, source->tag))
    {
        int32_t i;
        dest->tokens = (int32_t *)Z_TagMalloc(source->currentSize * sizeof(int32_t), source->tag);
        dest->tag = source->tag;
        dest->maxSize = source->currentSize;
        dest->currentSize = 0;
        for (i = 0; i < source->currentSize; i++) {
            if (source->tokens[i] >= 0)
                Q_SSetInsert(dest, Q_STGetString(&source->table, source->tokens[i]));
        }
        return true;
    }
    return false;
}

const char *Q_SSetGetString(sset_t *ss, int32_t index) {
    assert(ss != NULL);
    if (index >= 0 && index < ss->currentSize)
        return Q_STGetString(&ss->table, ss->tokens[index]);
    return NULL;
}

int32_t Q_SSetGetStrings(sset_t *ss, const char **strings, int32_t maxStrings) {
    assert(ss != NULL && strings != NULL);
    int i;

    for (i = 0; i < maxStrings && i < ss->currentSize; i++) {
        strings[i] = Q_STGetString(&ss->table, ss->tokens[i]);
    }
    return i;
}

const char **Q_SSetMakeStrings(sset_t *ss, int32_t *numStrings) {
    assert(ss != NULL);
    int i;
    const char **strings = Z_TagMalloc(sizeof(const char *) * ss->currentSize, ss->tag);
    
    if (!strings)
        return NULL;
    
    for (i = 0; i < ss->currentSize; i++) {
        strings[i] = Q_STGetString(&ss->table, ss->tokens[i]);
    }
    if (numStrings)
        *numStrings = ss->currentSize;
    return strings;
}

