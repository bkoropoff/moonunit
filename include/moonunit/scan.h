#ifndef __MU_SCAN_H__
#define __MU_SCAN_H__

#include <moonunit/test.h>

typedef struct MoonScanner
{
    // Opens a library and returns a handle
    void* (*open) (const char* path);
    // Scans an open library for unit tests
    // and returns a NULL-terminated list
    MoonUnitTest** (*scan) (void* handle)
    // Frees a list acquired with scan
    void (*cleanup) (MoonUnitTest** list);
    // Returns the library setup routine for handle
    (void (*)(void)) (*library_setup)(void* handle);
    // Returns the library teardown routine for handle
    (void (*)(void)) (*library_teardown)(void* handle);
    // Returns the fixture setup routine for suite name in handle
    (void (*)(void)) (*fixture_setup)(const char* name, void* handle);
    // Returns the fixture teardown routine for suite name in handle
    (void (*)(void)) (*fixture_teardown)(const char* name, void* handle);
    // Closes a library
    void (*close) (void* handle);
} MoonScanner;


#endif
