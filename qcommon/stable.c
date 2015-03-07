#include "qcommon.h"
#include "nflibs/nf_string_table.c"

qboolean Q_STInit(stable_t *st, int32_t baseSize, int32_t avgLength) {
    stable_t new = {0, 0};
    new.st = Z_TagMalloc(baseSize, TAG_SYSTEM);
    if (!new.st)
        return false;
    new.size = baseSize;
    nfst_init((struct nfst_StringTable *)new.st, baseSize, avgLength);
    *st = new;
    return true;
}

int Q_STRegister(stable_t *st, const char *string) {
    int result = nfst_to_symbol((struct nfst_StringTable *)st->st, string);
    if (result == NFST_STRING_TABLE_FULL) {
        void *new = NULL;
        int newsize = st->size * 2;
        if ((new = Z_Realloc(st->st, newsize)) != NULL) {
            st->st = new;
            st->size = newsize;
            nfst_grow((struct nfst_StringTable *)st->st, newsize);
            result = nfst_to_symbol((struct nfst_StringTable *)st->st, string);
        }
    
    }
    return result;
}

int Q_STLookup(stable_t st, const char *string) {
    return nfst_to_symbol_const((struct nfst_StringTable *)st.st, string);
};

const char *Q_STGetString(stable_t st, int token) {
    return nfst_to_string((struct nfst_StringTable *)st.st, token);
}

void Q_STPack(stable_t *st) {
    int size = nfst_pack((struct nfst_StringTable *)st->st);
    if (size > MIN_SIZE && size <= st->size/2) {
        void *new = NULL;
        if ((new = Z_Realloc(st->st, size)) != NULL) {
            st->st = new;
            st->size = size;
        }
    }
}

void Q_STFree(stable_t *st) {
    if (st->st) {
        Z_Free(st->st);
        st->st = NULL;
        st->size = 0;
    }
}