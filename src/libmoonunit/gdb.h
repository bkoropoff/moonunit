#ifndef __MU_INTERNAL_GDB_H__
#define __MU_INTERNAL_GDB_H__

#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>

void gdb_attach_interactive(const char* program, pid_t pid, const char* breakpoint);
void gdb_attach_backtrace(const char* program, pid_t pid, char **backtrace);

#endif
