#include <moonunit/loader.h>
#include <moonunit/util.h>
#include <moonunit/test.h>

#include <string.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

// Opens a library and returns a handle

struct MoonUnitLibrary
{
	const char* path;
	void* dlhandle;
};

static MoonUnitLibrary*
unixloader_open(const char* path)
{
	MoonUnitLibrary* library = malloc(sizeof (MoonUnitLibrary));
	
	library->path = strdup(path);
	library->dlhandle = dlopen(library->path, RTLD_LAZY);
	
	return library;
}

static MoonUnitTest** 
unixloader_scan (MoonUnitLibrary* handle)
{
	FILE* stream;
	const char* command;
	char buffer[1024];
	MoonUnitTest** tests;
	unsigned int tests_capacity = 256;
	unsigned int index = 0;
	
	command = format("nm '%s' | grep " MU_TEST_PREFIX " | sed 's/.*\\(" MU_TEST_PREFIX ".*\\)[ \t]*/\\1/g'",
					handle->path);
	stream = popen(command, "r");
	
	tests = malloc(tests_capacity * sizeof(*tests));
	
	while (fgets(buffer, sizeof(buffer)-1, stream))
	{
		unsigned int len = strlen(buffer);
		if (buffer[len-1] == '\n')
			buffer[len-1] = '\0';
		MoonUnitTest* test = dlsym(handle->dlhandle, buffer);
		
		if (test)
		{
			test->library = handle;
			test->loader = &mu_unixloader;
			
			if (index == tests_capacity - 1)
			{
				tests_capacity *= 2;
				tests = realloc(tests, tests_capacity * sizeof(*tests));
			}
			tests[index++] = test;
		}
	}
	
	tests[index] = NULL;
	free((void*) command);
	
	return tests;
}

void 
unixloader_cleanup (MoonUnitTest** list)
{
	free(list);
}
    
// Returns the library setup routine for handle
MoonUnitThunk
unixloader_library_setup (MoonUnitLibrary* handle)
{
	return dlsym(handle->dlhandle, "__mu_ls");
}

MoonUnitThunk
unixloader_library_teardown (MoonUnitLibrary* handle)
{
	return dlsym(handle->dlhandle, "__mu_lt");
}

MoonUnitThunk
unixloader_fixture_setup (const char* name, MoonUnitLibrary* handle)
{
	char* symbol_name = format(MU_FS_PREFIX "%s", name);
	void* result = dlsym(handle->dlhandle, symbol_name);
	free(symbol_name);
	return result;
}

MoonUnitThunk
unixloader_fixture_teardown (const char* name, MoonUnitLibrary* handle)
{
	char* symbol_name = format(MU_FT_PREFIX "%s", name);
	void* result = dlsym(handle->dlhandle, symbol_name);
	free(symbol_name);
	return result;
}
   
void
unixloader_close (MoonUnitLibrary* handle)
{
	dlclose(handle->dlhandle);
	free((void*) handle->path);
	free(handle);
}

const char*
unixloader_name (MoonUnitLibrary* handle)
{
	return basename(handle->path);
}

MoonUnitLoader mu_unixloader =
{
	unixloader_open,
	unixloader_scan,
	unixloader_cleanup,
	unixloader_library_setup,
	unixloader_library_teardown,
	unixloader_fixture_setup,
	unixloader_fixture_teardown,
	unixloader_close,
	unixloader_name
};
