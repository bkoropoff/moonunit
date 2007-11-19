#include <moonunit/scan.h>
#include <moonunit/util.h>
#include <moonunit/test.h>

#include <string.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

// Opens a library and returns a handle

struct __mu_library
{
	const char* path;
	void* dlhandle;
};

static MoonUnitLibrary*
unixscanner_open(const char* path)
{
	MoonUnitLibrary* library = malloc(sizeof (MoonUnitLibrary));
	
	library->path = strdup(path);
	library->dlhandle = dlopen(library->path, RTLD_LAZY);
	
	return library;
}

static MoonUnitTest** 
unixscanner_scan (MoonUnitLibrary* handle)
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
			test->library = basename(handle->path);
			
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
unixscanner_cleanup (MoonUnitTest** list)
{
	free(list);
}
    
// Returns the library setup routine for handle
MoonUnitThunk
unixscanner_library_setup (MoonUnitLibrary* handle)
{
	return dlsym(handle->dlhandle, "__mu_ls");
}

MoonUnitThunk
unixscanner_library_teardown (MoonUnitLibrary* handle)
{
	return dlsym(handle->dlhandle, "__mu_lt");
}

MoonUnitThunk
unixscanner_fixture_setup (const char* name, MoonUnitLibrary* handle)
{
	char* symbol_name = format(MU_FS_PREFIX "%s", name);
	void* result = dlsym(handle->dlhandle, symbol_name);
	free(symbol_name);
	return result;
}

MoonUnitThunk
unixscanner_fixture_teardown (const char* name, MoonUnitLibrary* handle)
{
	char* symbol_name = format(MU_FT_PREFIX "%s", name);
	void* result = dlsym(handle->dlhandle, symbol_name);
	free(symbol_name);
	return result;
}
   
void
unixscanner_close (MoonUnitLibrary* handle)
{
	dlclose(handle->dlhandle);
	free((void*) handle->path);
	free(handle);
}

MoonScanner mu_unixscanner =
{
	unixscanner_open,
	unixscanner_scan,
	unixscanner_cleanup,
	unixscanner_library_setup,
	unixscanner_library_teardown,
	unixscanner_fixture_setup,
	unixscanner_fixture_teardown,
	unixscanner_close
};
