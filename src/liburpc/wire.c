#include "wire.h"

#include <stdlib.h>
#include <poll.h>
#include <unistd.h>

void urpc_packet_send(int socket, urpc_packet* packet)
{
	write(socket, packet, sizeof(urpc_packet_header) + packet->header.length);
}

void urpc_packet_recv(int socket, urpc_packet** packet)
{
	urpc_packet_header header;
	
	read(socket, &header, sizeof(urpc_packet_header));
	
	*packet = malloc(sizeof(urpc_packet) + header.length);
	
	**packet = *(urpc_packet*)&header;
	
	read(socket, ((void*) (*packet)) + sizeof(urpc_packet_header), header.length);
}

int urpc_packet_available(int socket)
{
	struct pollfd fd;
	
	fd.fd = socket;
	fd.events = POLLIN;
	
	if (poll(&fd, 1, 0) > 0 && fd.revents & POLLIN)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
