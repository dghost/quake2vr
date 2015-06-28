#include "qcommon.h"
#include "nflibs/nf_string_table.c"

qboolean Q_STInit(stable_t *st, int32_t avgLength) {
    if (!st->st) {
        st->st = Z_TagMalloc(st->size, TAG_SYSTEM);
        if (!st->st)
            return false;
        st->heap = true;
    } else {
        st->heap = false;
    }
    nfst_init((struct nfst_StringTable *)st->st, st->size, avgLength);
    return true;
}

int32_t Q_STAutoRegister(stable_t *st, const char *string) {
    assert(st->st != NULL);    
    int result = nfst_to_symbol((struct nfst_StringTable *)st->st, string);
    if (result == NFST_STRING_TABLE_FULL && st->heap) {
        void *new_one = NULL;
        int newsize = st->size * 2;
        if ((new_one = Z_Realloc(st->st, newsize)) != NULL) {
            st->st = new_one;
            st->size = newsize;
            nfst_grow((struct nfst_StringTable *)st->st, newsize);
            result = nfst_to_symbol((struct nfst_StringTable *)st->st, string);
        }
    
    }
    if (result == NFST_STRING_TABLE_FULL) {
        Com_Printf(S_COLOR_RED"Error: could not register string: %s\n",string);
    }
    
    return result;
}

int32_t Q_STLookup(stable_t st, const char *string) {
    assert(st.st != NULL);
    return nfst_to_symbol_const((struct nfst_StringTable *)st.st, string);
};

const char *Q_STGetString(stable_t st, int token) {
    assert(st.st != NULL);
    return nfst_to_string((struct nfst_StringTable *)st.st, token);
}

int32_t Q_STUsedBytes(const stable_t st) {
    assert(st.st != NULL);
    return nfst_allocated_bytes(st.st);
}

int32_t Q_STAutoPack(stable_t *st) {
    assert(st->st != NULL);
    int32_t size = nfst_pack((struct nfst_StringTable *)st->st);
    if (st->heap && size > MIN_SIZE && size <= st->size/2) {
        void *new_one = NULL;
		if ((new_one = Z_Realloc(st->st, size)) != NULL) {
			st->st = new_one;
            st->size = size;
        }
    }
    return size;
}

void Q_STFree(stable_t *st) {
    if (st->heap && st->st) {
        Z_Free(st->st);
        st->st = NULL;
//        st->size = 0;
    }
}