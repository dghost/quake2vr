#include "qcommon.h"
#include "nflibs/nf_string_table.c"

qboolean  Q_STInit(stable_t *st,  uint32_t size, int32_t avgLength, int16_t memoryTag) {
    assert(st != NULL);
    st->tag = memoryTag;
    st->size = size;
    st->st = Z_TagMalloc(st->size, st->tag);
    if (!st->st)
        return false;
    nfst_init((struct nfst_StringTable *)st->st, st->size, avgLength);
    return true;
}


qboolean Q_STGrow(stable_t *st, size_t newSize) {
    assert(st->st != NULL);
    
    // check to see if it's a valid size
    if (newSize > st->size && newSize > MIN_SIZE) {
        void *new_one = NULL;
        // try to reallocate it into the larger size
        if ((new_one = Z_Realloc(st->st, newSize)) != NULL) {
            // if that succeeds, save the size and grow the string table
            st->st = new_one;
            st->size = newSize;
            nfst_grow((struct nfst_StringTable *)st->st, newSize);
            return true;
        }
    }
    // leave everything untouched
    return false;
}

qboolean Q_STShrink(stable_t *st, size_t newSize) {
    assert(st->st != NULL);
    
    // make sure that this is a valid resize
    if (newSize > MIN_SIZE && newSize < st->size) {
        // pack the string table into the smallest possible footprint
        int32_t size = nfst_pack((struct nfst_StringTable *)st->st);
        
        // if the string table can fit into
        if (size <= newSize) {
            void *new_one = NULL;
            if ((new_one = Z_Realloc(st->st, newSize)) != NULL) {
                st->st = new_one;
                st->size = newSize;
                nfst_grow((struct nfst_StringTable *)st->st, st->size);
                return true;
            }
        }
        
        // if it got here the allocation didn't change
        // either it's not small enough, or realloc failed
        // either way, expand the string table back to fit the allocation
        nfst_grow((struct nfst_StringTable *)st->st, st->size);
    }
    return false;
}


void Q_STFree(stable_t *st) {
    assert(st != NULL);
    if (st->st) {
        Z_Free(st->st);
        st->st = NULL;
        st->tag = TAG_UNTAGGED;
        //        st->size = 0;
    }
}

int32_t Q_STRegister(const stable_t *st, const char *string) {
    assert(st != NULL && st->st != NULL && string != NULL);
    return nfst_to_symbol((struct nfst_StringTable *)st->st, string);;
}



int32_t Q_STLookup(const stable_t *st, const char *string) {
    assert(st != NULL && st->st != NULL && string != NULL);
    return nfst_to_symbol_const((struct nfst_StringTable *)st->st, string);
};

const char *Q_STGetString(const stable_t *st, int token) {
    assert(st != NULL && st->st != NULL);
    return nfst_to_string((struct nfst_StringTable *)st->st, token);
}

int32_t Q_STUsedBytes(const stable_t *st) {
    assert(st != NULL && st->st != NULL);
    return nfst_allocated_bytes(st->st);
}

int32_t Q_STAutoRegister(stable_t *st, const char *string) {
    assert(st != NULL && st->st != NULL && string != NULL);
    int result = nfst_to_symbol((struct nfst_StringTable *)st->st, string);
    if (result == NFST_STRING_TABLE_FULL) {
        if (Q_STGrow(st,st->size * 2)) {
            result = nfst_to_symbol((struct nfst_StringTable *)st->st, string);
        }
    }
    if (result == NFST_STRING_TABLE_FULL) {
        Com_Printf(S_COLOR_RED"Error: could not register string: %s\n",string);
    }
    return result;
}

int32_t Q_STAutoPack(stable_t *st) {
    assert(st->st != NULL);
    int32_t size = nfst_pack((struct nfst_StringTable *)st->st);
    if (size <= st->size) {
        void *new_one = NULL;
		if ((new_one = Z_Realloc(st->st, size)) != NULL) {
			st->st = new_one;
            st->size = size;
            return size;
        }
    }
    // if the reallocation failed, expand the string table again
    nfst_grow((struct nfst_StringTable *)st->st, st->size);
    return st->size;
}
