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

#include <moonunit/loader.h>
#include <moonunit/private/util.h>
#include <moonunit/logger.h>
#include <moonunit/library.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void
mu_logger_set_option(MuLogger* logger, const char *name, ...)
{
    va_list ap;

    va_start(ap, name);

    mu_option_setv(logger->options, logger, name, ap);

    va_end(ap);
}

void 
mu_logger_set_option_string(MuLogger* logger, const char *name, const char *value)
{
    mu_option_set_string(logger->options, logger, name, value);
}

MuType
mu_logger_option_type(MuLogger* logger, const char *name)
{
    return mu_option_type(logger->options, name);
}

void
mu_logger_enter(MuLogger* logger)
{
    logger->enter(logger);
}
void
mu_logger_leave(MuLogger* logger)
{
    logger->leave(logger);
}

void
mu_logger_library_enter (struct MuLogger* logger, const char* path, MuLibrary* library)
{
    logger->library_enter(logger, path, library);
}

void
mu_logger_library_fail (struct MuLogger* logger, const char* reason)
{
    logger->library_fail(logger, reason);
}

void
mu_logger_library_leave (struct MuLogger* logger)
{
    logger->library_leave(logger);
}

void
mu_logger_suite_enter (struct MuLogger* logger, const char* name)
{
    logger->suite_enter(logger, name);
}

void
mu_logger_suite_leave (struct MuLogger* logger)
{
    logger->suite_leave(logger);
}

void
mu_logger_test_enter (struct MuLogger* logger, struct MuTest* test)
{
    logger->test_enter(logger, test);
}

void
mu_logger_test_log (struct MuLogger* logger, MuLogEvent const* event)
{
    logger->test_log(logger, event);
}

void
mu_logger_test_leave (struct MuLogger* logger, 
                     struct MuTest* test, struct MuTestResult* summary)
{
    logger->test_leave(logger, test, summary);
}

void
mu_logger_destroy(MuLogger* logger)
{
    logger->destroy(logger);
}
