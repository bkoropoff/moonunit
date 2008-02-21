/*
 * Copyright (c) 2007, Brian Koropoff
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Moonunit project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRIAN KOROPOFF ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL BRIAN KOROPOFF BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <moonunit/loader.h>
#include <moonunit/util.h>
#include <moonunit/test.h>
#include <moonunit/error.h>

#include <string.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include <config.h>

#ifdef HAVE_LIBELF
#include "elfscan.h"
#endif

// Opens a library and returns a handle

struct MuLibrary
{
    MuLoader* loader;
	const char* path;
	void* dlhandle;
	MuTest** tests;
	MuLibrarySetup* library_setup;
    MuLibraryTeardown* library_teardown;
    MuFixtureSetup** fixture_setups;
    MuFixtureTeardown** fixture_teardowns;
    bool stub;
};

/* Some stupid helper functions */

static unsigned int
PtrArray_Length(void** array)
{
    unsigned int len;

    if (!array)
        return 0;

    for (len = 0; *array; array++, len++);

    return len;
}

#define PTRARRAY_LENGTH(array) (PtrArray_Length((void**) (array)))

static void**
PtrArray_Append(void** array, void* element)
{
    unsigned int len = PtrArray_Length(array);
    void** expanded = realloc(array, sizeof(*array) * (len+2));
    
    expanded[len] = element;
    expanded[len+1] = NULL;

    return expanded;
}

#define PTRARRAY_APPEND(array, element, type) ((type*) PtrArray_Append((void **) (array), (type) element))

static bool
test_filter(const char* sym, void *unused)
{
	return !strncmp("__mu_", sym, strlen("__mu_"));
}

static void
test_init(MuTest* test, MuLibrary* library)
{
    test->loader = library->loader;
    test->library = library;
    test->methods = &Mu_TestMethods;
}

static bool
test_add(symbol* sym, void* _library, MuError **_err)
{
    MuLibrary* library = (MuLibrary*) _library;	

	if (!strncmp (MU_TEST_PREFIX, sym->name, strlen(MU_TEST_PREFIX)))
	{
		MuTest* test = (MuTest*) sym->addr;
		
		if (test->library)
        {
			return true; // Test was already added
        }
		else
        {
            library->tests = PTRARRAY_APPEND(library->tests, test, MuTest*);

            if (!library->tests)
                MU_RAISE_RETURN(false, _err, Mu_ErrorDomain_General, MU_ERROR_NOMEM, "Out of memory");
	
            test_init(test, library);
        }
  	}
   	else if (!strncmp(MU_FS_PREFIX, sym->name, strlen(MU_FS_PREFIX)))
   	{
        MuFixtureSetup* setup = (MuFixtureSetup*) sym->addr;
		
        library->fixture_setups = PTRARRAY_APPEND(library->fixture_setups, setup, MuFixtureSetup*);
	}
    else if (!strncmp(MU_FT_PREFIX, sym->name, strlen(MU_FT_PREFIX)))
   	{
        MuFixtureTeardown* teardown = (MuFixtureTeardown*) sym->addr;
		
        library->fixture_teardowns = PTRARRAY_APPEND(library->fixture_teardowns, teardown, MuFixtureTeardown*);
	}
	else if (!strncmp("__mu_ls", sym->name, strlen("__mu_ls")))
	{
		library->library_setup = (MuLibrarySetup*) sym->addr;	
	}
	else if (!strncmp("__mu_lt", sym->name, strlen("__mu_lt")))
	{
		library->library_teardown = (MuLibraryTeardown*) sym->addr;
	}

    return true;
}

#ifdef HAVE_LIBELF
static bool
unixloader_scan (MuLoader* _self, MuLibrary* handle, MuError ** _err)
{
    MuError* err = NULL;

	if (!ElfScan_GetScanner()(handle->dlhandle, test_filter, test_add, handle, &err))
    {
        MU_RERAISE_GOTO(error, _err, err);
    }
	
    return true;

error:
    
    return false;
}
#else
static bool
unixloader_scan (MuLoader* _self, MuLibrary* handle)
{
	const char* command;
	
	command = format("nm '%s' | grep " MU_TEST_PREFIX " | sed 's/.*\\(" MU_TEST_PREFIX ".*\\)[ \t]*/\\1/g'",
					handle->path);

	free((void*) command);
	
	return tests;
}
#endif

static MuLibrary*
unixloader_open(MuLoader* _self, const char* path, MuError** _err)
{
	MuLibrary* library = malloc(sizeof (MuLibrary));
    MuError* err = NULL;
    void (*stub_hook)(MuLibrarySetup** ls, MuLibraryTeardown** lt,
                      MuFixtureSetup*** fss, MuFixtureTeardown*** fts,
                      MuTest*** ts);


    if (!library)
    {
        MU_RAISE_RETURN(NULL, _err, Mu_ErrorDomain_General, MU_ERROR_NOMEM, "Out of memory");
    }

    library->loader = _self;
	library->tests = NULL;
	library->fixture_setups = NULL;
    library->fixture_teardowns = NULL;
	library->library_setup = NULL;
    library->library_teardown = NULL;
	library->path = strdup(path);
	library->dlhandle = dlopen(library->path, RTLD_LAZY);
    library->stub = false;

    if (!library->dlhandle)
    {
        free(library);
        MU_RAISE_RETURN(NULL, _err, Mu_ErrorDomain_General, MU_ERROR_GENERIC, "%s", dlerror());
    }

    if ((stub_hook = dlsym(library->dlhandle, "__mu_stub_hook")))
    {
        int i;
        stub_hook(&library->library_setup, &library->library_teardown,
                  &library->fixture_setups, &library->fixture_teardowns,
                  &library->tests);

        for (i = 0; library->tests[i]; i++)
        {
            test_init(library->tests[i], library);
        }

        library->stub = true;
    }
    else if (!unixloader_scan(_self, library, &err))
    {
        dlclose(library->dlhandle);
        free(library);
        
        MU_RERAISE_RETURN(NULL, _err, err);
    }
	
	return library;
}

static MuTest**
unixloader_tests (MuLoader* _self, MuLibrary* handle)
{
	return handle->tests;
}
    
// Returns the library setup routine for handle
static MuThunk
unixloader_library_setup (MuLoader* _self, MuLibrary* handle)
{
    if (handle->library_setup)
    	return handle->library_setup->function;
    else
        return NULL;
}

static MuThunk
unixloader_library_teardown (MuLoader* _self, MuLibrary* handle)
{
    if (handle->library_teardown)
    	return handle->library_teardown->function;
    else
        return NULL;
}

static MuTestThunk
unixloader_fixture_setup (MuLoader* _self, const char* name, MuLibrary* handle)
{
	unsigned int i;
	
	for (i = 0; handle->fixture_setups[i]; i++)
	{
		if (!strcmp(name, handle->fixture_setups[i]->name))
        {
			return handle->fixture_setups[i]->function;
        }
	}
    
	return NULL;
}

static MuTestThunk
unixloader_fixture_teardown (MuLoader* _self, const char* name, MuLibrary* handle)
{
	unsigned int i;
	
	for (i = 0; handle->fixture_teardowns[i]; i++)
	{
		if (!strcmp(name, handle->fixture_teardowns[i]->name))
        {
			return handle->fixture_teardowns[i]->function;
        }
	}
	
	return NULL;
}
   
static void
unixloader_close (MuLoader* _self, MuLibrary* handle)
{
	unsigned int i;
	
	dlclose(handle->dlhandle);
	free((void*) handle->path);

    if (!handle->stub)
    {
    	free(handle->tests);
        free(handle->fixture_setups);
        free(handle->fixture_teardowns);
    }
   	free(handle);
}

static const char*
unixloader_name (MuLoader* _self, MuLibrary* handle)
{
	return basename(handle->path);
}

MuLoader mu_unixloader =
{
    .plugin = NULL,
	.open = unixloader_open,
	.tests = unixloader_tests,
	.library_setup = unixloader_library_setup,
	.library_teardown = unixloader_library_teardown,
	.fixture_setup = unixloader_fixture_setup,
	.fixture_teardown = unixloader_fixture_teardown,
	.close = unixloader_close,
	.name = unixloader_name
};
