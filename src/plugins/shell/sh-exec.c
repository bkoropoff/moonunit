/*
 * Copyright (c) Brian Koropoff
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

#include "sh-exec.h"

#include <string.h>

char* mu_sh_helper_path = LIBEXEC_PATH "/mu.sh";
int mu_sh_timeout = 10000;

void Mu_Sh_Exec(Process* handle, const char* script, const char* command)
{
    char template[] = "/tmp/mu_sh_XXXXXX";
    int fd = mkstemp(template);
    FILE* file = fdopen(fd, "w");
    char* argv[] = 
    {
        "bash",
        template,
        NULL
    };
    
    fprintf(file, "rm -f '%s'\n", template);
    fprintf(file, "MU_CMD_IN=%i\n", MU_SH_CMD_IN);
    fprintf(file, "MU_CMD_OUT=%i\n", MU_SH_CMD_OUT);
    fprintf(file, "source '%s'\n", mu_sh_helper_path);
    fprintf(file, "source '%s'\n", script);
    fprintf(file, "%s\n", command);

    fclose(file);

    Process_Open(handle, argv, 5,
                 PROCESS_CHANNEL_DEFAULT, /* Output to process stdin */
                 PROCESS_CHANNEL_DEFAULT,  /* Input process stdout */
                 PROCESS_CHANNEL_DEFAULT,  /* Input process stderr */
                 PROCESS_CHANNEL_OUT, /* Output to process command channel */
                 PROCESS_CHANNEL_IN); /* Input process command channel */
}

static char*
Mu_Sh_GetToken(char** ptokens)
{
    if (*ptokens)
    {
        char* tokens = *ptokens;
        char* ff = strchr(tokens, '\f');
        
        if (ff)
        {
            *ptokens = ff+1;
            *ff = '\0';
        }
        else
        {
            *ptokens = NULL;
        }

        return tokens;
    }
    else
    {
        return NULL;
    }
}

static MuTestStatus
Mu_Sh_StringToTestStatus(const char* str)
{
    if (!strcmp(str, "success"))
        return MU_STATUS_SUCCESS;
    else if (!strcmp(str, "failure"))
        return MU_STATUS_FAILURE;
    else if (!strcmp(str, "assertion"))
        return MU_STATUS_ASSERTION;
    else if (!strcmp(str, "crash"))
        return MU_STATUS_CRASH;
    else if (!strcmp(str, "timeout"))
        return MU_STATUS_TIMEOUT;
    else if (!strcmp(str, "exception"))
        return MU_STATUS_EXCEPTION;
    else if (!strcmp(str, "skipped"))
        return MU_STATUS_SKIPPED;
    else
        return MU_STATUS_FAILURE;
}

static MuTestStage
Mu_Sh_StringToTestStage(const char* str)
{
    if (!strcmp(str, "fixture setup"))
        return MU_STAGE_FIXTURE_SETUP;
    else if (!strcmp(str, "fixture teardown"))
        return MU_STAGE_FIXTURE_TEARDOWN;
    else if (!strcmp(str, "library setup"))
        return MU_STAGE_LIBRARY_SETUP;
    else if (!strcmp(str, "library teardown"))
        return MU_STAGE_LIBRARY_TEARDOWN;
    else if (!strcmp(str, "test"))
        return MU_STAGE_TEST;
    else
        return MU_STAGE_UNKNOWN;
}

static MuLogLevel
Mu_Sh_StringToLogLevel(const char* str)
{
    if (!strcmp(str, "warning"))
        return MU_LEVEL_WARNING;
    else if (!strcmp(str, "info"))
        return MU_LEVEL_INFO;
    else if (!strcmp(str, "verbose"))
        return MU_LEVEL_VERBOSE;
    else if (!strcmp(str, "debug"))
        return MU_LEVEL_DEBUG;
    else if (!strcmp(str, "trace"))
        return MU_LEVEL_TRACE;
    else
        return MU_LEVEL_INFO;
}



void
Mu_Sh_ProcessResult(MuTestResult* result, char** ptokens)
{
    char* status = Mu_Sh_GetToken(ptokens);
    char* stage = Mu_Sh_GetToken(ptokens);
    char* file = Mu_Sh_GetToken(ptokens);
    char* line = Mu_Sh_GetToken(ptokens);
    char* message = Mu_Sh_GetToken(ptokens);

    result->status = Mu_Sh_StringToTestStatus(status);
    result->stage = Mu_Sh_StringToTestStage(stage);
    result->reason = strdup(message);
    result->file = strdup(file);
    result->line = atoi(line);
}

void
Mu_Sh_ProcessEvent(MuLogCallback lcb, void* data, char** ptokens)
{
    MuLogEvent event;

    char* level = Mu_Sh_GetToken(ptokens);
    char* stage = Mu_Sh_GetToken(ptokens);
    char* file = Mu_Sh_GetToken(ptokens);
    char* line = Mu_Sh_GetToken(ptokens);
    char* message = Mu_Sh_GetToken(ptokens);

    event.level = Mu_Sh_StringToLogLevel(level);
    event.stage = Mu_Sh_StringToTestStage(stage);
    event.file = strdup(file);
    event.line = atoi(line);
    event.message = strdup(message);

    lcb(&event, data);
}

int
Mu_Sh_ProcessCommand(MuTestResult* result, ProcessTimeout* timeout, char* cmd, MuLogCallback lcb, void* data)
{
    char* tokens = cmd;
    char* operation = Mu_Sh_GetToken(&tokens);

    if (!strcmp(operation, "RESULT"))
    {
        Mu_Sh_ProcessResult(result, &tokens);
        return 1;
    }
    else if (!strcmp(operation, "EXPECT"))
    {
        result->expected = Mu_Sh_StringToTestStatus(Mu_Sh_GetToken(&tokens));
        return 0;
    }
    else if (!strcmp(operation, "LOG"))
    {
        Mu_Sh_ProcessEvent(lcb, data, &tokens);
        return 0;
    }
    else if (!strcmp(operation, "TIMEOUT"))
    {
        char* str = Mu_Sh_GetToken(&tokens);
        int ms;

        if (timeout)
        {
            ms = atoi(str);
            Process_GetTime(timeout, ms);
        }
        return 0;
    }
    else
    {
        return 0;
    }
}

struct MuTestResult*
Mu_Sh_Dispatch (const char* script, const char* suite, const char* name, const char* function, MuLogCallback lcb, void* data)
{
    Process handle;
    ProcessTimeout timeout;
    MuTestResult* result = calloc(1, sizeof(*result));
    int res;
    char* command;

    Process_GetTime(&timeout, mu_sh_timeout);

    command = format("mu_run_test '%s' '%s' '%s'", suite, name, function);

    Mu_Sh_Exec(&handle, script, command);

    free(command);

    while ((res = Process_Select(&handle, &timeout, 1, 4)) > 0)
    {
        if (Process_Channel_Ready(&handle, 4))
        {
            char* line;
            unsigned int len;

            if ((len = Process_Channel_ReadLine(&handle, 4, &line)))
            {
                line[len - 1] = '\0';
                if (Mu_Sh_ProcessCommand(result, &timeout, line, lcb, data))
                {
                    free(line);
                    break;
                }
                free(line);
            }
            else if (len == 0)
            {
                break;
            }
        }
    }
    
    if (res <= 0)
    {
        result->status = MU_STATUS_TIMEOUT;
        result->stage = MU_STAGE_UNKNOWN;
        result->reason = format("Test timed out");
    }
    else if (!result)
    {
        result->status = MU_STATUS_SUCCESS;
        result->expected = MU_STATUS_SUCCESS;
    }

    Process_Close(&handle);

    return result;
}