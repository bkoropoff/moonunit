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
#include <uipc/ipc.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

static uipc_typeinfo testsummary_info =
{
	1,
	{
		UIPC_POINTER(MoonUnitTestSummary, reason, NULL)
	}
};

static MoonUnitTestStage current_stage;
static MoonUnitTest* current_test;

void unixharness_result(MoonUnitHarness* _self, MoonUnitTest* test, const MoonUnitTestSummary* _summary)
{	
	uipc_handle* ipc_handle = test->data;

    if (!ipc_handle)
    {
        exit(0);
    }

	uipc_message* message = uipc_msg_new(ipc_handle, 2048);
	
	MoonUnitTestSummary* summary = uipc_msg_alloc(message, sizeof(MoonUnitTestSummary));
	
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
	MoonUnitTestSummary summary;
	
	summary.result = MOON_RESULT_CRASH;
	summary.stage = current_stage;
	summary.reason = strdup(strsignal(sig));
	summary.line = 0;
	
	current_test->harness->result(current_test->harness, current_test, &summary);
}


void unixharness_dispatch(MoonUnitHarness* _self, MoonUnitTest* test, MoonUnitTestSummary* summary)
{
	int sockets[2];
	pid_t pid;
	
	socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	
    // We must force a flush of all open output streams or the child
    // will end up flushing non-empty buffers on exit, resulting in
    // bizarre duplicate output
    
    fflush(NULL);

	if (!(pid = fork()))
	{
		MoonUnitTestThunk thunk;
		uipc_handle* ipc_test = uipc_connect(sockets[1]);

		close(sockets[0]);

		current_test = test;
        
        test->harness = _self;
		test->data = ipc_test;
		
		signal(SIGSEGV, signal_handler);
		signal(SIGPIPE, signal_handler);
		signal(SIGFPE, signal_handler);
		signal(SIGABRT, signal_handler);
				
		current_stage = MOON_STAGE_SETUP;
		
		if ((thunk = test->loader->fixture_setup(test->loader, test->suite, test->library)))
			thunk(test);
			
		current_stage = MOON_STAGE_TEST;
		
		test->function(test);
		
		current_stage = MOON_STAGE_TEARDOWN;
		
		if ((thunk = test->loader->fixture_teardown(test->loader, test->suite, test->library)))
			thunk(test);
		
		test->methods->success(test);
	
        uipc_waitdone(ipc_test, NULL);

        uipc_disconnect(ipc_test);

        close(sockets[1]);

		exit(0);
	}
	else
	{
		uipc_handle* ipc_harness = uipc_connect(sockets[0]);
		MoonUnitTestSummary *_summary;
		uipc_message* message = NULL;
		int status;
        UipcStatus uipc_result, uipc_result2;
        // FIXME: make configurable
        long timeout = 2000;
        long timeleft = timeout;
	
		close(sockets[1]);
		
		uipc_result = uipc_waitread(ipc_harness, &message, &timeleft);

		if (uipc_result == UIPC_SUCCESS)
		{
			_summary = uipc_msg_payload_get(message, &testsummary_info);
			*summary = *_summary;
			if (summary->reason)
				summary->reason = strdup(_summary->reason);
			uipc_msg_free(message);
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
                char* reason;
                asprintf(&reason, "Test timed out after %li milliseconds", timeout);

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

pid_t unixharness_debug(MoonUnitHarness* _self, MoonUnitTest* test)
{
	int sockets[2];
	pid_t pid;
	
	socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	
	if (!(pid = fork()))
	{
		MoonUnitTestThunk thunk;

		close(sockets[0]);

		current_test = test;

		test->harness = _self;
		test->data = NULL;

        select(0, NULL, NULL, NULL, NULL);
		
		current_stage = MOON_STAGE_SETUP;
		
		if ((thunk = test->loader->fixture_setup(test->loader, test->suite, test->library)))
			thunk(test);
			
		current_stage = MOON_STAGE_TEST;
		
		test->function(test);
		
		current_stage = MOON_STAGE_TEARDOWN;
		
		if ((thunk = test->loader->fixture_teardown(test->loader, test->suite, test->library)))
			thunk(test);
		
		test->methods->success(test);
	
		exit(0);
	}
	else
	{
        return pid;
	}
}
  
void unixharness_cleanup (MoonUnitHarness* _self, MoonUnitTestSummary* summary)
{
	free((void*) summary->reason);
}

MoonUnitHarness mu_unixharness =
{
    .plugin = NULL,
	.result = unixharness_result,
	.dispatch = unixharness_dispatch,
    .debug = unixharness_debug,
	.cleanup = unixharness_cleanup
};
