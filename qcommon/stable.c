#include "qcommon.h"
#include "nflibs/nf_string_table.c"

qboolean Q_STInit(stable_t *st, int32_t avgLength) {
    if (!st->st) {
        st->st = Z_TagMalloc(st->size, TAG_SYSTEM);
        if (!st->st)
            return false;
        st->heap = true;
    }
    nfst_init((struct nfst_StringTable *)st->st, st->size, avgLength);

    return true;
}

int Q_STRegister(stable_t *st, const char *string) {
    int result = nfst_to_symbol((struct nfst_StringTable *)st->st, string);
    if (result == NFST_STRING_TABLE_FULL && st->heap) {
        void *new = NULL;
        int newsize = st->size * 2;
        if ((new = Z_Realloc(st->st, newsize)) != NULL) {
            st->st = new;
            st->size = newsize;
            nfst_grow((struct nfst_StringTable *)st->st, newsize);
            result = nfst_to_symbol((struct nfst_StringTable *)st->st, string);
        }
    
    }
    if (result == NFST_STRING_TABLE_FULL) {
        Com_Printf("Error registering string: %s\n",string);
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
    if (st->heap && size > MIN_SIZE && size <= st->size/2) {
        void *new = NULL;
        if ((new = Z_Realloc(st->st, size)) != NULL) {
            st->st = new;
            st->size = size;
        }
    }
}

void Q_STFree(stable_t *st) {
    if (st->heap && st->st) {
        Z_Free(st->st);
        st->st = NULL;
        st->size = 0;
    }
}