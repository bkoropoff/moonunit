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

#include <string.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_LIBELF
#include "elfscan.h"
#endif

// Opens a library and returns a handle

typedef struct
{
	const char* name;
	MoonUnitTestThunk thunk;
} NamedTestThunk;

struct MoonUnitLibrary
{
	const char* path;
	void* dlhandle;
	MoonUnitTest** tests;
	MoonUnitThunk setup, teardown;
	NamedTestThunk** fixture_thunks;
};

typedef struct
{
	struct
	{
		MoonUnitTest** tests;
		unsigned long index;
		unsigned long capacity;
	} test;
	struct
	{
		NamedTestThunk** thunks;
		unsigned long index;
		unsigned long capacity;
	} fixture;
	
	MoonUnitLoader* loader;
	MoonUnitLibrary* library;
} testbuffer;

static bool
test_filter(const char* sym, void *unused)
{
	return !strncmp("__mu_", sym, strlen("__mu_"));
}

static void
test_add(symbol* sym, void* _buffer)
{
	testbuffer* buffer = (testbuffer*) _buffer;

	if (!strncmp (MU_TEST_PREFIX, sym->name, strlen(MU_TEST_PREFIX)))
	{
		MoonUnitTest* test = (MoonUnitTest*) sym->addr;
		
		if (test->library)
			return; // Test was already added
		
		if (buffer->test.index >= buffer->test.capacity-1)
		{
			buffer->test.capacity *= 2;
			buffer->test.tests = realloc(buffer->test.tests, sizeof(test) * buffer->test.capacity);
		}
		
		buffer->test.tests[buffer->test.index++] = test;
	
		test->loader = buffer->loader;
		test->library = buffer->library;
		test->methods = &Mu_TestMethods;
  	}
   	else if (!strncmp(MU_FS_PREFIX, sym->name, strlen(MU_FS_PREFIX)) ||
   			 !strncmp(MU_FT_PREFIX, sym->name, strlen(MU_FT_PREFIX)))
   	{
		NamedTestThunk* thunk = malloc(sizeof(*thunk));
		thunk->name = strdup(sym->name);
		thunk->thunk = (MoonUnitTestThunk) sym->addr;
		
		if (buffer->fixture.index >= buffer->fixture.capacity-1)
		{
			buffer->fixture.capacity *= 2;
			buffer->fixture.thunks = realloc(buffer->fixture.thunks, sizeof(thunk) * buffer->fixture.capacity);
		}
		
		buffer->fixture.thunks[buffer->fixture.index++] = thunk;
	}
	else if (!strncmp("__mu_ls", sym->name, strlen("__mu_ls")))
	{
		buffer->library->setup = (MoonUnitThunk) sym->addr;	
	}
	else if (!strncmp("__mu_lt", sym->name, strlen("__mu_lt")))
	{
		buffer->library->teardown = (MoonUnitThunk) sym->addr;
	}
}

#ifdef HAVE_LIBELF
static void
unixloader_scan (MoonUnitLibrary* handle)
{
	testbuffer buffer = {{NULL, 0, 512}, {NULL, 0, 512}, &mu_unixloader, handle};

	buffer.test.tests = malloc(sizeof(MoonUnitTest*) * buffer.test.capacity);
	buffer.fixture.thunks = malloc(sizeof(NamedTestThunk*) * buffer.fixture.capacity);
	
	ElfScan_GetScanner()(handle->dlhandle, test_filter, test_add, &buffer);
	
	buffer.test.tests[buffer.test.index] = NULL;
	buffer.fixture.thunks[buffer.fixture.index] = NULL;
	
	handle->tests = buffer.test.tests;
	handle->fixture_thunks = buffer.fixture.thunks;
}
#else
static void
unixloader_scan (MoonUnitLibrary* handle)
{
	const char* command;
	
	command = format("nm '%s' | grep " MU_TEST_PREFIX " | sed 's/.*\\(" MU_TEST_PREFIX ".*\\)[ \t]*/\\1/g'",
					handle->path);

	free((void*) command);
	
	return tests;
}
#endif

static MoonUnitLibrary*
unixloader_open(const char* path)
{
	MoonUnitLibrary* library = malloc(sizeof (MoonUnitLibrary));
	
	library->tests = NULL;
	library->fixture_thunks = NULL;
	library->setup = NULL;
	library->teardown = NULL;
	library->path = strdup(path);
	library->dlhandle = dlopen(library->path, RTLD_LAZY);
	
	unixloader_scan(library);
	
	return library;
}

static MoonUnitTest**
unixloader_tests (MoonUnitLibrary* handle)
{
	return handle->tests;
}
    
// Returns the library setup routine for handle
static MoonUnitThunk
unixloader_library_setup (MoonUnitLibrary* handle)
{
	return handle->setup;
}

static MoonUnitThunk
unixloader_library_teardown (MoonUnitLibrary* handle)
{
	return handle->teardown;
}

static MoonUnitTestThunk
unixloader_fixture_setup (const char* name, MoonUnitLibrary* handle)
{
	char* symbol_name = format(MU_FS_PREFIX "%s", name);
	unsigned int i;
	
	for (i = 0; handle->fixture_thunks[i]; i++)
	{
		if (!strcmp(symbol_name, handle->fixture_thunks[i]->name))
			return handle->fixture_thunks[i]->thunk;
	}
	
	return NULL;
}

static MoonUnitTestThunk
unixloader_fixture_teardown (const char* name, MoonUnitLibrary* handle)
{
	char* symbol_name = format(MU_FT_PREFIX "%s", name);
	unsigned int i;
	
	for (i = 0; handle->fixture_thunks[i]; i++)
	{
		if (!strcmp(symbol_name, handle->fixture_thunks[i]->name))
			return handle->fixture_thunks[i]->thunk;
	}
	
	return NULL;
}
   
static void
unixloader_close (MoonUnitLibrary* handle)
{
	unsigned int i;
	
	dlclose(handle->dlhandle);
	free((void*) handle->path);
	free(handle->tests);
	
	for (i = 0; handle->fixture_thunks[i]; i++)
	{
		free((void*) handle->fixture_thunks[i]->name);
		free(handle->fixture_thunks[i]);
	}
	
	free(handle->fixture_thunks);
	free(handle);
}

static const char*
unixloader_name (MoonUnitLibrary* handle)
{
	return basename(handle->path);
}

MoonUnitLoader mu_unixloader =
{
	unixloader_open,
	unixloader_tests,
	unixloader_library_setup,
	unixloader_library_teardown,
	unixloader_fixture_setup,
	unixloader_fixture_teardown,
	unixloader_close,
	unixloader_name
};
