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
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

UrpcStatus
urpc_packet_send(int socket, urpc_packet* packet)
{
    char* buffer = (char*) packet;
    ssize_t total = sizeof(urpc_packet_header) + packet->header.length;
    ssize_t remaining = total;

    while (remaining)
    {
        ssize_t sent = send(socket, buffer + (total - remaining), remaining, MSG_NOSIGNAL);
        
        if (sent < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                // If we haven't sent anything yet...
                if (remaining == total)
                    // It's safe to return
                    return URPC_RETRY;
                else
                    // Otherwise we better push through the rest
                    continue;
            }
            else if (errno == EPIPE)
            {
                return URPC_EOF;
            }
            else
            {
                return URPC_ERROR;
            }
        } 
        else if (sent == 0)
        {
            // This shouldn't happen
            return URPC_ERROR;
        }
        else
        {
            remaining -= sent;
        }
    }

    return URPC_SUCCESS;
}

UrpcStatus
urpc_packet_recv(int socket, urpc_packet** packet)
{
	urpc_packet_header header;
    ssize_t amount_read;
    ssize_t remaining;
    char* buffer;

    amount_read = read(socket, &header, sizeof(urpc_packet_header));
    
    if (amount_read < 0)
    {
        if (errno == EAGAIN || errno == EINTR)
        {
            return URPC_RETRY;
        }
        else
        {
            *packet = NULL;
            return URPC_ERROR;
        }
	}
    else if (amount_read == 0)
    {
        return URPC_EOF;
    }
	
	*packet = malloc(sizeof(urpc_packet) + header.length);

    if (!*packet)
        return URPC_NOMEM;
	
	**packet = *(urpc_packet*)&header;
	
    amount_read = 0;
    remaining = header.length;
    buffer = ((char*) (*packet)) + sizeof(urpc_packet_header);

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
                return URPC_ERROR;
            }
        }
        else if (amount_read == 0)
        {
            free(*packet);
            return URPC_ERROR;
        }
        else
        {
            remaining -= amount_read;
        }
    }

    return URPC_SUCCESS;
}

UrpcStatus
urpc_packet_available(int socket, long* _timeout)
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
		return URPC_ERROR;
	else if (FD_ISSET(socket, &readset))
		return URPC_SUCCESS;
	else if (_timeout)
    {
        elapsed = (after.tv_sec - before.tv_sec) * 1000 - (after.tv_usec - before.tv_usec) / 1000;
        *_timeout -= elapsed;
        if (*_timeout <= 0)
        {
            *_timeout = 0;
            return URPC_TIMEOUT;
        }
    }

	return URPC_RETRY;
}

UrpcStatus
urpc_packet_sendable(int socket, long* _timeout)
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
		return URPC_ERROR;
	else if (FD_ISSET(socket, &writeset))
		return URPC_SUCCESS;
	else if (_timeout)
    {   
        elapsed = (after.tv_sec - before.tv_sec) * 1000 - (after.tv_usec - before.tv_usec) / 1000;
        *_timeout -= elapsed;
        if (*_timeout < 0)
        {
            *_timeout = 0;
            return URPC_TIMEOUT;
        }
    }

	return URPC_RETRY;
}
