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

#include <moonunit/test.h>
#include <moonunit/harness.h>
#include <moonunit/loader.h>
#include <moonunit/util.h>
#include <uipc/ipc.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

typedef struct
{
    MuTestToken base;
    MuTestStage current_stage;
    MuTest* current_test;
    uipc_handle* ipc_handle;
} UnixToken;

static UnixToken* current_token;

static uipc_typeinfo testsummary_info =
{
	1,
	{
		UIPC_POINTER(MuTestSummary, reason, NULL)
	}
};

static uipc_typeinfo logevent_info =
{
    2,
    {
        UIPC_POINTER(MuLogEvent, file, NULL),
        UIPC_POINTER(MuLogEvent, message, NULL)
    }
};

#define MSG_TYPE_RESULT 0
#define MSG_TYPE_EVENT 1

void unixtoken_event(MuTestToken* _token, const MuLogEvent* _event)
{
    UnixToken* token = (UnixToken*) _token;
    uipc_handle* ipc_handle = token->ipc_handle;

    if (!ipc_handle)
    {
        exit(0);
    }
    
    uipc_message* message = uipc_msg_new(ipc_handle, MSG_TYPE_EVENT, 2048);

    MuLogEvent* event = uipc_msg_alloc(message, sizeof(MuLogEvent));

    *event = *_event;

    if (event->file)
    {
        event->file = uipc_msg_alloc(message, strlen(_event->file) + 1);
		strcpy((char*) event->file, _event->file);
    }

    if (event->message)
    {
        event->message = uipc_msg_alloc(message, strlen(_event->message) + 1);
		strcpy((char*) event->message, _event->message);
    }

    event->stage = token->current_stage;

	uipc_msg_payload_set(message, event, &logevent_info);
	uipc_msg_send(message);
	uipc_msg_free(message);

    uipc_waitdone(ipc_handle, NULL);
}

void unixtoken_result(MuTestToken* _token, const MuTestSummary* _summary)
{	
    UnixToken* token = (UnixToken*) _token;
	uipc_handle* ipc_handle = token->ipc_handle;

    if (!ipc_handle)
    {
        exit(0);
    }

	uipc_message* message = uipc_msg_new(ipc_handle, MSG_TYPE_RESULT, 2048);
	
	MuTestSummary* summary = uipc_msg_alloc(message, sizeof(MuTestSummary));
	
	*summary = *_summary;
	
	if (summary->reason)
	{
		summary->reason = uipc_msg_alloc(message, strlen(_summary->reason) + 1);
		strcpy((char*) summary->reason, _summary->reason);
	}
	
	uipc_msg_payload_set(message, summary, &testsummary_info);
	uipc_msg_send(message);
	uipc_msg_free(message);

    uipc_waitdone(ipc_handle, NULL);
    uipc_disconnect(ipc_handle);
	
	exit(0);
}

static void
signal_handler(int sig)
{
	MuTestSummary summary;
	
	summary.result = MOON_RESULT_CRASH;
	summary.stage = current_token->current_stage;
	summary.reason = strdup(strsignal(sig));
	summary.line = 0;
	
	current_token->base.result((MuTestToken*) current_token, &summary);
}

static UnixToken*
unixtoken_new(MuTest* test)
{
    UnixToken* token = calloc(1, sizeof(UnixToken));

    Mu_TestToken_FillMethods((MuTestToken*) token);

    token->base.result = unixtoken_result;
    token->base.event = unixtoken_event;

    return token;
}

void unixharness_dispatch(MuHarness* _self, MuTest* test, MuTestSummary* summary, MuLogCallback cb, void* data)
{
	int sockets[2];
	pid_t pid;
    UnixToken* token = current_token = unixtoken_new(test);
	
	socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	
    // We must force a flush of all open output streams or the child
    // will end up flushing non-empty buffers on exit, resulting in
    // bizarre duplicate output
    
    fflush(NULL);

	if (!(pid = fork()))
	{
		MuTestThunk thunk;
		uipc_handle* ipc_test = uipc_connect(sockets[1]);

		close(sockets[0]);

		token->base.test = test;
        token->ipc_handle = ipc_test;
		
		signal(SIGSEGV, signal_handler);
		signal(SIGPIPE, signal_handler);
		signal(SIGFPE, signal_handler);
		signal(SIGABRT, signal_handler);
				
		token->current_stage = MOON_STAGE_SETUP;
		
		if ((thunk = Mu_Loader_FixtureSetup(test->loader, test->library, test->suite)))
			thunk((MuTestToken*) token);
			
		token->current_stage = MOON_STAGE_TEST;
		
		test->run((MuTestToken*) token);
		
		token->current_stage = MOON_STAGE_TEARDOWN;
		
		if ((thunk = Mu_Loader_FixtureTeardown(test->loader, test->library, test->suite)))
			thunk((MuTestToken*) token);
		
		token->base.method.success((MuTestToken*) token);
	
        uipc_waitdone(ipc_test, NULL);

        uipc_disconnect(ipc_test);

        close(sockets[1]);

		exit(0);
	}
	else
	{
		uipc_handle* ipc_harness = uipc_connect(sockets[0]);
		MuTestSummary *_summary;
		uipc_message* message = NULL;
		int status;
        UipcStatus uipc_result, uipc_result2;
        // FIXME: make configurable
        long timeout = 2000;
        long timeleft = timeout;
        bool done = false;	

		close(sockets[1]);
        
        while (!done)
        {	
    		uipc_result = uipc_waitread(ipc_harness, &message, &timeleft);

	    	if (uipc_result == UIPC_SUCCESS)
		    {
                switch (uipc_msg_get_type(message))
                {
                    case MSG_TYPE_RESULT:
                        _summary = uipc_msg_payload_get(message, &testsummary_info);
                        *summary = *_summary;
                        if (summary->reason)
                            summary->reason = strdup(_summary->reason);
                        done = true;
                        break;
                    case MSG_TYPE_EVENT:
                    {
                        MuLogEvent* event = uipc_msg_payload_get(message, &logevent_info);
                        cb(event, data);
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

        uipc_result2 = uipc_waitdone(ipc_harness, &timeleft);
        uipc_disconnect(ipc_harness);	
		close(sockets[0]);

        if (uipc_result == UIPC_TIMEOUT || uipc_result2 == UIPC_TIMEOUT)
        {
             kill(pid, SIGKILL);
        }

		waitpid(pid, &status, 0);

        if (!message)
		{
            // Timed out waiting for response
            if (uipc_result == UIPC_TIMEOUT || uipc_result2 == UIPC_TIMEOUT)
            {
                char* reason = format("Test timed out after %li milliseconds", timeout);

                summary->result = MOON_RESULT_TIMEOUT;
                summary->reason = reason;
                summary->stage = MOON_STAGE_UNKNOWN;
                summary->line = 0;
            }
			// Couldn't get message or an error occurred, try to figure out what happend
            else if (WIFSIGNALED(status))
			{
				summary->result = MOON_RESULT_CRASH;
				summary->stage = MOON_STAGE_UNKNOWN;
				summary->line = 0;
				
				if (WTERMSIG(status))
					summary->reason = strdup(strsignal(WTERMSIG(status)));
			}
            else
            {
                summary->result = MOON_RESULT_FAILURE;
				summary->stage = MOON_STAGE_UNKNOWN;
				summary->line = 0;
                summary->reason = strdup("Unexpected termination");
            }
		}
	}
}

pid_t unixharness_debug(MuHarness* _self, MuTest* test)
{
	int sockets[2];
	pid_t pid;
    UnixToken* token = current_token = unixtoken_new(test);
	
	socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	
	if (!(pid = fork()))
	{
		MuTestThunk thunk;

		close(sockets[0]);

		token->base.test = test;
        select(0, NULL, NULL, NULL, NULL);
		
		token->current_stage = MOON_STAGE_SETUP;
		
		if ((thunk = Mu_Loader_FixtureSetup(test->loader, test->library, test->suite)))
			thunk((MuTestToken*) test);
			
		token->current_stage = MOON_STAGE_TEST;
		
		test->run((MuTestToken*) token);
		
		token-> current_stage = MOON_STAGE_TEARDOWN;
		
		if ((thunk = Mu_Loader_FixtureTeardown(test->loader, test->library, test->suite)))
			thunk((MuTestToken*) token);
		
		token->base.method.success((MuTestToken*) token);
	
		exit(0);
	}
	else
	{
        return pid;
	}
}
  
void unixharness_cleanup (MuHarness* _self, MuTestSummary* summary)
{
	free((void*) summary->reason);
}

MuHarness mu_unixharness =
{
    .plugin = NULL,
	.dispatch = unixharness_dispatch,
    .debug = unixharness_debug,
	.cleanup = unixharness_cleanup
};
