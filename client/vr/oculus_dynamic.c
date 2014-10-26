// Uses SDL for library loading. If you're not using SDL, you can
// replace SDL_LoadObject with dlopen/LoadLibrary and SDL_LoadFunction
// with dlsym/GetProcAddress

#include <SDL.h>
#include <stdlib.h>
#include "oculus_dynamic.h"

void *oculus_library_handle;

#define OVRFUNC(need, rtype, fn, params, cparams)       \
    pfn_##fn d_##fn;                                    \
                                                        \
    rtype fn params {                                   \
        return d_##fn cparams;                          \
    }
#define OVRFUNCV(need, fn, params, cparams)     \
    pfn_##fn d_##fn;                            \
                                                \
    void fn params {                            \
        d_##fn cparams;                         \
    }
#include "ovr_dynamic_funcs.h"

ovr_dynamic_load_result oculus_dynamic_load_library(const char* libname, const char** failed_function) {
    const char* current_function;

    if (!libname)
        libname = getenv("LIBOVR");

    if (!libname) {
#ifdef OVR_OS_WIN32
        libname = "libovr.dll";
#else
        libname = "./libovr.so";
#endif
    }

    oculus_library_handle = SDL_LoadObject(libname);
    if (!oculus_library_handle) {
        return OVR_DYNAMIC_RESULT_LIBOVR_COULD_NOT_LOAD;
    }

    return oculus_dynamic_load_handle(oculus_library_handle, failed_function);

}
ovr_dynamic_load_result oculus_dynamic_load_handle(void* libhandle, const char** failed_function) {
#define OVRFUNC(need, r, f, p, c)                                       \
    current_function = #f;                                              \
        d_##f = (pfn_##f)SDL_LoadFunction(libhandle, current_function); \
            if (need && !d_##f)                                         \
                goto error_function;
#include "ovr_dynamic_funcs.h"

    return OVR_DYNAMIC_RESULT_SUCCESS;

error_function:
    if (failed_function)
        *failed_function = current_function;
    return OVR_DYNAMIC_RESULT_LIBOVR_COULD_NOT_LOAD_FUNCTION;
}

void oculus_dynamic_free_library() {
    if (oculus_library_handle)
        SDL_FreeObject(oculus_library_handle);
    oculus_library_handle = NULL;
}
