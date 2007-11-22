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
