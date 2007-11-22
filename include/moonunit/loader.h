#ifndef __MU_LOADER_H__
#define __MU_LOADER_H__

struct MoonUnitTest;
struct MoonUnitLibrary;
typedef struct MoonUnitLibrary MoonUnitLibrary;

typedef void (*MoonUnitThunk) (void);
typedef void (*MoonUnitTestThunk) (struct MoonUnitTest*);

typedef struct MoonUnitLoader
{
    // Opens a library and returns a handle
    MoonUnitLibrary* (*open) (const char* path);
    // Returns a null-terminated list of unit tests
    struct MoonUnitTest** (*tests) (MoonUnitLibrary* handle);
    // Returns the library setup routine for handle
    MoonUnitThunk (*library_setup)(MoonUnitLibrary* handle);
    // Returns the library teardown routine for handle
    MoonUnitThunk (*library_teardown)(MoonUnitLibrary* handle);
    // Returns the fixture setup routine for suite name in handle
    MoonUnitTestThunk (*fixture_setup)(const char* name, MoonUnitLibrary* handle);
    // Returns the fixture teardown routine for suite name in handle
    MoonUnitTestThunk (*fixture_teardown)(const char* name, MoonUnitLibrary* handle);
    // Closes a library
    void (*close) (MoonUnitLibrary* handle);
    // Get name of a library
    const char * (*name) (MoonUnitLibrary* handle);
} MoonUnitLoader;

extern MoonUnitLoader mu_unixloader;

#endif
