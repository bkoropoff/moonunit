#define _GNU_SOURCE

#include <moonunit/test.h>
#include <moonunit/harness.h>
#include <moonunit/loader.h>
#include <urpc/rpc.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

static urpc_typeinfo testsummary_info =
{
	1,
	{
		URPC_POINTER(MoonUnitTestSummary, reason, NULL)
	}
};

static MoonUnitTestStage current_stage;
static MoonUnitTest* current_test;

void unixharness_result(MoonUnitTest* test, const MoonUnitTestSummary* _summary)
{	
	urpc_handle* rpc_handle = test->data;

	urpc_message* message = urpc_msg_new(rpc_handle, 2048);
	
	MoonUnitTestSummary* summary = urpc_msg_alloc(message, sizeof(MoonUnitTestSummary));
	
	*summary = *_summary;
	
	if (summary->reason)
	{
		summary->reason = urpc_msg_alloc(message, strlen(_summary->reason) + 1);
		strcpy((char*) summary->reason, _summary->reason);
	}
	
	urpc_msg_payload_set(message, summary, &testsummary_info);
	urpc_msg_send(message);
	urpc_msg_free(message);
	urpc_process(rpc_handle);
	urpc_process(rpc_handle);
	
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
	
	unixharness_result(current_test, &summary);
}


void unixharness_dispatch(MoonUnitTest* test, MoonUnitTestSummary* summary)
{
	int sockets[2];
	pid_t pid;
	
	socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	
	if (!(pid = fork()))
	{
		MoonUnitTestThunk thunk;
		urpc_handle* rpc_test = urpc_connect(sockets[1]);

		close(sockets[0]);

		current_test = test;

		test->harness = &mu_unixharness;
		test->data = rpc_test;
		
		signal(SIGSEGV, signal_handler);
		signal(SIGPIPE, signal_handler);
		signal(SIGFPE, signal_handler);
		signal(SIGABRT, signal_handler);
				
		current_stage = MOON_STAGE_SETUP;
		
		if ((thunk = test->loader->fixture_setup(test->suite, test->library)))
			thunk(test);
			
		current_stage = MOON_STAGE_TEST;
		
		test->function(test);
		
		current_stage = MOON_STAGE_TEARDOWN;
		
		if ((thunk = test->loader->fixture_teardown(test->suite, test->library)))
			thunk(test);
		
		test->methods->success(test);
	
		exit(0);
	}
	else
	{
		urpc_handle* rpc_harness = urpc_connect(sockets[0]);
		MoonUnitTestSummary *_summary;
		urpc_message* message;
		int status;
		
		close(sockets[1]);
		
		waitpid(pid, &status, 0);
		message = urpc_waitread(rpc_harness);
		
		if (message)
		{
			_summary = urpc_msg_payload_get(message, &testsummary_info);
			*summary = *_summary;
			if (summary->reason)
				summary->reason = strdup(_summary->reason);
			urpc_msg_free(message);
		}
		else
		{
			// Couldn't get message, try to figure out what happend
			if (WIFSIGNALED(status))
			{
				summary->result = MOON_RESULT_CRASH;
				summary->stage = MOON_STAGE_UNKNOWN;
				summary->line = 0;
				
				if (WTERMSIG(status) == 11)
					summary->reason = strdup(strsignal(WTERMSIG(status)));
			}
		}
		
		close(sockets[0]);
	}
}
  
void unixharness_cleanup (MoonUnitTestSummary* summary)
{
	free((void*) summary->reason);
}

MoonUnitHarness mu_unixharness =
{
	unixharness_result,
	unixharness_dispatch,
	unixharness_cleanup
};
