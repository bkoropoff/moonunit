#include "gdb.h"

#include <moonunit/util.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void gdb_attach_interactive(const char* program, pid_t pid, const char* breakpoint)
{
    char *command;
    char template[] = "/tmp/mu_gdbinit_XXXXXX";

    int fd = mkstemp(template);

    if (fd < 0)
        return;

    FILE* file = fdopen(fd, "w");

    fprintf(file, "break %s\n", breakpoint);
    fprintf(file, "signal SIGCONT");

    fclose(file);

    command = format("gdb '%s' %lu -x '%s' -q", program, (unsigned long) pid, template);

    system(command);

    unlink(template);

    free(command);
}

void gdb_attach_backtrace(const char* program, pid_t pid, char **backtrace)
{
    char* buffer;
    unsigned int capacity = 2048;
    char *command;
    char template[] = "/tmp/mu_gdbinit_XXXXXX";
    unsigned int position;
    size_t bytes;
    FILE* file;

    int fd = mkstemp(template);

    if (fd < 0)
        return;

    file = fdopen(fd, "w");   

    fprintf(file, "bt");

    fclose(file);

    command = format("gdb '%s' %lu -x '%s' --batch'", program, (unsigned long) pid, template);

    file = popen(command, "r");

    if (!file)
        return;

    buffer = malloc(capacity * sizeof(*buffer));
    position = 0;

    while ((bytes = fread(buffer + position, capacity - position - 1, sizeof(*buffer), file)) > 0)
    {
        position += bytes;
        if (position >= capacity - 1)
        {
            capacity *= 2;
            buffer = realloc(buffer, capacity * sizeof(*buffer));
        }
    }

    pclose(file);

    free(command);
    unlink(template);

    *backtrace = buffer;
}
