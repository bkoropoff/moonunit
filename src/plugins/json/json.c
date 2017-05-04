/*
 * Copyright (c) 2014, Brian Koropoff
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

#include <config.h>

#include <moonunit/plugin.h>
#include <moonunit/logger.h>
#include <moonunit/test.h>
#include <moonunit/library.h>
#include <moonunit/private/util.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define INDENT "    "
#define MAX_NEST 32
#define VERSION "MoonUnit 0.7"

typedef struct
{
    bool need_comma;
} JsonState;

typedef struct
{
    MuLogger base;
    int fd;
    char* file;
    FILE* out;
    MuTest* current_test;
    char* title;
    MuLogLevel loglevel;
    bool pretty;
    bool need_nl;
    bool need_suites;
    bool need_events;
    unsigned int nest;
    JsonState state[MAX_NEST];
} JsonLogger;

static void
do_comma_nl(JsonLogger* self)
{
    JsonState* state = &self->state[self->nest];
    unsigned int i = 0;

    if (state->need_comma)
    {
        fputs(",", self->out);
        state->need_comma = false;
    }

    if (self->pretty && self->need_nl)
    {
        fputs("\n", self->out);
        for (i = 0; i < self->nest; i++)
        {
            fputs(INDENT, self->out);
        }
        self->need_nl = false;
    }
}

static void
output(JsonLogger* self, char const* str)
{
    do_comma_nl(self);
    fputs(str, self->out);
}


static void
outputc(JsonLogger* self, char c)
{
    do_comma_nl(self);
    fputc(c, self->out);
}


static void
print(JsonLogger* self, char const* f, ...)
{
    va_list ap;

    do_comma_nl(self);
    va_start(ap, f);
    vfprintf(self->out, f, ap);
    va_end(ap);
}

static void
escape(JsonLogger* self, char const* str)
{
    for (; *str; str++)
    {
        if (*str <= 0x1f)
        {
            print(self, "\\u%.4x", (unsigned int) *str);
        }
        else switch (*str)
        {
        case '"':
            output(self, "\"");
            break;
        case '\\':
            output(self, "\\");
            break;
        default:
            outputc(self, *str);
        }
    }
}

static void
nest(JsonLogger* self)
{
    self->nest++;
    assert(self->nest < MAX_NEST);
    self->state[self->nest].need_comma = false;
}

static void
unnest(JsonLogger* self)
{
    self->nest--;
}

static void
need_comma(JsonLogger* self)
{
    self->state[self->nest].need_comma = true;
}

static void
need_nl(JsonLogger* self)
{
    self->need_nl = true;
}

static void
begin_object(JsonLogger* self)
{
    output(self, "{");
    need_nl(self);
    nest(self);
}

static void
end_object(JsonLogger* self)
{
    unnest(self);
    output(self, "}");
}

static void
begin_array(JsonLogger* self)
{
    output(self, "[");
    need_nl(self);
    nest(self);
}

static void
end_array(JsonLogger* self)
{
    unnest(self);
    output(self, "]");
}

static void
key_begin(JsonLogger* self, char const* key)
{
    output(self, "\"");
    escape(self, key);
    output(self, "\":");
    if (self->pretty)
    {
        outputc(self, ' ');
    }
}

static void
key_end(JsonLogger* self)
{
    need_nl(self);
    need_comma(self);
}

static void
string(JsonLogger* self, char const* value)
{
    output(self, "\"");
    escape(self, value);
    output(self, "\"");
}

static void
integer(JsonLogger* self, int value)
{
    print(self, "%d", value);
}

static void
key_string(JsonLogger* self, char const* key, char const* value)
{
    key_begin(self, key);
    string(self, value);
    key_end(self);
}

static void
key_integer(JsonLogger* self, char const* key, int value)
{
    key_begin(self, key);
    integer(self, value);
    key_end(self);
}

static void
key_array_begin(JsonLogger* self, char const* key)
{
    key_begin(self, key);
    begin_array(self);
}

static void
key_array_end(JsonLogger* self)
{
    end_array(self);
    key_end(self);
}

static void
key_object_begin(JsonLogger* self, char const* key)
{
    key_begin(self, key);
    begin_object(self);
}

static void
key_object_end(JsonLogger* self)
{
    end_object(self);
    key_end(self);
}

static void
elem_begin(JsonLogger* self)
{
}

static void
elem_end(JsonLogger* self)
{
    need_nl(self);
    need_comma(self);
}

static void
elem_object_begin(JsonLogger* self)
{
    elem_begin(self);
    begin_object(self);
}

static void
elem_object_end(JsonLogger* self)
{
    end_object(self);
    elem_end(self);
}

static void
enter(MuLogger* _self)
{
    JsonLogger* self = (JsonLogger*) _self;

    begin_object(self);
    key_string(self, "version", VERSION);
    if (self->title)
    {
        key_string(self, "title", self->title);
    }
    key_array_begin(self, "libraries");
}

static void
leave(MuLogger* _self)
{
    JsonLogger* self = (JsonLogger*) _self;

    key_array_end(self);
    end_object(self);

    output(self, "\n");
}

static void library_enter(MuLogger* _self, const char* path, MuLibrary* library)
{
    JsonLogger* self = (JsonLogger*) _self;

    elem_object_begin(self);
    key_string(self, "file", path);
    if (library)
    {
        key_string(self, "name", mu_library_name(library));
    }
    self->need_suites = true;
}

static void library_fail(MuLogger* _self, const char* reason)
{
    JsonLogger* self = (JsonLogger*) _self;

    key_string(self, "failure", reason);
}

static void library_leave(MuLogger* _self)
{
    JsonLogger* self = (JsonLogger*) _self;

    if (!self->need_suites)
    {
        key_array_end(self);
    }

    elem_object_end(self);
}

static void suite_enter(MuLogger* _self, const char* name)
{
    JsonLogger* self = (JsonLogger*) _self;

    if (self->need_suites)
    {
        key_array_begin(self, "suites");
        self->need_suites = false;
    }

    elem_object_begin(self);
    key_string(self, "name", name);
    key_array_begin(self, "tests");
}

static void suite_leave(MuLogger* _self)
{
    JsonLogger* self = (JsonLogger*) _self;

    key_array_end(self);
    elem_object_end(self);
}

static void test_enter(MuLogger* _self, MuTest* test)
{
    JsonLogger* self = (JsonLogger*) _self;

    self->current_test = test;

    elem_object_begin(self);
    key_string(self, "name", mu_test_name(test));

    self->need_events = true;
}

static void test_log(MuLogger* _self, MuLogEvent const* event)
{
    JsonLogger* self = (JsonLogger*) _self;
    const char* level_str = "unknown";

    if (event->level > self->loglevel)
    {
        return;
    }

    if (self->need_events)
    {
        key_array_begin(self, "events");
        self->need_events = false;
    }

    switch (event->level)
    {
        case MU_LEVEL_WARNING:
            level_str = "warning"; break;
        case MU_LEVEL_INFO:
            level_str = "info"; break;
        case MU_LEVEL_VERBOSE:
            level_str = "verbose"; break;
        case MU_LEVEL_DEBUG:
            level_str = "debug"; break;
        case MU_LEVEL_TRACE:
            level_str = "trace"; break;
    }

    elem_object_begin(self);

    key_string(self, "level", level_str);
    key_string(self, "stage", mu_test_stage_to_string(event->stage));

    if (event->file)
    {
        key_string(self, "file", event->file);
    }

    if (event->line)
    {
        key_integer(self, "line", event->line);
    }

    if (event->message)
    {
        key_string(self, "message", event->message);
    }

    elem_object_end(self);
}

static void test_leave(MuLogger* _self,
                       MuTest* test, MuTestResult* summary)
{
    JsonLogger* self = (JsonLogger*) _self;

    if (!self->need_events)
    {
        key_array_end(self);
    }

    key_object_begin(self, "result");

    key_string(self, "expected", mu_test_status_to_string(summary->expected));
    key_string(self, "status", mu_test_status_to_string(summary->status));
    key_string(self, "stage", mu_test_stage_to_string(summary->stage));
    if (summary->reason)
    {
        key_string(self, "reason", summary->reason);
    }
    if (summary->file)
    {
        key_string(self, "file", summary->file);
    }
    if (summary->line)
    {
        key_integer(self, "line", summary->line);
    }

    key_object_end(self);

    if (summary->backtrace)
    {
        MuBacktrace* frame;

        key_array_begin(self, "backtrace");
        for (frame = summary->backtrace; frame; frame = frame->up)
        {
            elem_object_begin(self);
            if (frame->file_name)
            {
                key_string(self, "binary_file", frame->file_name);
            }
            if (frame->func_name && *frame->func_name)
            {
                key_string(self, "function", frame->func_name);
            }
            if (frame->func_addr)
            {
                key_begin(self, "func_addr");
                print(self, "\"0x%lx\"", frame->func_addr);
                key_end(self);
            }
            if (frame->return_addr)
            {
                key_begin(self, "return_addr");
                print(self, "\"0x%lx\"", frame->return_addr);
                key_end(self);
            }
            elem_object_end(self);
        }
        key_array_end(self);
    }

    elem_object_end(self);
}

static
MuLogLevel
max_log_level(struct MuLogger* logger_)
{
    JsonLogger* logger = (JsonLogger*) logger_;

    return logger->loglevel;
}

static int
get_fd(JsonLogger* self)
{
    return self->fd;
}

static void
set_fd(JsonLogger* self, int fd)
{
    self->fd = fd;
    if (self->out)
        fclose(self->out);
    self->out = fdopen(dup(fd), "w");
}

static const char*
get_file(JsonLogger* self)
{
    return self->file;
}

static void
set_file(JsonLogger* self, const char* file)
{
    if (self->file)
        free(self->file);
    self->file = strdup(file);
    if (self->out)
        fclose(self->out);
    self->out = fopen(self->file, "w");
    self->fd = fileno(self->out);
}

static const char*
get_title(JsonLogger* self)
{
    return self->title;
}

static void
set_title(JsonLogger* self, const char* title)
{
    if (self->title)
        free(self->title);
    self->title = strdup(title);
}

static const char*
get_loglevel(JsonLogger* self)
{
    switch ((int) self->loglevel)
    {
    case -1:
        return "none";
    case MU_LEVEL_WARNING:
        return "warning";
    case MU_LEVEL_INFO:
        return "info";
    case MU_LEVEL_VERBOSE:
        return "verbose";
    case MU_LEVEL_DEBUG:
        return "debug";
    case MU_LEVEL_TRACE:
        return "trace";
    default:
        return "unknown";
    }
}

static void
set_loglevel(JsonLogger* self, const char* level)
{
    if (!strcmp(level, "warning"))
    {
        self->loglevel = MU_LEVEL_WARNING;
    }
    else if (!strcmp(level, "info"))
    {
        self->loglevel = MU_LEVEL_INFO;
    }
    else if (!strcmp(level, "verbose"))
    {
        self->loglevel = MU_LEVEL_VERBOSE;
    }
    else if (!strcmp(level, "debug"))
    {
        self->loglevel = MU_LEVEL_DEBUG;
    }
    else if (!strcmp(level, "trace"))
    {
        self->loglevel = MU_LEVEL_TRACE;
    }
    else if (!strcmp(level, "none"))
    {
        self->loglevel = -1;
    }
}

static bool
get_pretty(JsonLogger* self)
{
    return self->pretty;
}

static void
set_pretty(JsonLogger* self, bool value)
{
    self->pretty = value;
}

static void
destroy(MuLogger* _logger)
{
    JsonLogger* logger = (JsonLogger*) _logger;

    if (logger->out)
        fclose(logger->out);
    if (logger->file)
        free(logger->file);

    free(logger);
}

static MuOption jsonlogger_options[] =
{
    MU_OPTION("fd", MU_TYPE_INTEGER, get_fd, set_fd,
              "File descriptor to which results will be written"),
    MU_OPTION("file", MU_TYPE_STRING, get_file, set_file,
              "File to which results will be written"),
    MU_OPTION("title", MU_TYPE_STRING, get_title, set_title,
              "Title of the test results"),
    MU_OPTION("loglevel", MU_TYPE_STRING, get_loglevel, set_loglevel,
              "Maximum level of logged events which will be recorded"),
    MU_OPTION("pretty", MU_TYPE_BOOLEAN, get_pretty, set_pretty,
              "Output prettified JSON"),
    MU_OPTION_END
};

static JsonLogger jsonlogger =
{
    .base =
    {
        .enter = enter,
        .leave = leave,
        .library_enter = library_enter,
        .library_fail = library_fail,
        .library_leave = library_leave,
        .suite_enter = suite_enter,
        .suite_leave = suite_leave,
        .test_enter = test_enter,
        .test_log = test_log,
        .test_leave = test_leave,
        .max_log_level = max_log_level,
        .destroy = destroy,
        .options = jsonlogger_options
    },
    .fd = -1,
    .file = NULL,
    .out = NULL,
    .loglevel = MU_LEVEL_INFO,
    .pretty = false
};

static MuLogger*
create_jsonlogger()
{
    JsonLogger* logger = xmalloc(sizeof(JsonLogger));

    *logger = jsonlogger;

    mu_logger_set_option((MuLogger*) logger, "fd", fileno(stdout));
    mu_logger_set_option((MuLogger*) logger, "title", "Test Results");

    return (MuLogger*) logger;
}

static MuPlugin plugin =
{
    .version = MU_PLUGIN_API_1,
    .type = MU_PLUGIN_LOGGER,
    .name = "json",
    .author = "Brian Koropoff",
    .description = "Outputs test results as JSON",
    .create_logger = create_jsonlogger,
};

MU_PLUGIN_INIT
{
    return &plugin;
}
