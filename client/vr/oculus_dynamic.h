/* OVR_CAPI.h should really define this. There should be no reason to
 * include all of the other C++ crap just to get something this
 * simple. */

#ifdef __linux__
#define OVR_OS_LINUX
#elif defined(__WIN32__)
#define OVR_OS_WIN32
#elif defined(__APPLE__)
#define OVR_OS_MACOS
#else
#error "Unknown O/S"
#endif

#include "../OVR_CAPI.h"

#define OVRFUNC(need, rtype, fn, params, cparams)       \
    typedef rtype (*pfn_ ## fn) params;                 \
    extern pfn_##fn d_##fn;
#include "ovr_dynamic_funcs.h"

typedef enum {
    OVR_DYNAMIC_RESULT_SUCCESS,
    OVR_DYNAMIC_RESULT_LIBOVR_COULD_NOT_LOAD,
    OVR_DYNAMIC_RESULT_LIBOVR_COULD_NOT_LOAD_FUNCTION
} ovr_dynamic_load_result;

#ifdef __cplusplus
extern "C" {
#endif

extern void *oculus_library_handle;
extern ovr_dynamic_load_result oculus_dynamic_load_library(const char* libname, const char** failed_function);
extern ovr_dynamic_load_result oculus_dynamic_load_handle(void* libhandle, const char** failed_function);
extern void oculus_dynamic_free_library();

#ifdef __cplusplus
}
#endif
