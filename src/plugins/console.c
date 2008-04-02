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

#include <moonunit/harness.h>
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
        case MU_LOG_WARNING:
            level_str = "warning"; level_code = 31; break;
        case MU_LOG_INFO:
            level_str = "info"; level_code = 33; break;
        case MU_LOG_VERBOSE:
            level_str = "verbose"; level_code = 34; break;
        case MU_LOG_TRACE:
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
test_leave(MuLogger* _self, MuTest* test, MuTestSummary* summary)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;
    FILE* out = self->out;
	int i;
	const char* reason, * stage;
	char* failure_message;

	fprintf(out, "    %s:", test->name);
	
	switch (summary->result)
	{
		case MOON_RESULT_SUCCESS:
			for (i = self->align - strlen(test->name) - 5 - 4; i > 0; i--)
				fprintf(out, " ");
            if (self->ansi)
                fprintf(out, "\e[32m\e[1mPASS\e[22m\e[0m\n");
            else
                fprintf(out, "PASS\n");
			break;
		case MOON_RESULT_FAILURE:
		case MOON_RESULT_ASSERTION:
		case MOON_RESULT_CRASH:
        case MOON_RESULT_TIMEOUT:
			stage = Mu_TestStageToString(summary->stage);
			
			for (i = self->align - strlen(test->name) - strlen(stage) - 3 - 5 - 4; i > 0; i--)
				fprintf(out, " ");
			
			reason = summary->reason ? summary->reason : "unknown";
            if (self->ansi)
                fprintf(out, "(%s) \e[31m\e[1mFAIL\e[22m\e[0m\n", stage);
            else
                fprintf(out, "(%s) FAIL\n", stage);
			
			failure_message = summary->line != 0 
				? format("%s:%i: %s", basename_pure(test->file), summary->line, reason)
				: format("%s", reason);
			
			for (i = self->align - strlen(failure_message); i > 0; i--)
				fprintf(out, " ");
			fprintf(out, "%s\n", failure_message);

            free(failure_message);
	}

    fprintf(out, "%s", self->test_log);
    free(self->test_log);    
}

static void
option_set(void* _self, const char* name, void* data)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

    if (!strcmp(name, "fd"))
    {
        self->fd = dup(*(int*) data);
        
        if (self->out)
            fclose(self->out);

        self->out = fdopen(self->fd, "w");
    }
    else if (!strcmp(name, "file"))
    {
        if (self->file)
            free(self->file);
        self->file = strdup((char*) data);
        if (self->out)
            fclose(self->out);
        self->out = fopen(self->file, "w");
    }
    else if (!strcmp(name, "ansi"))
    {
        self->ansi = *(bool*) data;
    }
    else if (!strcmp(name, "align"))
    {
        self->align = *(int*) data;
    }
}

static const void*
option_get(void* _self, const char* name)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

    if (!strcmp(name, "fd"))
    {
        return &self->fd;
    }
    else if (!strcmp(name, "file"))
    {
        return self->file;
    }
    else if (!strcmp(name, "ansi"))
    {
        return &self->ansi;
    }
    else if (!strcmp(name, "align"))
    {
        return &self->align;
    }
    else
    {
        return NULL;
    }
}


static MuType
option_type(void* _self, const char* name)
{
    if (!strcmp(name, "fd"))
    {
        return MU_INTEGER;
    }
    else if (!strcmp(name, "file"))
    {
        return MU_STRING;
    }
    else if (!strcmp(name, "ansi"))
    {
        return MU_BOOLEAN;
    }
    else if (!strcmp(name, "align"))
    {
        return MU_INTEGER;
    }
    else
    {
        return MU_UNKNOWN_TYPE;
    }
}

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
        .option = 
        {
            .get = option_get,
            .set = option_set,
            .type = option_type
        }
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
