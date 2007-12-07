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
    MoonUnitLogger base;

    int fd;
    FILE* out;

    int align;
    bool ansi;
} ConsoleLogger;

static void
library_enter(MoonUnitLogger* _self, const char* name)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

	fprintf(self->out, "Library: %s\n", name);
}

static void
library_leave(MoonUnitLogger* _self)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

	fprintf(self->out, "\n");
}

static void
suite_enter(MoonUnitLogger* _self, const char* name)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

	fprintf(self->out, "  Suite: %s\n", name);
}

static void
suite_leave(MoonUnitLogger* _self)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

	fprintf(self->out, "\n");
}

static void
result(MoonUnitLogger* _self, MoonUnitTest* test, MoonUnitTestSummary* summary)
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
                fprintf(out, "\e[32mPASS\e[0m\n");
            else
                fprintf(out, "PASS\n");
			break;
		case MOON_RESULT_FAILURE:
		case MOON_RESULT_ASSERTION:
		case MOON_RESULT_CRASH:
			stage = Mu_TestStageToString(summary->stage);
			
			for (i = self->align - strlen(test->name) - strlen(stage) - 3 - 5 - 4; i > 0; i--)
				fprintf(out, " ");
			
			reason = summary->reason ? summary->reason : "unknown";
            if (self->ansi)
                fprintf(out, "(%s) \e[31mFAIL\e[0m\n", stage);
            else
                fprintf(out, "(%s) FAIL\n", stage);
			
			failure_message = summary->line != 0 
				? format("%s:%i: %s", basename(test->file), summary->line, reason)
				: format("%s", reason);
			
			for (i = self->align - strlen(failure_message); i > 0; i--)
				fprintf(out, " ");
			fprintf(out, "%s\n", failure_message);

            free(failure_message);
	}
}

static void
option_set(void* _self, const char* name, void* data)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

    if (!strcmp(name, "fd"))
    {
        self->fd = dup(*(int*) data);
        self->out = fdopen(self->fd, "w");
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


static MoonUnitType
option_type(void* _self, const char* name)
{
    if (!strcmp(name, "fd"))
    {
        return MU_INTEGER;
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
        .library_enter = library_enter,
        .library_leave = library_leave,
        .suite_enter = suite_enter,
        .suite_leave = suite_leave,
        .result = result,
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
    .align = 80
};

static MoonUnitLogger*
create_consolelogger()
{
    ConsoleLogger* logger = malloc(sizeof(ConsoleLogger));

    *logger = consolelogger;

    return (MoonUnitLogger*) logger;
}

static MoonUnitPlugin plugin =
{
    .name = "console",
    .create_loader = NULL,
    .create_harness = NULL,
    .create_runner = NULL,
    .create_logger = create_consolelogger
};

MU_PLUGIN_INIT
{
    return &plugin;
}
