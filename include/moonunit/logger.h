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

#ifndef __MU_LOGGER_H__
#define __MU_LOGGER_H__

#include <moonunit/option.h>
#include <moonunit/test.h>

typedef struct MuLogger
{
    struct MuPlugin* plugin;
    void (*enter) (struct MuLogger*);
    void (*leave) (struct MuLogger*);
    void (*library_enter) (struct MuLogger*, const char* path, struct MuLibrary* library);
    void (*library_fail) (struct MuLogger*, const char* reason);
    void (*library_leave) (struct MuLogger*);
    void (*suite_enter) (struct MuLogger*, const char*);
    void (*suite_leave) (struct MuLogger*);
    void (*test_enter) (struct MuLogger*, struct MuTest* test);
    void (*test_log) (struct MuLogger*, struct MuLogEvent const* event);
    void (*test_leave) (struct MuLogger*, 
                    struct MuTest*, struct MuTestResult*);
    void (*destroy) (struct MuLogger*);
    MuOption* options;
} MuLogger;

C_BEGIN_DECLS

void mu_logger_set_option(MuLogger* logger, const char *name, ...);
void mu_logger_set_option_string(MuLogger* logger, const char *name, const char *value);
MuType mu_logger_option_type(MuLogger* logger, const char *name);

void mu_logger_enter(struct MuLogger*);
void mu_logger_leave(struct MuLogger*);
void mu_logger_library_enter (struct MuLogger*, const char*, struct MuLibrary* library);
void mu_logger_library_fail (struct MuLogger*, const char*);
void mu_logger_library_leave (struct MuLogger*);
void mu_logger_suite_enter (struct MuLogger*, const char*);
void mu_logger_suite_leave (struct MuLogger*);
void mu_logger_test_enter (struct MuLogger*, struct MuTest* test);
void mu_logger_test_log (struct MuLogger*, struct MuLogEvent const* event);
void mu_logger_test_leave (struct MuLogger*, 
                          struct MuTest*, struct MuTestResult*);
void mu_logger_destroy(MuLogger* logger);

C_END_DECLS

#endif
