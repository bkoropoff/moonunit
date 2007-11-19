#ifndef __MU_SCAN_H__
#define __MU_SCAN_H__

#include <moonunit/test.h>

struct __mu_library;
typedef struct __mu_library MoonUnitLibrary;

typedef void (*MoonUnitThunk) (void);

typedef struct MoonScanner
{
    // Opens a library and returns a handle
    MoonUnitLibrary* (*open) (const char* path);
    // Scans an open library for unit tests
    // and returns a NULL-terminated list
    MoonUnitTest** (*scan) (MoonUnitLibrary* handle);
    // Frees a list acquired with scan
    void (*cleanup) (MoonUnitTest** list);
    // Returns the library setup routine for handle
    MoonUnitThunk (*library_setup)(MoonUnitLibrary* handle);
    // Returns the library teardown routine for handle
    MoonUnitThunk (*library_teardown)(MoonUnitLibrary* handle);
    // Returns the fixture setup routine for suite name in handle
    MoonUnitThunk (*fixture_setup)(const char* name, MoonUnitLibrary* handle);
    // Returns the fixture teardown routine for suite name in handle
    MoonUnitThunk (*fixture_teardown)(const char* name, MoonUnitLibrary* handle);
    // Closes a library
    void (*close) (MoonUnitLibrary* handle);
} MoonScanner;

extern MoonScanner mu_unixscanner;

#endif
