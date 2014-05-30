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

#define INDENT "  "
#define INDENT_MOONUNIT ""
#define INDENT_RUN INDENT_MOONUNIT INDENT
#define INDENT_LIBRARY INDENT_RUN INDENT
#define INDENT_SUITE INDENT_LIBRARY INDENT
#define INDENT_TEST INDENT_SUITE INDENT

typedef struct
{
    MuLogger base;
    int fd;
    char* file;
    FILE* out;
    MuTest* current_test;
    char* title;
    char* name;
    MuLogLevel loglevel;
} XmlLogger;

static void
output(FILE* out, char const* str)
{
    fputs(str, out);
}

static void
xml_escape(FILE* out, char const* str)
{
    for (; *str; str++)
    {
        if (*str <= 0x1f || *str == 0x7f)
        {
            fprintf(out, "&#%u;", (unsigned int) *str);
        }
        else switch (*str)
        {
        case '"':
            output(out, "&quot;");
            break;
        case '\'':
            output(out, "&apos;");
            break;
        case '&':
            output(out, "&amp;");
            break;
        case '<':
            output(out, "&lt;");
            break;
        case '>':
            output(out, "&gt;");
            break;
        default:
            fputc(*str, out);
        }
    }
}

static void
xml_escape_wrap(FILE* out, char const* prefix, char const* str, char const* suffix)
{
    output(out, prefix);
    xml_escape(out, str);
    output(out, suffix);
}

static void
enter(MuLogger* _self)
{
    XmlLogger* self = (XmlLogger*) _self;
    
    output(self->out, INDENT_MOONUNIT "<moonunit");
    if (self->title)
    {
        xml_escape_wrap(self->out, " title=\"", self->title, "\"");
    }

    output(self->out, ">\n");

    output(self->out, INDENT_RUN "<run");

    if (self->name)
    {
        xml_escape_wrap(self->out, " name=\"", self->name, "\"");
    }

    xml_escape_wrap(self->out, " cpu=\"", HOST_CPU, "\"");
    xml_escape_wrap(self->out, " vendor=\"", HOST_VENDOR, "\"");
    xml_escape_wrap(self->out, " os=\"", HOST_OS, "\">\n");
}

static void
leave(MuLogger* _self)
{
    XmlLogger* self = (XmlLogger*) _self;

    output(self->out, INDENT_RUN "</run>\n" INDENT_MOONUNIT "</moonunit>\n");
}

static void library_enter(MuLogger* _self, const char* path, MuLibrary* library)
{
    XmlLogger* self = (XmlLogger*) _self;

    xml_escape_wrap(self->out, INDENT_LIBRARY "<library file=\"", basename_pure(path), "\"");
    if (library)
    {
        xml_escape_wrap(self->out, " name=\"", mu_library_name(library), "\"");
    }
    output(self->out, ">\n");
}

static void library_fail(MuLogger* _self, const char* reason)
{
    XmlLogger* self = (XmlLogger*) _self;

    xml_escape_wrap(self->out, INDENT_LIBRARY INDENT "<abort reason=\"", reason, "\"/>\n");
}

static void library_leave(MuLogger* _self)
{
    XmlLogger* self = (XmlLogger*) _self;

	output(self->out, INDENT_LIBRARY "</library>\n");
}

static void suite_enter(MuLogger* _self, const char* name)
{
    XmlLogger* self = (XmlLogger*) _self;
    
	xml_escape_wrap(self->out, INDENT_SUITE "<suite name=\"", name, "\">\n");
}

static void suite_leave(MuLogger* _self)
{
    XmlLogger* self = (XmlLogger*) _self;
    
	output(self->out, INDENT_SUITE "</suite>\n");
}

static void test_enter(MuLogger* _self, MuTest* test)
{
    XmlLogger* self = (XmlLogger*) _self;

    self->current_test = test;

    xml_escape_wrap(self->out, INDENT_TEST "<test name=\"", mu_test_name(test), "\">\n");
}

static void test_log(MuLogger* _self, MuLogEvent const* event)
{
    XmlLogger* self = (XmlLogger*) _self;
    const char* level_str = "unknown";

    if (event->level > self->loglevel)
    {
        return;
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
    fprintf(self->out, INDENT_TEST INDENT "<event level=\"%s\"", level_str);

    fprintf(self->out, " stage=\"%s\"", mu_test_stage_to_string(event->stage));
    
    if (event->file)
    {
        xml_escape_wrap(self->out, " file=\"", basename_pure(event->file), "\"");
    }

    if (event->line)
        fprintf(self->out, " line=\"%u\"", event->line);

    xml_escape_wrap(self->out, ">", event->message, "</event>\n");
}

static void test_leave(MuLogger* _self, 
                       MuTest* test, MuTestResult* summary)
{
    XmlLogger* self = (XmlLogger*) _self;
    const char* stage;
    FILE* out = self->out;
    bool result = summary->status == summary->expected;
    const char* result_str;

    if (summary->status == MU_STATUS_DEBUG)
    {
        result_str = "debug";
    }
    else if (result)
    {
        switch (summary->status)
        {		
        case MU_STATUS_SUCCESS:
            result_str = "pass";
            break;
        case MU_STATUS_SKIPPED:
            result_str = "skip";
            break;
        default:
            result_str = "xfail";
            break;
        }
    }
    else
    {
        switch (summary->status)
        {		
        case MU_STATUS_SUCCESS:
            result_str = "xpass";
            break;
        case MU_STATUS_SKIPPED:
            result_str = "skip";
            break;
        default:
            result_str = "fail";
            break;
        }
    }
        
    if (summary->status == MU_STATUS_SUCCESS)
	{
        fprintf(out, INDENT_TEST INDENT "<result status=\"%s\"/>\n", result_str);
    }
    else
    {
        stage = mu_test_stage_to_string(summary->stage);
        
        if (summary->reason)
        {
            fprintf(out, INDENT_TEST INDENT "<result status=\"%s\" stage=\"%s\"", result_str, stage);
            if (summary->file)
            {
                xml_escape_wrap(out, " file=\"", basename_pure(summary->file), "\"");
            }
            if (summary->line)
                fprintf(out, " line=\"%i\"", summary->line);

            output(out, ">");
            xml_escape(out, summary->reason);
            output(out, "</result>\n");
        }
        else
        {
            fprintf(out, INDENT_TEST INDENT "<result status=\"fail\" stage=\"%s\"", stage);
            if (summary->file)
            {
                xml_escape_wrap(out, " file=\"", basename_pure(summary->file), "\"");
            }
            if (summary->line)
                fprintf(out, " line=\"%i\"", summary->line);
            output(out, "/>\n");
        }
	}

    if (summary->backtrace)
    {
        MuBacktrace* frame;
        fprintf(out, INDENT_TEST INDENT "<backtrace>\n");
        for (frame = summary->backtrace; frame; frame = frame->up)
        {
            fprintf(out, INDENT_TEST INDENT INDENT "<frame");
            if (frame->file_name)
            {
                xml_escape_wrap(out, " binary_file=\"", frame->file_name, "\"");
            }
            if (frame->func_name)
            {
                xml_escape_wrap(out, " function=\"", frame->func_name, "\"");
            }
            if (frame->func_addr)
            {
                fprintf(out, " func_addr=\"%lx\"", frame->func_addr);
            }
            if (frame->return_addr)
            {
                fprintf(out, " return_addr=\"%lx\"", frame->return_addr);
            }
            output(out, "/>\n");
        }
        output(out, INDENT_TEST " 　</backtrace>\n");
    }

    output(out, INDENT_TEST "</test>\n");
}

static int
get_fd(XmlLogger* self)
{
    return self->fd;
}

static void
set_fd(XmlLogger* self, int fd)
{
    self->fd = fd;
    if (self->out)
        fclose(self->out);
    self->out = fdopen(dup(fd), "w");  
}

static const char*
get_file(XmlLogger* self)
{
    return self->file;
}

static void
set_file(XmlLogger* self, const char* file)
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
get_name(XmlLogger* self)
{
    return self->name;
}

static void
set_name(XmlLogger* self, const char* name)
{
    if (self->name)
        free(self->name);
    self->name = strdup(name);
}

static const char*
get_title(XmlLogger* self)
{
    return self->title;
}

static void
set_title(XmlLogger* self, const char* title)
{
    if (self->title)
        free(self->title);
    self->title = strdup(title);
}

static const char*
get_loglevel(XmlLogger* self)
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
set_loglevel(XmlLogger* self, const char* level)
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

static void
destroy(MuLogger* _logger)
{
    XmlLogger* logger = (XmlLogger*) _logger;
    
    if (logger->out)
        fclose(logger->out);
    if (logger->name)
        free(logger->name);
    if (logger->file)
        free(logger->file);

    free(logger);
}

static MuOption xmllogger_options[] =
{
    MU_OPTION("fd", MU_TYPE_INTEGER, get_fd, set_fd,
              "File descriptor to which results will be written"),
    MU_OPTION("file", MU_TYPE_STRING, get_file, set_file,
              "File to which results will be written"),
    MU_OPTION("name", MU_TYPE_STRING, get_name, set_name,
              "Value of the name attribute on the <run> node"),
    MU_OPTION("title", MU_TYPE_STRING, get_title, set_title,
              "Value of the title attribute on the <moonunit> node"),
    MU_OPTION("loglevel", MU_TYPE_STRING, get_loglevel, set_loglevel,
              "Maximum level of logged events which will be recorded"),
    MU_OPTION_END
};

static XmlLogger xmllogger =
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
        .destroy = destroy,
        .options = xmllogger_options
    },
    .fd = -1,
    .file = NULL,
    .out = NULL,
    .name = NULL,
    .loglevel = MU_LEVEL_INFO
};

static MuLogger*
create_xmllogger()
{
    XmlLogger* logger = malloc(sizeof(XmlLogger));

    *logger = xmllogger;

    mu_logger_set_option((MuLogger*) logger, "fd", fileno(stdout));
    mu_logger_set_option((MuLogger*) logger, "title", "Test Results");
   
    return (MuLogger*) logger;
}

static MuPlugin plugin =
{
    .version = MU_PLUGIN_API_1,
    .type = MU_PLUGIN_LOGGER,
    .name = "xml",
    .author = "Brian Koropoff",
    .description = "Outputs test results as XML",
    .create_logger = create_xmllogger,
};

MU_PLUGIN_INIT
{
    return &plugin;
}
