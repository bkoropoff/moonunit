
#ifndef __OPTION_H__
#define __OPTION_H__

#include <stdbool.h>
#include <popt.h>

typedef struct
{
    char ** value;
    unsigned int size;
    unsigned int capacity;
} StringSet;

typedef struct 
{
    bool gdb;
    bool all;
    char* logger;
    StringSet tests;
    StringSet files;
    StringSet logger_options;
    char* errormsg;

    poptContext context;
} OptionTable;

struct MoonUnitLogger;

int Option_Parse(int argc, char** argv, OptionTable* option);
int Option_ApplyToLogger(OptionTable* option, struct MoonUnitLogger* logger);
void Option_Release(OptionTable* option);

#endif
