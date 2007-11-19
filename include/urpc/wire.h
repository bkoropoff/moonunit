#ifndef __URPC_WIRE_H__
#define __URPC_WIRE_H__

typedef struct urpc_packet_header
{
	enum
	{
		PACKET_MESSAGE, PACKET_ACK
	} type;
	unsigned int length;
} urpc_packet_header;

typedef struct urpc_packet_message
{
	unsigned int id;
	void* payload;
	unsigned int length;
	char path[];
} urpc_packet_message;

typedef struct urpc_packet_ack
{
	unsigned int message_id;
} urpc_packet_ack;

typedef struct urpc_packet
{
	urpc_packet_header header;
	union
	{
		urpc_packet_message message;
		urpc_packet_ack ack;
	};
} urpc_packet;

void urpc_packet_send(int socket, urpc_packet* packet);
void urpc_packet_recv(int socket, urpc_packet** packet);
int urpc_packet_available(int socket, long timeout);
int urpc_packet_sendable(int socket, long timeout);

#endif
