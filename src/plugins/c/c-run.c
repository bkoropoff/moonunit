/*
 * Copyright (c) 2007-2008, Brian Koropoff
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
#include <moonunit/private/util.h>
#include <moonunit/interface.h>
#include <moonunit/error.h>
#include <uipc/ipc.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#ifdef HAVE_SIGNAL_H
#    include <signal.h>
#endif
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>

#include "backtrace.h"
#include "c-token.h"
#include "c-load.h"
#include "c-run.h"

#ifdef CPLUSPLUS_ENABLED
#    include "cplusplus.h"
#endif

static long default_timeout = 2000;
static unsigned int default_iterations = 1;
static bool is_debug = false;
static MuInterfaceToken* current_token;

typedef struct
{
    MuTestStatus expect_status;
} ExpectMsg;

typedef struct
{
    long timeout;
} TimeoutMsg;

typedef struct
{
    unsigned int count;
} IterationsMsg;

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

static uipc_typeinfo iterations_info =
{
    .size = sizeof(IterationsMsg),
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
#define MSG_TYPE_ITERATIONS 4

static MuInterfaceToken*
ctoken_current(void* data)
{
    return (MuInterfaceToken*) data;
}

static
void
ctoken_event_fork(MuInterfaceToken* _token, const MuLogEvent* event)
{
    CTokenFork* token = (CTokenFork*) _token;
    uipc_handle* ipc_handle = token->ipc_handle;

    if (!ipc_handle || event->level > token->max_log_level)
    {
        return;
    }

    pthread_mutex_lock(&token->lock);

    ((MuLogEvent*) event)->stage = token->current_stage;    

    uipc_message* message = uipc_msg_new(MSG_TYPE_EVENT);
    uipc_msg_set_payload(message, event, &logevent_info);
    uipc_send(ipc_handle, message, NULL);
    uipc_msg_free(message);

    pthread_mutex_unlock(&token->lock);
}

static void ctoken_free_fork(CTokenFork* token);

static
void
ctoken_result_fork(MuInterfaceToken* _token, const MuTestResult* summary)
{    
    CTokenFork* token = (CTokenFork*) _token;
    uipc_handle* ipc_handle = token->ipc_handle;

    assert(ipc_handle != NULL);

    pthread_mutex_lock(&token->lock);
    
    ((MuTestResult*) summary)->stage = token->current_stage;
    uipc_message* message = uipc_msg_new(MSG_TYPE_RESULT);
    uipc_msg_set_payload(message, summary, &testresult_info);
    uipc_send(ipc_handle, message, NULL);
    uipc_msg_free(message);

    ctoken_free_fork(token);
    uipc_close(ipc_handle);
    exit(0);

    pthread_mutex_unlock(&token->lock);
}

static
void
ctoken_meta_fork(MuInterfaceToken* _token, MuInterfaceMeta type, ...)
{
    CTokenFork* token = (CTokenFork*) _token;
    va_list ap;

    if (!token->ipc_handle)
    {
        return;
    }

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
    case MU_META_ITERATIONS:
    {
        uipc_handle* ipc_handle = token->ipc_handle;
        IterationsMsg msg = { va_arg(ap, unsigned int) };
	
        if (!ipc_handle)
            return;
        
        uipc_message* message = uipc_msg_new(MSG_TYPE_ITERATIONS);
        uipc_msg_set_payload(message, &msg, &iterations_info);
        uipc_send(ipc_handle, message, NULL);
        uipc_msg_free(message);
        break;
    }
    case MU_META_LOG_LEVEL:
        *va_arg(ap, MuLogLevel*) = token->max_log_level;
        break;
    }

    va_end(ap);

    pthread_mutex_unlock(&token->lock);
}

static void
longjmp_on_signal(int sig)
{
    CTokenInproc* token = (CTokenInproc*) current_token;

    siglongjmp(token->jmpbuf, 1);
}

static void
ctoken_longjmp_inproc(CTokenInproc* token)
{
    if (pthread_self() == token->self)
    {
        siglongjmp(token->jmpbuf, 1);
    }
    else
    {
        struct sigaction action = {};
        action.sa_handler = longjmp_on_signal;
        action.sa_flags = SA_RESETHAND;

        (void) sigaction(SIGINT, &action, NULL);

        pthread_kill(token->self, SIGINT);
    }
}

static void
ctoken_event_inproc(MuInterfaceToken* _token, const MuLogEvent* event)
{
    CTokenInproc* token = (CTokenInproc*) _token;

    if (event->level <= token->max_log_level)
    {
        token->cb(event, token->data);
    }
}

static void
ctoken_result_inproc(MuInterfaceToken* _token, const MuTestResult* summary)
{    
    CTokenInproc* token = (CTokenInproc*) _token;

    if (summary->status != MU_STATUS_SKIPPED && summary->status != token->result->expected)
    {
        abort();
    }

    token->result->status = summary->status;
    token->result->reason = safe_strdup(summary->reason);
    token->result->file = safe_strdup(summary->file);

    ctoken_longjmp_inproc(token);
}

// This signal handler is wildly unsafe.  It is used
// to make a best-effort attempt to deal with crashing
// tests when forking is disabled.
static
void crash_recover(int sig)
{
    CTokenInproc* token = (CTokenInproc*) current_token;

    token->result->status = MU_STATUS_CRASH;

    ctoken_longjmp_inproc(token);
}

static int crash_signals[] =
{
    SIGILL,
    SIGABRT,
    SIGFPE,
    SIGSEGV,
    SIGPIPE
    -1
};

static
void
prepare_for_crash(void)
{
    int i = 0;

    for (i = 0; crash_signals[i] >= 0; i++)
    {
        (void) signal(crash_signals[i], crash_recover);
    }
}

static
void
reset_crash_signals()
{
    int i = 0;

    for (i = 0; crash_signals[i] >= 0; i++)
    {
        (void) signal(crash_signals[i], SIG_DFL);
    }
}

static
void
ctoken_meta_inproc(MuInterfaceToken* _token, MuInterfaceMeta type, ...)
{
    CTokenInproc* token = (CTokenInproc*) _token;
    va_list ap;
    long int timeout = 0;

    pthread_mutex_lock(&token->lock);

    va_start(ap, type);

    switch (type)
    {
    case MU_META_EXPECT:
        token->result->expected = va_arg(ap, MuTestStatus);
        if (token->result->expected == MU_STATUS_CRASH)
        {
            // The test intends to crash.  Install signal handlers
            // to attempt to deal with it.  This is, of course, on
            // a best-effort basis only.
            prepare_for_crash();
        }
        break;
    case MU_META_TIMEOUT:
        timeout = va_arg(ap, long int);
        if (timeout < 1000)
        {
            timeout = 1000;
        }
        (void) alarm(timeout/1000);
        break;
    case MU_META_ITERATIONS:
        *token->iterations = va_arg(ap, unsigned int);
        break;
    case MU_META_LOG_LEVEL:
        *va_arg(ap, MuLogLevel*) = token->max_log_level;
        break;
    default:
        break;
    }

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
    CTokenFork* token = (CTokenFork*) current_token;

    if (getpid() == token->child)
    {
        MuTestResult summary;
    
        summary.status = MU_STATUS_CRASH;
        summary.expected = token->expected;
        summary.stage = token->current_stage;
        summary.reason = signal_description(sig);
        summary.file = NULL;
        summary.line = 0;
        summary.backtrace = get_backtrace(0);

        current_token->result(current_token, &summary);
    }
    else
    {
        /* This is not the child process but a
           child of the child that has inherited
           this signal handler.  Switch back to the
           default handler and reraise the signal */
        signal(sig, SIG_DFL);
        raise(sig);
    }
}

static void
signal_setup(void)
{
    /* List of signals we care about */
    static int siglist[] =
    {
        SIGSEGV,
        SIGBUS,
        SIGILL,
        SIGPIPE,
        SIGFPE,
        SIGABRT,
        SIGTERM,
        0
    };

    struct sigaction act;
    int i;
    
    act.sa_flags = 0;
    act.sa_handler = signal_handler;
    sigemptyset(&act.sa_mask);

    /* Set up a mask that blocks all other handled
       signals while the handler is running.
       This prevents deadlocks */
    for (i = 0; siglist[i]; i++)
    {
        sigaddset(&act.sa_mask, siglist[i]);
    }

    for (i = 0; siglist[i]; i++)
    {
        sigaction(siglist[i], &act, NULL);
    }
}

static CTokenFork*
ctoken_new_fork(MuTest* test)
{
    CTokenFork* token = xcalloc(1, sizeof(CTokenFork));

    token->base.test = test;
    token->base.meta = ctoken_meta_fork;
    token->base.result = ctoken_result_fork;
    token->base.event = ctoken_event_fork;
    token->expected = MU_STATUS_SUCCESS;
    pthread_mutex_init(&token->lock, NULL);

    return token;
}

static CTokenInproc*
ctoken_new_inproc(MuTest* test)
{
    CTokenInproc* token = xcalloc(1, sizeof(CTokenInproc));

    token->base.test = test;
    token->base.meta = ctoken_meta_inproc;
    token->base.result = ctoken_result_inproc;
    token->base.event = ctoken_event_inproc;
    pthread_mutex_init(&token->lock, NULL);

    return token;
}

static void
ctoken_free_fork(CTokenFork* token)
{
    pthread_mutex_destroy(&token->lock);
    free(token);
}

static void
ctoken_free_inproc(CTokenInproc* token)
{
    pthread_mutex_destroy(&token->lock);
    free(token);
}

#ifdef CPLUSPLUS_ENABLED
#   define INVOKE(thunk) (cplusplus_trampoline((thunk)))
#else
#   define INVOKE(thunk) ((thunk)())
#endif

static void
cloader_run_child(MuTest* test, CTokenFork* token)
{
    MuThunk thunk;

    /* Set up the C/C++ interface to call into our token */
    mu_interface_set_current_token_callback(ctoken_current, token);
        
    /* Set up handlers to catch asynchronous/fatal signals */
    signal_setup();

    /* Stage: library setup */
    token->current_stage = MU_STAGE_LIBRARY_SETUP;
    
    if ((thunk = cloader_library_setup(test->loader, test->library)))
        INVOKE(thunk);
    
    /* Stage: fixture setup */
    token->current_stage = MU_STAGE_FIXTURE_SETUP;
    
    if ((thunk = cloader_fixture_setup(test->loader, test)))
        INVOKE(thunk);
    
    /* Stage: test */
    token->current_stage = MU_STAGE_TEST;
    
    INVOKE(((CTest*) test)->entry->run);
    
    /* Stage: fixture teardown */
    token->current_stage = MU_STAGE_FIXTURE_TEARDOWN;
    
    if ((thunk = cloader_fixture_teardown(test->loader, test)))
        INVOKE(thunk);
    
    /* Stage: library teardown */
    token->current_stage = MU_STAGE_LIBRARY_TEARDOWN;
    
    if ((thunk = cloader_library_teardown(test->loader, test->library)))
        INVOKE(thunk);
    
    /* If we got this far without incident, explicitly succeed */
    mu_interface_result(NULL, 0, MU_STATUS_SUCCESS, NULL);
}

#ifdef HAVE_SIGTIMEDWAIT
static void
sigchld_handler()
{
}

static int
wait_child(pid_t pid, int* status, int ms)
{
    sigset_t set, oldset;
    struct sigaction act, oldact;
    struct timespec timeout;
    int ret = 0;

    /* Block SIGCHLD */
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigprocmask(SIG_BLOCK, &set, &oldset);
    /* Register handler for SIGCHLD to ensure
       it is processed */
    act.sa_handler = sigchld_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, &oldact);
    
    /* Check if the child already exited before
       we blocked the signal */
    if (waitpid(pid, status, WNOHANG) == pid)
    {
        /* It did, so we are done */
        ret = 0;
        goto done;
    }
    
    timeout.tv_sec = ms / 1000;
    timeout.tv_nsec = (ms % 1000) * 1000000;
    /* Otherwise, wait for it to exit with a timeout */
    sigtimedwait(&set, NULL, &timeout);
    
    /* Check one more time for status */
    if (waitpid(pid, status, WNOHANG) == pid)
    {
        /* It finally exited */
        ret = 0;
        goto done;
    }
    else
    {
        /* Kill the thing and wait once more to reap
           the zombie process */
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        ret = -1;
        goto done;
    }

done:
    /* Finally, restore old signal handler/signal masks */
    sigprocmask(SIG_SETMASK, &oldset, NULL);
    sigaction(SIGCHLD, &oldact, NULL);

    return ret;
}
#else

static int loop_fd;

static void
sigchld_handler(int sig)
{
    char c = 0;

    write(loop_fd, (void*) &c, 1);
}

static int
wait_child(pid_t pid, int* status, int ms)
{
    sigset_t set, oldset;
    struct sigaction act, oldact;
    struct timeval timeout;
    int ret = 0;
    int loop[2];
    fd_set readfds;

    /* Block SIGCHLD */
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigprocmask(SIG_BLOCK, &set, &oldset);
    /* Register handler for SIGCHLD to ensure
       it is processed */
    act.sa_handler = sigchld_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, &oldact);
    
    /* Create a pipe for the signal handler */
    pipe(loop);
    loop_fd = loop[1];

    /* Unblock the signal */
    sigprocmask(SIG_UNBLOCK, &set, NULL);

    /* Check if the child already exited before
       we blocked the signal */
    if (waitpid(pid, status, WNOHANG) == pid)
    {
        /* It did, so we are done */
        ret = 0;
        goto done;
    }

    /* Do a select on the read fd to wait for the signal */
    FD_ZERO(&readfds);
    FD_SET(loop[0], &readfds);
    timeout.tv_sec = ms / 1000;
    timeout.tv_usec = (ms % 1000) * 1000; 
    select(loop[0] + 1, &readfds, NULL, NULL, &timeout); 
    
    if (waitpid(pid, status, WNOHANG) == pid)
    {
        /* It's done now */
        ret = 0;
        goto done;
    }
    else
    {
        /* Kill the thing and wait once more to reap
           the zombie process */
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        ret = -1;
        goto done;
    }

done:
    sigaction(SIGCHLD, &oldact, NULL);
    sigprocmask(SIG_SETMASK, &oldset, NULL);

    close(loop[0]);
    close(loop[1]);

    return ret;
}
#endif

/* Main loop for harvesting messages from the child process */
static MuTestResult*
cloader_run_parent(MuTest* test, CTokenFork* token, MuLogCallback cb, void* cb_data, unsigned int* iterations)
{
    uipc_handle* ipc = token->ipc_handle;
    MuTestResult *summary = NULL;
    uipc_message* message = NULL;
    int status;
    uipc_status uipc_result = UIPC_SUCCESS;
    long timeout = default_timeout;
    uipc_time deadline;
    bool done = false;
    /* Have we timed out once already? */
    bool timedout = false;

    uipc_time_current_offset(&deadline, 0, timeout * 1000);

process:
    while (!done)
    {    
        uipc_result = uipc_recv(ipc, &message, &deadline);
        
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
                cb(event, cb_data);
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
                timeout = msg->timeout;
                uipc_time_current_offset(&deadline, 0, timeout * 1000);
                uipc_msg_free_payload(msg, &timeout_info);
                uipc_msg_free(message);
                message = NULL;
                break;
            }
            case MSG_TYPE_ITERATIONS:
            {
                IterationsMsg* msg = uipc_msg_get_payload(message, &iterations_info);
                *iterations = msg->count;
                uipc_msg_free_payload(msg, &iterations_info);
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
     
    /* If the test timed out */
    if (uipc_result == UIPC_TIMEOUT && !timedout)
    {
        /* Poke the child process to give it a chance to send us results */
        kill(token->child, SIGTERM);
        /* Put another 10th of a second on the clock */
        uipc_time_current_offset(&deadline, 0, 100 * 1000);
        /* Go back into the event processing loop */
        timedout = true;
        done = false;
        goto process;
    }

    /* Wait for up to 500 ms for the child to finish exiting */
    wait_child(token->child, &status, 500);
        
    if (!summary)
    {
        summary = xcalloc(1, sizeof(MuTestResult));
        // Timed out waiting for response
        if (uipc_result == UIPC_TIMEOUT)
        {
            char* reason = format("Test timed out after %li milliseconds", timeout);
            
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
        /* If we timed out, change the test result to reflect this */
        if (timedout)
        {
            summary->status = MU_STATUS_TIMEOUT;
            if (summary->reason)
                free((void*) summary->reason);
            summary->reason = format("Test timed out after %li milliseconds", timeout);
        }
    }
    
    if (message)
        uipc_msg_free(message);
    
    return summary;
}

static MuTestResult*
cloader_run_fork(MuTest* test, MuLogCallback cb, void* data, MuLogLevel max_level,
                 unsigned int* iterations)
{
    int sockets[2];
    pid_t pid;
    CTokenFork* token = ctoken_new_fork(test);

    current_token = &token->base;
    
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    
    /* We must force a flush of all open output streams or the child
     * will end up flushing non-empty buffers on exit, resulting in
     * bizarre duplicate output
     */

    fflush(NULL);
    
    if (!(pid = fork()))
    {
        /* Child */
        
        uipc_handle* ipc;
                
        /* Set up ipc handle, close unneeded socket end */
        ipc = uipc_attach(sockets[1]);
        close(sockets[0]);
        
        /* Set up token */
        token->ipc_handle = ipc;
        token->max_log_level = max_level;
        token->child = getpid();
        
        /* Run test procedure */
        cloader_run_child(test, token);

        /* Tear down ipc handle and close connection */
        uipc_detach(ipc);
        close(sockets[1]);

        /* Exit (although it's unlikely we'll get here) */
        exit(0);
    }
    else
    {
        /* Parent */
        
        uipc_handle* ipc;
        MuTestResult* result;

        /* Set up ipc handle, close unneeded socket end */
        ipc = uipc_attach(sockets[0]);
        close(sockets[1]);
        
        /* Set up token */
        token->ipc_handle = ipc;
        token->child = pid;

        /* Harvest events/result from child */
        result = cloader_run_parent(test, token, cb, data, iterations);

        /* Tear down ipc handle and close connection */
        uipc_detach(ipc);
        close(sockets[0]);

        /* Free token */
        ctoken_free_fork(token);

        return result;
    }
}

static MuTestResult*
cloader_run_thunk_inproc(MuThunk thunk)
{
    CTokenInproc* token = ctoken_new_inproc(NULL);
    MuTestResult* volatile result = xcalloc(1, sizeof(*result));

    result->status = MU_STATUS_SUCCESS;

    if (!sigsetjmp(token->jmpbuf, 1))
    {
        token->result = result;
        /* Set up the C/C++ interface to call into our token */
        mu_interface_set_current_token_callback(ctoken_current, token);
        
        INVOKE(thunk);
    }

    ctoken_free_inproc(token);

    return result;
}

void
cloader_free_result(MuLoader* _self, MuTestResult* result)
{
    uipc_msg_free_payload(result, &testresult_info);
}


static void
cloader_run_inproc(MuTest* test, CTokenInproc* token)
{
    MuThunk thunk;

    /* Set up the C/C++ interface to call into our token */
    mu_interface_set_current_token_callback(ctoken_current, token);

    /* Stage: library setup */
    token->result->stage = MU_STAGE_LIBRARY_SETUP;

    if ((thunk = cloader_library_setup(test->loader, test->library)))
        INVOKE(thunk);

    /* Stage: fixture setup */
    token->result->stage = MU_STAGE_FIXTURE_SETUP;

    if ((thunk = cloader_fixture_setup(test->loader, test)))
        INVOKE(thunk);

    /* Stage: test */
    token->result->stage = MU_STAGE_TEST;

    INVOKE(((CTest*) test)->entry->run);

    /* Stage: fixture teardown */
    token->result->stage = MU_STAGE_FIXTURE_TEARDOWN;

    if ((thunk = cloader_fixture_teardown(test->loader, test)))
        INVOKE(thunk);

    /* Stage: library teardown */
    token->result->stage = MU_STAGE_LIBRARY_TEARDOWN;

    if ((thunk = cloader_library_teardown(test->loader, test->library)))
        INVOKE(thunk);

    /* If we got this far without incident, explicitly succeed */
    mu_interface_result(NULL, 0, MU_STATUS_SUCCESS, NULL);
}

static void
timeout_signal(int sig)
{
    CTokenInproc* token = (CTokenInproc*) current_token;

    if (token->result->expected != MU_STATUS_TIMEOUT)
    {
        abort();
    }

    token->result->status = MU_STATUS_TIMEOUT;

    ctoken_longjmp_inproc(token);
}

static MuTestResult*
cloader_debug(MuTest* test, MuLogCallback cb, void* data, MuLogLevel max_level,
              unsigned int* iterations)
{
    MuTestResult* volatile result = xcalloc(1, sizeof(*result));
    CTokenInproc* token = ctoken_new_inproc(test);
    long int timeout = default_timeout;

    current_token = &token->base;

    token->base.test = test;
    token->result = result;
    token->cb = cb;
    token->data = data;
    token->iterations = iterations;
    token->max_log_level = max_level;
    token->self = pthread_self();

    if (timeout < 1000)
    {
        timeout = 1000;
    }

    (void) signal(SIGALRM, timeout_signal);
    (void) alarm(timeout);

    if (!sigsetjmp(token->jmpbuf, 1))
    {
        cloader_run_inproc(test, token);
    }

    (void) alarm(0);
    reset_crash_signals();

    ctoken_free_inproc(token);

    return result;
}

MuTestResult*
cloader_dispatch(MuLoader* _self, MuTest* test, MuLogCallback cb, void* data, MuLogLevel max_level)
{
    unsigned int iterations = default_iterations;
    unsigned int i;
    MuTestResult* result = NULL;

    for (i = 0; i < iterations; i++)
    {
        if (result)
        {
            cloader_free_result(_self, result);
        }

        if (is_debug)
        {
            result = cloader_debug(test, cb, data, max_level, &iterations);
        }
        else
        {
            result = cloader_run_fork(test, cb, data, max_level, &iterations);
        }

        if (result->status == MU_STATUS_SKIPPED || result->status != result->expected)
            break;
    }

    return result;
}

void
cloader_construct(MuLoader* _self, MuLibrary* _library, MuError** err)
{
    CLibrary* library = (CLibrary*) _library;
    MuTestResult* result = NULL;

    if (library->library_construct)
    {
        result = cloader_run_thunk_inproc(library->library_construct->run);

        if (result->status != MU_STATUS_SUCCESS)
        {
            MU_RAISE_GOTO(error, err, MU_ERROR_CONSTRUCT_LIBRARY, "%s", result->reason);
        }
    }
    
error:
    
    if (result)
    {
        cloader_free_result(_self, result);
    }
}

void
cloader_destruct(MuLoader* _self, MuLibrary* _library, MuError** err)
{
    CLibrary* library = (CLibrary*) _library;
    MuTestResult* result = NULL;

    if (library->library_destruct)
    {
        result = cloader_run_thunk_inproc(library->library_destruct->run);

        if (result->status != MU_STATUS_SUCCESS)
        {
            MU_RAISE_GOTO(error, err, MU_ERROR_CONSTRUCT_LIBRARY, "%s", result->reason);
        }
    }
    
error:
    
    if (result)
    {
        cloader_free_result(_self, result);
    }
}

static
void
timeout_set(MuLoader* self, int timeout)
{
    default_timeout = timeout;
}

static
int
timeout_get(MuLoader* self)
{
    return default_timeout;
}

static
void
iterations_set(MuLoader* self, int count)
{
    default_iterations = count;
}

static
int
iterations_get(MuLoader* self)
{
    return (int) default_iterations;
}

static
void
debug_set(MuLoader* self, bool set)
{
    is_debug = set;
}

static
bool
debug_get(MuLoader* self)
{
    return is_debug;
}

MuOption cloader_options[] =
{

    MU_OPTION("timeout", MU_TYPE_INTEGER, timeout_get, timeout_set,
              "Time in milliseconds before tests automatically "
              "fail and are forcefully terminated"),

    MU_OPTION("iterations", MU_TYPE_INTEGER, iterations_get, iterations_set,
              "The number of times each test is run (unless specified by the test)"),

    MU_OPTION("debug", MU_TYPE_BOOLEAN, debug_get, debug_set,
              "Whether to run in debug mode (avoid forking)"),
    MU_OPTION_END
};
