#include <moonunit/test.h>
#include <moonunit/harness.h>
#include <urpc/rpc.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

static urpc_typeinfo testsummary_info =
{
	1,
	{
		URPC_POINTER(MoonTestSummary, reason, NULL)
	}
};

void unixharness_result(MoonUnitTest* test, const MoonTestSummary* _summary)
{	
	urpc_handle* rpc_handle = test->data;

	urpc_message* message = urpc_msg_new(rpc_handle, 2048);
	
	MoonTestSummary* summary = urpc_msg_alloc(message, sizeof(MoonTestSummary));
	
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
	
	exit(0);
}

void unixharness_dispatch(MoonUnitTest* test, MoonTestSummary* summary)
{
	int sockets[2];
	
	socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	
	urpc_handle* rpc_harness = urpc_connect(sockets[0]);
	
	if (!fork())
	{
		urpc_handle* rpc_test = urpc_connect(sockets[1]);

		test->harness = &mu_unixharness;
		test->data = rpc_test;
		
		test->function(test);
		
		__mu_success(test);
		exit(0);
	}
	else
	{
		MoonTestSummary *_summary;
		urpc_message* message;
		
		while (!(message = urpc_read(rpc_harness)))
			urpc_process(rpc_harness);
			
		_summary = urpc_msg_payload_get(message, &testsummary_info);
		
		*summary = *_summary;
		if (summary->reason)
			summary->reason = strdup(_summary->reason);
		urpc_msg_free(message);
	}
}
  
void unixharness_cleanup (MoonTestSummary* summary)
{
	free((void*) summary->reason);
}

MoonHarness mu_unixharness =
{
	unixharness_result,
	unixharness_dispatch,
	unixharness_cleanup
};
