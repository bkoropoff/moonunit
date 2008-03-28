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

#include "wire.h"

#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#ifdef HAVE_SYS_SELECT_H
#    include <sys/select.h>
#endif
#ifdef HAVE_SELECT_H
#    include <select.h>
#endif
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

uipc_status
uipc_packet_send(int socket, uipc_packet* packet)
{
    char* buffer = (char*) packet;
    ssize_t total = sizeof(uipc_packet_header) + packet->header.length;
    ssize_t remaining = total;

    while (remaining)
    {
	ssize_t sent;
#ifdef MSG_NOSIGNAL
	sent = send(socket, buffer + (total - remaining), remaining, MSG_NOSIGNAL);
#else
    /* Block SIGPIPE for the send */
    struct sigaction blocked, original;

    blocked.sa_handler = SIG_IGN;
    sigemptyset(&blocked.sa_mask);

    sigaction(SIGPIPE, &blocked, &original);
    sent = send(socket, buffer + (total - remaining), remaining, 0);
    sigaction(SIGPIPE, &original, &blocked);
#endif
        
        if (sent < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                // If we haven't sent anything yet...
                if (remaining == total)
                    // It's safe to return
                    return UIPC_RETRY;
                else
                    // Otherwise we better push through the rest
                    continue;
            }
            else if (errno == EPIPE)
            {
                return UIPC_EOF;
            }
            else
            {
                return UIPC_ERROR;
            }
        } 
        else if (sent == 0)
        {
            // This shouldn't happen
            return UIPC_ERROR;
        }
        else
        {
            remaining -= sent;
        }
    }

    return UIPC_SUCCESS;
}

uipc_status
uipc_packet_recv(int socket, uipc_packet** packet)
{
	uipc_packet_header header;
    ssize_t amount_read;
    ssize_t remaining;
    char* buffer;

    amount_read = read(socket, &header, sizeof(uipc_packet_header));
    
    if (amount_read < 0)
    {
        if (errno == EAGAIN || errno == EINTR)
        {
            return UIPC_RETRY;
        }
        else
        {
            *packet = NULL;
            return UIPC_ERROR;
        }
	}
    else if (amount_read == 0)
    {
        return UIPC_EOF;
    }
	
	*packet = malloc(sizeof(uipc_packet) + header.length);

    if (!*packet)
        return UIPC_NOMEM;
	
	**packet = *(uipc_packet*)&header;
	
    amount_read = 0;
    remaining = header.length;
    buffer = ((char*) (*packet)) + sizeof(uipc_packet_header);

    while (remaining)
    {
        amount_read = read(socket, buffer + (header.length - remaining), remaining);

        if (amount_read < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
                // Must finish since we've already read something
                continue;
            else
            {
                free(*packet);
                return UIPC_ERROR;
            }
        }
        else if (amount_read == 0)
        {
            free(*packet);
            return UIPC_ERROR;
        }
        else
        {
            remaining -= amount_read;
        }
    }

    return UIPC_SUCCESS;
}

uipc_status
uipc_packet_available(int socket, long* _timeout)
{
	fd_set readset;
	fd_set exset;
	
    long elapsed;
    struct timeval before, after;
	struct timeval timeout;

    if (_timeout)
    {
        timeout.tv_sec = *_timeout / 1000;
        timeout.tv_usec = (*_timeout % 1000) * 1000;
    }	

	FD_ZERO(&readset);
	FD_SET(socket, &readset);
	
	FD_ZERO(&exset);
	FD_SET(socket, &exset);
		
    gettimeofday(&before, NULL);
	select(socket+1, &readset, NULL, &exset, _timeout ? &timeout : NULL);
    gettimeofday(&after, NULL);


	if (FD_ISSET(socket, &exset))
		return UIPC_ERROR;
	else if (FD_ISSET(socket, &readset))
		return UIPC_SUCCESS;
	else if (_timeout)
    {
        elapsed = (after.tv_sec - before.tv_sec) * 1000 - (after.tv_usec - before.tv_usec) / 1000;
        *_timeout -= elapsed;
        if (*_timeout <= 0)
        {
            *_timeout = 0;
            return UIPC_TIMEOUT;
        }
    }

	return UIPC_RETRY;
}

uipc_status
uipc_packet_sendable(int socket, long* _timeout)
{
	fd_set writeset;
	fd_set exset;
	
    long elapsed;
    struct timeval before, after;
	struct timeval timeout;

    if (_timeout)
    {
        timeout.tv_sec = *_timeout / 1000;
        timeout.tv_usec = (*_timeout % 1000) * 1000;
    }	

	FD_ZERO(&writeset);
	FD_SET(socket, &writeset);
	
	FD_ZERO(&exset);
	FD_SET(socket, &exset);
		
    gettimeofday(&before, NULL);
	select(socket+1, NULL, &writeset, &exset, _timeout ? &timeout : NULL);
    gettimeofday(&after, NULL);


	if (FD_ISSET(socket, &exset))
		return UIPC_ERROR;
	else if (FD_ISSET(socket, &writeset))
		return UIPC_SUCCESS;
	else if (_timeout)
    {   
        elapsed = (after.tv_sec - before.tv_sec) * 1000 - (after.tv_usec - before.tv_usec) / 1000;
        *_timeout -= elapsed;
        if (*_timeout < 0)
        {
            *_timeout = 0;
            return UIPC_TIMEOUT;
        }
    }

	return UIPC_RETRY;
}
