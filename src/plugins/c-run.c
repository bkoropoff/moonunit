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

#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#    include <config.h>
#endif

#include <moonunit/test.h>
#include <moonunit/loader.h>
#include <moonunit/util.h>
#include <moonunit/interface.h>
#include <uipc/ipc.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>

#include "backtrace.h"
#include "c-token.h"

#ifdef CPLUSPLUS_ENABLED
#    include "cplusplus.h"
#endif

static long default_timeout;
static CToken* current_token;

typedef struct
{
    MuTestStatus expect_status;
} ExpectMsg;

typedef struct
{
    long timeout;
} TimeoutMsg;

static uipc_typeinfo backtrace_info =
{
    .name = "MuBacktrace",
    .size = sizeof(MuBacktrace),
    .members =
    {
        UIPC_STRING(MuBacktrace, file_name),
        UIPC_STRING(MuBacktrace, func_name),
        UIPC_POINTER(MuBacktrace, up, &backtrace_info),
        UIPC_END
    }
};

static uipc_typeinfo testresult_info =
{
    .name = "MuTestResult",
    .size = sizeof(MuTestResult),
    .members =
    {
        UIPC_STRING(MuTestResult, file),
        UIPC_STRING(MuTestResult, reason),
        UIPC_POINTER(MuTestResult, backtrace, &backtrace_info),
        UIPC_END
    }
};

static uipc_typeinfo logevent_info =
{
    .size = sizeof(MuLogEvent),
    .members =
    {
        UIPC_STRING(MuLogEvent, file),
        UIPC_STRING(MuLogEvent, message),
        UIPC_END
    }
};


static uipc_typeinfo timeout_info =
{
    .size = sizeof(TimeoutMsg),
    .members =
    {
	UIPC_END
    }
};


static uipc_typeinfo expect_info =
{
    .size = sizeof(ExpectMsg),
    .members =
    {
	UIPC_END
    }
};

#define MSG_TYPE_RESULT 0
#define MSG_TYPE_EVENT 1
#define MSG_TYPE_TIMEOUT 2
#define MSG_TYPE_EXPECT 3

static MuTestToken*
ctoken_current(void* data)
{
    return (MuTestToken*) data;
}

void
ctoken_event(MuTestToken* _token, const MuLogEvent* event)
{
    CToken* token = (CToken*) _token;
    uipc_handle* ipc_handle = token->ipc_handle;

    if (!ipc_handle)
    {
        exit(0);
    }

    pthread_mutex_lock(&token->lock);

    ((MuLogEvent*) event)->stage = token->current_stage;    

    uipc_message* message = uipc_msg_new(MSG_TYPE_EVENT);
    uipc_msg_set_payload(message, event, &logevent_info);
    uipc_send(ipc_handle, message, NULL);
    uipc_msg_free(message);

    pthread_mutex_unlock(&token->lock);
}

static void ctoken_free(CToken* token);

void ctoken_result(MuTestToken* _token, const MuTestResult* summary)
{    
    CToken* token = (CToken*) _token;
    uipc_handle* ipc_handle = token->ipc_handle;

    pthread_mutex_lock(&token->lock);
    
    if (!ipc_handle)
    {
        goto done;
    }

    ((MuTestResult*) summary)->stage = token->current_stage;
    uipc_message* message = uipc_msg_new(MSG_TYPE_RESULT);
    uipc_msg_set_payload(message, summary, &testresult_info);
    uipc_send(ipc_handle, message, NULL);
    uipc_msg_free(message);

done:
    if (ipc_handle)
        uipc_detach(ipc_handle);
    ctoken_free(token); 

    exit(0);

    pthread_mutex_unlock(&token->lock);
}

void
ctoken_meta(MuTestToken* _token, MuTestMeta type, ...)
{
    CToken* token = (CToken*) _token;
    va_list ap;

    pthread_mutex_lock(&token->lock);
    
    va_start(ap, type);

    switch (type)
    {
    case MU_META_EXPECT:
    {
        uipc_handle* ipc_handle = token->ipc_handle;
        ExpectMsg msg = { va_arg(ap, MuTestStatus) };
	
        if (!ipc_handle)
            return;
        
        uipc_message* message = uipc_msg_new(MSG_TYPE_EXPECT);
        uipc_msg_set_payload(message, &msg, &expect_info);
        uipc_send(ipc_handle, message, NULL);
        uipc_msg_free(message);
        break;
    }
    case MU_META_TIMEOUT:
    {
        uipc_handle* ipc_handle = token->ipc_handle;
        TimeoutMsg msg = { va_arg(ap, long) };
	
        if (!ipc_handle)
            return;
        
        uipc_message* message = uipc_msg_new(MSG_TYPE_TIMEOUT);
        uipc_msg_set_payload(message, &msg, &timeout_info);
        uipc_send(ipc_handle, message, NULL);
        uipc_msg_free(message);
        break;
    }
    }

    va_end(ap);

    pthread_mutex_unlock(&token->lock);
}

static char*
signal_description(int sig)
{
#ifdef HAVE_STRSIGNAL
    return strdup(strsignal(sig));
#else
    return format("Signal %i", sig);
#endif
}

static void
signal_handler(int sig)
{
    if (getpid() == current_token->child)
    {
        MuTestResult summary;
    
        summary.status = MU_STATUS_CRASH;
        summary.expected = current_token->expected;
        summary.stage = current_token->current_stage;
        summary.reason = signal_description(sig);
        summary.file = NULL;
        summary.line = 0;
        summary.backtrace = get_backtrace(1);
    
        current_token->base.result((MuTestToken*) current_token, &summary);
    }
    else
    {
        signal(sig, SIG_DFL);
        raise(sig);
    }
}

static CToken*
ctoken_new(MuTest* test)
{
    CToken* token = calloc(1, sizeof(CToken));

    token->base.meta = ctoken_meta;
    token->base.result = ctoken_result;
    token->base.event = ctoken_event;
    token->expected = MU_STATUS_SUCCESS;
    pthread_mutex_init(&token->lock, NULL);

    return token;
}

static void
ctoken_free(CToken* token)
{
    pthread_mutex_destroy(&token->lock);
    free(token);
}

#ifdef CPLUSPLUS_ENABLED
#   define INVOKE(thunk, token) (cplusplus_trampoline((thunk), (MuTestToken*) (token)))
#else
#   define INVOKE(thunk, token) ((thunk)((MuTestToken*) (token)))
#endif

MuTestResult*
cloader_dispatch(MuLoader* _self, MuTest* test, MuLogCallback cb, void* data)
{
    int sockets[2];
    pid_t pid;
    CToken* token = current_token = ctoken_new(test);
    
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    
    // We must force a flush of all open output streams or the child
    // will end up flushing non-empty buffers on exit, resulting in
    // bizarre duplicate output
    
    fflush(NULL);
    
    if (!(pid = fork()))
    {
        MuTestThunk thunk;
        uipc_handle* ipc_test = uipc_attach(sockets[1]);
        token->child = getpid();
        
        close(sockets[0]);
        
        token->base.test = test;
        token->ipc_handle = ipc_test;
    
        Mu_Interface_SetCurrentTokenCallback(ctoken_current, token);
        
        signal(SIGSEGV, signal_handler);
        signal(SIGPIPE, signal_handler);
        signal(SIGFPE, signal_handler);
        signal(SIGABRT, signal_handler);
    
        token->current_stage = MU_STAGE_SETUP;
    
        if ((thunk = Mu_Loader_FixtureSetup(test->loader, test->library, test->suite)))
            INVOKE(thunk, token);
        
        token->current_stage = MU_STAGE_TEST;
        
        INVOKE(test->run, token);
        
        token->current_stage = MU_STAGE_TEARDOWN;
        
        if ((thunk = Mu_Loader_FixtureTeardown(test->loader, test->library, test->suite)))
            INVOKE(thunk, token);
        
        Mu_Test_Success((MuTestToken*) token);
    
        close(sockets[1]);
        
        exit(0);
    }
    else
    {
        uipc_handle* ipc_loader = uipc_attach(sockets[0]);
        MuTestResult *summary = NULL;
        uipc_message* message = NULL;
        int status;
        uipc_status uipc_result;
        long timeleft = default_timeout;
        bool done = false;    
        
        close(sockets[1]);
        
        while (!done)
        {    
            uipc_result = uipc_recv(ipc_loader, &message, &timeleft);
            
            if (uipc_result == UIPC_SUCCESS)
            {
                switch (uipc_msg_get_type(message))
                {
                case MSG_TYPE_RESULT:
                    summary = uipc_msg_get_payload(message, &testresult_info);
                    done = true;
                    break;
                case MSG_TYPE_EVENT:
                {
                    MuLogEvent* event = uipc_msg_get_payload(message, &logevent_info);
                    cb(event, data);
                    uipc_msg_free_payload(event, &logevent_info);
                    uipc_msg_free(message);
                    message = NULL;
                    break;
                } 
                case MSG_TYPE_EXPECT:
                {
                    ExpectMsg* msg = uipc_msg_get_payload(message, &expect_info);
                    token->expected = msg->expect_status;
                    uipc_msg_free_payload(msg, &expect_info);
                    uipc_msg_free(message);
                    message = NULL;
                    break;
                }
                case MSG_TYPE_TIMEOUT:
                {
                    TimeoutMsg* msg = uipc_msg_get_payload(message, &timeout_info);
                    timeleft = msg->timeout;
                    uipc_msg_free_payload(msg, &timeout_info);
                    uipc_msg_free(message);
                    message = NULL;
                    break;
                }
                }
            }
            else
            {
                done = true;
            }
        }
        
        uipc_detach(ipc_loader); 
        close(sockets[0]);
        
        if (uipc_result == UIPC_TIMEOUT)
        {
            kill(pid, SIGKILL);
        }
        
        waitpid(pid, &status, 0);
        
        if (!summary)
        {
            summary = calloc(1, sizeof(MuTestResult));
            // Timed out waiting for response
            if (uipc_result == UIPC_TIMEOUT)
            {
                char* reason = format("Test timed out after %li milliseconds", default_timeout);
                
                summary->expected = token->expected;
                summary->status = MU_STATUS_TIMEOUT;
                summary->reason = reason;
                summary->stage = MU_STAGE_UNKNOWN;
                summary->line = 0;
            }
            // Couldn't get message or an error occurred, try to figure out what happend
            else if (WIFSIGNALED(status))
            {
		summary->expected = token->expected;
                summary->status = MU_STATUS_CRASH;
                summary->stage = MU_STAGE_UNKNOWN;
                summary->line = 0;
        
                if (WTERMSIG(status))
                    summary->reason = signal_description(WTERMSIG(status));
            }
            else
            {
                summary->status = MU_STATUS_FAILURE;
                summary->stage = MU_STAGE_UNKNOWN;
                summary->line = 0;
                summary->reason = strdup("Unexpected termination");
            }
        }
        else
        {
            summary->expected = token->expected;
        }

        if (message)
            uipc_msg_free(message);

        ctoken_free(token);

        return summary;
    }
}

void
cloader_free_result(MuLoader* _self, MuTestResult* result)
{
    uipc_msg_free_payload(result, &testresult_info);
}

pid_t cloader_debug(MuLoader* _self, MuTest* test)
{
    int sockets[2];
    pid_t pid;
    CToken* token = current_token = ctoken_new(test);
    
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    
    if (!(pid = fork()))
    {
        MuTestThunk thunk;

        close(sockets[0]);

        token->base.test = test;
        Mu_Interface_SetCurrentTokenCallback(ctoken_current, token);

        select(0, NULL, NULL, NULL, NULL);
        
        token->current_stage = MU_STAGE_SETUP;
        
        if ((thunk = Mu_Loader_FixtureSetup(test->loader, test->library, test->suite)))
            thunk((MuTestToken*) test);
            
        token->current_stage = MU_STAGE_TEST;
        
        test->run((MuTestToken*) token);
        
        token-> current_stage = MU_STAGE_TEARDOWN;
        
        if ((thunk = Mu_Loader_FixtureTeardown(test->loader, test->library, test->suite)))
            thunk((MuTestToken*) token);
        
        Mu_Test_Success((MuTestToken*) token);
    
        exit(0);
    }
    else
    {
        return pid;
    }
}
  
static void
timeout_set(MuLoader* self, int timeout)
{
    default_timeout = timeout;
}

static int
timeout_get(MuLoader* self)
{
    return default_timeout;
}

MuOption cloader_options[] =
{
    MU_OPTION("timeout", MU_TYPE_INTEGER, timeout_get, timeout_set,
              "Time in milliseconds before tests automatically "
              "fail and are forcefully terminated"),
    MU_OPTION_END
};

