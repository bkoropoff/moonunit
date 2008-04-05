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

#include <moonunit/plugin.h>
#include <moonunit/logger.h>
#include <moonunit/harness.h>
#include <moonunit/test.h>
#include <moonunit/util.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct
{
    MuLogger base;
    int fd;
    char* file;
    FILE* out;
    MuTest* current_test;
    char* name;
} XmlLogger;

static void
enter(MuLogger* _self)
{
    XmlLogger* self = (XmlLogger*) _self;
    
    if (self->name)
        fprintf(self->out, "<libraries name=\"%s\">\n", self->name);
    else
        fprintf(self->out, "<libraries>\n");
}

static void
leave(MuLogger* _self)
{
    XmlLogger* self = (XmlLogger*) _self;
    
    fprintf(self->out, "</libraries>\n");
}

static void library_enter(MuLogger* _self, const char* name)
{
    XmlLogger* self = (XmlLogger*) _self;

    fprintf(self->out, "  <library name=\"%s\">\n", name);
}

static void library_leave(MuLogger* _self)
{
    XmlLogger* self = (XmlLogger*) _self;

	fprintf(self->out, "  </library>\n");
}

static void suite_enter(MuLogger* _self, const char* name)
{
    XmlLogger* self = (XmlLogger*) _self;
    
	fprintf(self->out, "    <suite name=\"%s\">\n", name);
}

static void suite_leave(MuLogger* _self)
{
    XmlLogger* self = (XmlLogger*) _self;
    
	fprintf(self->out, "    </suite>\n");
}

static void test_enter(MuLogger* _self, MuTest* test)
{
    XmlLogger* self = (XmlLogger*) _self;

    self->current_test = test;

    fprintf(self->out, "      <test name=\"%s\">\n", test->name);
}

static void test_log(MuLogger* _self, MuLogEvent* event)
{
    XmlLogger* self = (XmlLogger*) _self;
    const char* level_str = "unknown";

    switch (event->level)
    {
        case MU_LEVEL_WARNING:
            level_str = "warning"; break;
        case MU_LEVEL_INFO:
            level_str = "info"; break;
        case MU_LEVEL_VERBOSE:
            level_str = "verbose"; break;
        case MU_LEVEL_TRACE:
            level_str = "trace"; break;
    }
    fprintf(self->out, "        <event level=\"%s\" file=\"%s\" line=\"%u\" stage=\"%s\">\n",
            level_str, basename_pure(event->file), event->line,
            Mu_TestStageToString(event->stage));
    fprintf(self->out, "          <![CDATA[%s]]>\n", event->message);
    fprintf(self->out, "        </event>\n");
}

static void test_leave(MuLogger* _self, 
                       MuTest* test, MuTestResult* summary)
{
    XmlLogger* self = (XmlLogger*) _self;
    const char* stage;
    FILE* out = self->out;
    bool result = summary->status == summary->expected;
    const char* result_str;

    if (result)
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
        default:
            result_str = "fail";
            break;
        }
    }
        
    if (summary->status == MU_STATUS_SUCCESS)
	{
        fprintf(out, "        <result status=\"%s\"/>\n", result_str);
    }
    else
    {
        stage = Mu_TestStageToString(summary->stage);
        
        if (summary->reason)
        {
            fprintf(out, "        <result status=\"%s\" stage=\"%s\"", result_str, stage);
            if (summary->line)
                fprintf(out, " file=\"%s\" line=\"%u\"", basename_pure(test->file), summary->line);
            fprintf(out, ">\n");
            fprintf(out, "          <![CDATA[%s]]>\n", summary->reason);
            fprintf(out, "        </result>\n");
        }
        else
        {
            fprintf(out, "        <result status=\"fail\" stage=\"%s\"", stage);
            if (summary->line)
                fprintf(out, " file=\"%s\" line=\"%u\"", basename_pure(test->file), summary->line);
            fprintf(out, "/>\n");
        }
	}
    fprintf(out, "      </test>\n");
}

static void option_set(void* _self, const char* name, void* data)
{
    XmlLogger* self = (XmlLogger*) _self;

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
    else if (!strcmp(name, "name"))
    {
        if (self->name)
            free(self->name);
        self->name = strdup((char*) data);
    }
}                       

static const void* option_get(void* _self, const char* name)
{
    XmlLogger* self = (XmlLogger*) _self;

    if (!strcmp(name, "fd"))
    {
        return &self->fd;
    }
    else if (!strcmp(name, "file"))
    {
        return self->file;
    }
    else if (!strcmp(name, "name"))
    {
        return self->name;
    }
    else
    {
        return NULL;
    }
}

static MuType option_type(void* _self, const char* name)
{
    if (!strcmp(name, "fd"))
    {
        return MU_TYPE_INTEGER;
    }
    else if (!strcmp(name, "file"))
    {
        return MU_TYPE_STRING;
    }
    else if (!strcmp(name, "name"))
    {
        return MU_TYPE_STRING;
    }
    else
    {
        return MU_TYPE_UNKNOWN;
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

static XmlLogger xmllogger =
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
        .option =
        {
            .set = option_set,
            .get = option_get,
            .type = option_type
        }
    },
    .fd = -1,
    .file = NULL,
    .out = NULL,
    .name = NULL
};

static MuLogger*
create_xmllogger()
{
    XmlLogger* logger = malloc(sizeof(XmlLogger));

    *logger = xmllogger;

    Mu_Logger_SetOption((MuLogger*) logger, "fd", fileno(stdout));
   
    return (MuLogger*) logger;
}

static MuPlugin plugin =
{
    .name = "xml",
    .create_logger = create_xmllogger,
};

MU_PLUGIN_INIT
{
    return &plugin;
}
