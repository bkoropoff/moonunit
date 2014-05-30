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
#include <moonunit/private/util.h>
#include <moonunit/test.h>
#include <moonunit/library.h>

#include "multilog.h"

typedef struct
{
    MuLogger base;

    array* loggers;
} MultiLogger;

static void
enter(MuLogger* _self)
{
    MultiLogger* self = (MultiLogger*) _self;
    unsigned int index;

    for (index = 0; index < array_size(self->loggers); index++)
    {
        mu_logger_enter(self->loggers[index]);
    }
}

static void
leave(MuLogger* _self)
{
    MultiLogger* self = (MultiLogger*) _self;
    unsigned int index;

    for (index = 0; index < array_size(self->loggers); index++)
    {
        mu_logger_leave(self->loggers[index]);
    }
}

static void
library_enter(MuLogger* _self, const char* path, MuLibrary* library)
{
    MultiLogger* self = (MultiLogger*) _self;
    unsigned int index;

    for (index = 0; index < array_size(self->loggers); index++)
    {
        mu_logger_library_enter(self->loggers[index], path, library);
    }
}

static void
library_leave(MuLogger* _self)
{
    MultiLogger* self = (MultiLogger*) _self;
    unsigned int index;

    for (index = 0; index < array_size(self->loggers); index++)
    {
        mu_logger_library_leave(self->loggers[index]);
    }
}

static void
suite_enter(MuLogger* _self, const char* name)
{
    MultiLogger* self = (MultiLogger*) _self;
    unsigned int index;

    for (index = 0; index < array_size(self->loggers); index++)
    {
        mu_logger_suite_enter(self->loggers[index], name);
    }
}

static void
suite_leave(MuLogger* _self)
{
    MultiLogger* self = (MultiLogger*) _self;
    unsigned int index;

    for (index = 0; index < array_size(self->loggers); index++)
    {
        mu_logger_suite_leave(self->loggers[index]);
    }
}

static void
test_enter(MuLogger* _self, MuTest* test)
{
    MultiLogger* self = (MultiLogger*) _self;
    unsigned int index;

    for (index = 0; index < array_size(self->loggers); index++)
    {
        mu_logger_test_enter(self->loggers[index], test);
    }
}

static void
test_log(MuLogger* _self, MuLogEvent const* event)
{
    MultiLogger* self = (MultiLogger*) _self;
    unsigned int index;

    for (index = 0; index < array_size(self->loggers); index++)
    {
        mu_logger_test_log(self->loggers[index], event);
    }
}

static void
test_leave(MuLogger* _self, MuTest* test, MuTestResult* summary)
{
    MultiLogger* self = (MultiLogger*) _self;
    unsigned int index;

    for (index = 0; index < array_size(self->loggers); index++)
    {
        mu_logger_test_leave(self->loggers[index], test, summary);
    }
}

static void
destroy(MuLogger* _self)
{
    MultiLogger* self = (MultiLogger*) _self;
    unsigned int index;

    for (index = 0; index < array_size(self->loggers); index++)
    {
        mu_logger_destroy(self->loggers[index]);
    }

    array_free(self->loggers);
    free(self);
}

static MultiLogger multilogger =
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
        .options = NULL 
    },
    .loggers = NULL
};

MuLogger*
create_multilogger(array* loggers)
{
    MultiLogger* logger = malloc(sizeof(MultiLogger));

    *logger = multilogger;
    
    logger->loggers = loggers;

    return (MuLogger*) logger;
}
