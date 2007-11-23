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
#include <sys/types.h>
#include <sys/socket.h>

void urpc_packet_send(int socket, urpc_packet* packet)
{
	send(socket, packet, sizeof(urpc_packet_header) + packet->header.length, MSG_NOSIGNAL);
}

void urpc_packet_recv(int socket, urpc_packet** packet)
{
	urpc_packet_header header;
	
	if (read(socket, &header, sizeof(urpc_packet_header)) <= 0)
	{
		*packet = NULL;
		return;
	}
	
	*packet = malloc(sizeof(urpc_packet) + header.length);
	
	**packet = *(urpc_packet*)&header;
	
	read(socket, ((void*) (*packet)) + sizeof(urpc_packet_header), header.length);
}

int urpc_packet_available(int socket, long _timeout)
{
	fd_set readset;
	fd_set exset;
	
	struct timeval timeout;
	timeout.tv_sec = _timeout;
	timeout.tv_usec = 0;
	
	FD_ZERO(&readset);
	FD_SET(socket, &readset);
	
	FD_ZERO(&exset);
	FD_SET(socket, &exset);
		
	select(socket+1, &readset, NULL, &exset, _timeout >=0 ? &timeout : NULL);
	
	if (FD_ISSET(socket, &exset))
		return 0;
	else if (FD_ISSET(socket, &readset))
		return 1;
	else
		return 0;
}

int urpc_packet_sendable(int socket, long _timeout)
{
	fd_set writeset;
	fd_set exset;
	
	struct timeval timeout;
	timeout.tv_sec = _timeout;
	timeout.tv_usec = 0;
	
	FD_ZERO(&writeset);
	FD_SET(socket, &writeset);
	
	FD_ZERO(&exset);
	FD_SET(socket, &exset);
		
	select(socket+1, NULL, &writeset, &exset, _timeout >=0 ? &timeout : NULL);
	
	if (FD_ISSET(socket, &exset))
		return 0;
	else if (FD_ISSET(socket, &writeset))
		return 1;
	else
		return 0;
}
