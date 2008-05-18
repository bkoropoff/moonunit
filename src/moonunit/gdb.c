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

#include "gdb.h"

#include <moonunit/private/util.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void gdb_attach_interactive(const char* program, pid_t pid, const char* breakpoint)
{
    int result;
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

    result = system(command);
    if (result)
	fprintf(stderr, "WARNING: failed to create interactive gdb session");

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
