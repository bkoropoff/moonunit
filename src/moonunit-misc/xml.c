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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct
{
    MoonUnitLogger base;
    int fd;
    FILE* out;
} XmlLogger;

static void library_enter(MoonUnitLogger* _self, const char* name)
{
    XmlLogger* self = (XmlLogger*) _self;

    fprintf(self->out, "<library name=\"%s\">\n", name);
}

static void library_leave(MoonUnitLogger* _self)
{
    XmlLogger* self = (XmlLogger*) _self;

	fprintf(self->out, "</library>\n");
}

static void suite_enter(MoonUnitLogger* _self, const char* name)
{
    XmlLogger* self = (XmlLogger*) _self;
    
	fprintf(self->out, "  <suite name=\"%s\">\n", name);
}

static void suite_leave(MoonUnitLogger* _self)
{
    XmlLogger* self = (XmlLogger*) _self;
    
	fprintf(self->out, "  </suite>\n");
}

static void result(MoonUnitLogger* _self, 
                   MoonUnitTest* test, MoonUnitTestSummary* summary)
{
    XmlLogger* self = (XmlLogger*) _self;
    const char* stage;
    FILE* out = self->out;
   
	switch (summary->result)
	{
		case MOON_RESULT_SUCCESS:
            fprintf(out, "    <test name=\"%s\" result=\"pass\"/>\n", test->name);
			break;
		case MOON_RESULT_FAILURE:
		case MOON_RESULT_ASSERTION:
		case MOON_RESULT_CRASH:
			stage = Mu_TestStageToString(summary->stage);

            if (summary->reason)
            {
                fprintf(out, "    <test name=\"%s\" result=\"fail\" stage=\"%s\">\n",
                       test->name, stage);
                fprintf(out, "      <![CDATA[%s]]>\n", summary->reason);
                fprintf(out, "    </test>\n");
            }
            else
            {
                fprintf(out, "    <test name=\"%s\" result=\"fail\" stage=\"%s\"/>\n",
                       test->name, stage);
            }
	}
}

static void option_set(void* _self, const char* name, void* data)
{
    XmlLogger* self = (XmlLogger*) _self;

    if (!strcmp(name, "fd"))
    {
        self->fd = dup(*(int*) data);
        self->out = fdopen(self->fd, "w");
    }
}                       

static const void* option_get(void* _self, const char* name)
{
    XmlLogger* self = (XmlLogger*) _self;

    if (!strcmp(name, "fd"))
    {
        return &self->fd;
    }
    else
    {
        return NULL;
    }
}

static MoonUnitType option_type(void* _self, const char* name)
{
    if (!strcmp(name, "fd"))
    {
        return MU_INTEGER;
    }
    else
    {
        return MU_UNKNOWN_TYPE;
    }
}

static XmlLogger xmllogger =
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
            .set = option_set,
            .get = option_get,
            .type = option_type
        }
    },
    .fd = -1
};

static MoonUnitLogger*
create_xmllogger()
{
    XmlLogger* logger = malloc(sizeof(XmlLogger));

    *logger = xmllogger;

    return (MoonUnitLogger*) logger;
}

static void
destroy_xmllogger(MoonUnitLogger* _logger)
{
    XmlLogger* logger = (XmlLogger*) _logger;

    fclose(logger->out);
    free(logger);
}


static MoonUnitPlugin plugin =
{
    .name = "xml",
    .create_loader = NULL,
    .create_harness = NULL,
    .create_runner = NULL,
    .create_logger = create_xmllogger,
    .destroy_logger = destroy_xmllogger
};

MU_PLUGIN_INIT
{
    return &plugin;
}
