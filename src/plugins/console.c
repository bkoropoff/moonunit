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

#include <moonunit/logger.h>
#include <moonunit/plugin.h>
#include <moonunit/test.h>
#include <moonunit/util.h>

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct
{
    MuLogger base;

    int fd;
    char* file;
    FILE* out;

    int align;
    bool ansi;
    bool details;

    char* test_log;
} ConsoleLogger;

static void
enter(MuLogger* _self)
{
}

static void
leave(MuLogger* _self)
{
}

static void
library_enter(MuLogger* _self, const char* name)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

	fprintf(self->out, "Library: %s\n", name);
}

static void
library_leave(MuLogger* _self)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

	fprintf(self->out, "\n");
}

static void
suite_enter(MuLogger* _self, const char* name)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

	fprintf(self->out, "  Suite: %s\n", name);
}

static void
suite_leave(MuLogger* _self)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

	fprintf(self->out, "\n");
}

static void
test_enter(MuLogger* _self, MuTest* test)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

    self->test_log = format("");
}

static void
test_log(MuLogger* _self, MuLogEvent* event)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;
    char* level_str = NULL;
    char* old = NULL;
    int level_code = 0;

    switch (event->level)
    {
        case MU_LEVEL_WARNING:
            level_str = "warning"; level_code = 31; break;
        case MU_LEVEL_INFO:
            level_str = "info"; level_code = 33; break;
        case MU_LEVEL_VERBOSE:
            level_str = "verbose"; level_code = 34; break;
        case MU_LEVEL_TRACE:
            level_str = "trace"; level_code = 35; break;
    }

    old = self->test_log;

    if (self->ansi)
        self->test_log = format("%s      %s:%u: (\e[%im\e[1m%s\e[22m\e[0m) %s\n", old,
                            basename_pure(event->file), event->line,
                            level_code, level_str, event->message);
    else
        self->test_log = format("%s      %s:%u: (%s) %s\n", old,
                            basename_pure(event->file), event->line,
                            level_str, event->message);

    free(old);
}

static void
test_leave(MuLogger* _self, MuTest* test, MuTestResult* summary)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;
    FILE* out = self->out;
	int i;
	const char* reason, * stage;
	char* failure_message;
    bool result = summary->status == summary->expected;
    const char* result_str = NULL;
    unsigned int result_code;

	fprintf(out, "    %s:", test->name);
	
    
    if (result)
    {
        result_code = 32;

        switch (summary->status)
        {
        case MU_STATUS_SUCCESS:
            result_str = "PASS ";
            break;
		case MU_STATUS_FAILURE:
		case MU_STATUS_ASSERTION:
		case MU_STATUS_CRASH:
        case MU_STATUS_TIMEOUT:
        case MU_STATUS_EXCEPTION:
            result_str = "XFAIL";
            break;
        case MU_STATUS_SKIPPED:
            result_str = "SKIP ";
            result_code = 33;
            break;
        }
    }
    else
    {
        result_code = 31;
    
        switch (summary->status)
        {
        case MU_STATUS_SUCCESS:
            result_str = "XPASS";
            break;
		case MU_STATUS_FAILURE:
		case MU_STATUS_ASSERTION:
		case MU_STATUS_CRASH:
        case MU_STATUS_TIMEOUT:
        case MU_STATUS_EXCEPTION:
            result_str = "FAIL ";
            break;
        case MU_STATUS_SKIPPED:
            result_str = "SKIP ";
            result_code = 33;
            break;
        }
    }

    if (summary->status == MU_STATUS_SUCCESS || (result && !self->details))
    {
        for (i = self->align - strlen(test->name) - 5 - strlen(result_str); i > 0; i--)
            fprintf(out, " ");
        if (self->ansi)
            fprintf(out, "\e[%um\e[1m%s\e[22m\e[0m\n", result_code, result_str);
        else
            fprintf(out, "%s\n", result_str);
    }
    else
    {
        stage = Mu_TestStageToString(summary->stage);
		
        for (i = self->align - strlen(test->name) - strlen(stage) - 3 - 5 - strlen(result_str); i > 0; i--)
            fprintf(out, " ");
        
        reason = summary->reason ? summary->reason : "unknown";
        if (self->ansi)
        {
            fprintf(out, "(%s) \e[%um\e[1m%s\e[22m\e[0m\n", stage, result_code, result_str);
        }
        else
        {
            fprintf(out, "(%s) %s\n", stage, result_str);
        }
        
        failure_message = summary->line != 0 
            ? format("%s:%i: %s", basename_pure(test->file), summary->line, reason)
            : format("%s", reason);
        
        fprintf(out, "      %s\n", failure_message);
        
        free(failure_message);

        if (summary->backtrace)
        {
            MuBacktrace* frame;
            unsigned int i = 1;

            for (frame = summary->backtrace; frame; frame = frame->up)
            {
                fprintf(out, "        #%2u: ", i++);
                if (frame->func_name)
                {
                    fprintf(out, "%s ", frame->func_name);
                }
                else if (frame->file_name)
                {
                    fprintf(out, "<unknown> ");
                }

                if (frame->file_name)
                {
                    fprintf(out, "in %s", basename_pure(frame->file_name));
                }
                else if (frame->return_addr)
                {
                    fprintf(out, "[0x%lx]", frame->return_addr);
                }
                               
                fprintf(out, "\n");
            }
        }
	}
    
    fprintf(out, "%s", self->test_log);
    free(self->test_log);
    self->test_log = NULL;
}

static int
get_fd(ConsoleLogger* self)
{
    return self->fd;
}

static void
set_fd(ConsoleLogger* self, int fd)
{
    self->fd = dup(fd);
    if (self->out)
        fclose(self->out);
    self->out = fdopen(self->fd, "w");  
}

static const char*
get_file(ConsoleLogger* self)
{
    return self->file;
}

static void
set_file(ConsoleLogger* self, const char* file)
{
    if (self->file)
        free(self->file);
    self->file = strdup(file);
    if (self->out)
        fclose(self->out);
    self->out = fopen(self->file, "w");
}

static bool
get_ansi(ConsoleLogger* self)
{
    return self->ansi;
}

static void
set_ansi(ConsoleLogger* self, bool ansi)
{
    self->ansi = ansi;
}

static int
get_align(ConsoleLogger* self)
{
    return self->align;
}

static void
set_align(ConsoleLogger* self, int align)
{
    self->align = align;
}

static bool
get_details(ConsoleLogger* self)
{
    return self->details;
}

static void
set_details(ConsoleLogger* self, bool details)
{
    self->details = details;
}

static void
destroy(MuLogger* _self)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

    if (self->out)
        fclose(self->out);
    if (self->file)
        free(self->file);
    if (self->test_log)
        free(self->test_log);

    free(self);
}


static MuOption consolelogger_options[] =
{
    MU_OPTION("fd", MU_TYPE_INTEGER, get_fd, set_fd,
              "File descriptor to which results will be written"),
    MU_OPTION("file", MU_TYPE_STRING, get_file, set_file,
              "File to which results will be written"),
    MU_OPTION("ansi", MU_TYPE_BOOLEAN, get_ansi, set_ansi,
              "Whether to use ANSI color/fonts in output"),
    MU_OPTION("align", MU_TYPE_INTEGER, get_align, set_align,
              "Column number use for right-aligned output"),
    MU_OPTION("details", MU_TYPE_BOOLEAN, get_details, set_details,
              "Whether result details should be output for failed "
              "tests even if the failure is expected"),
    MU_OPTION_END
};

static ConsoleLogger consolelogger =
{
    .base = 
    {
        .enter = enter,
        .leave = leave,
        .library_enter = library_enter,
        .library_leave = library_leave,
        .suite_enter = suite_enter,
        .suite_leave = suite_leave,
        .test_enter = test_enter,
        .test_log = test_log,
        .test_leave = test_leave,
        .destroy = destroy,
        .options = consolelogger_options
    },
    .fd = -1,
    .out = NULL,
    .ansi = false,
    .align = 60
};

static MuLogger*
create_consolelogger()
{
    ConsoleLogger* logger = malloc(sizeof(ConsoleLogger));

    *logger = consolelogger;

    Mu_Logger_SetOption((MuLogger*) logger, "fd", fileno(stdout));

    return (MuLogger*) logger;
}

static MuPlugin plugin =
{
    .name = "console",
    .create_logger = create_consolelogger
};

MU_PLUGIN_INIT
{
    return &plugin;
}
