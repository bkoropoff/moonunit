#define _GNU_SOURCE
#include "rpc.h"
#include "wire.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

struct __urpc_message
{
	struct __urpc_handle* handle;
	struct __urpc_message* next;
	unsigned int refcount;
	size_t max_size;
	void* shmem_memory;
	void* shmem_next;
	const char* shmem_path;
	int shmem_fd;
	void* payload;
	unsigned int id;
};

struct __urpc_handle
{
	int socket;
	unsigned int shmem_count;
	unsigned int id_count;
	urpc_message* send_queue;
	urpc_message* recv_queue;
	urpc_message* ack_queue;
};

static void
queue_message(urpc_message* message)
{
	urpc_handle* handle = message->handle;
	
	if (!handle->send_queue)
		handle->send_queue = message;
	else
	{
		urpc_message* cur;
		
		for (cur = handle->send_queue; cur->next; cur = cur->next);
		
		cur->next = message;
	}
}

static void
send_message(urpc_message* message)
{
	unsigned int length = sizeof(urpc_packet_message) + strlen(message->shmem_path) + 1;
	urpc_packet* packet = malloc(sizeof (urpc_packet) + length);
	
	packet->header.type = PACKET_MESSAGE;
	packet->header.length = length;
	packet->message.id = message->id;
	packet->message.payload = urpc_msg_offset(message, message->payload);
	packet->message.length = message->max_size;
	strcpy(packet->message.path, message->shmem_path);
	urpc_packet_send(message->handle->socket, packet);
	free(packet);
}

static void
ack_message(urpc_message* message)
{
	urpc_packet* packet = malloc(sizeof(urpc_packet) + sizeof(urpc_packet_ack));
	
	packet->header.type = PACKET_ACK;
	packet->header.length = sizeof(urpc_packet_ack);
	packet->ack.message_id = message->id;
	
	urpc_packet_send(message->handle->socket, packet);
	free(packet);
}

static urpc_message* message_from_packet(urpc_packet* packet)
{
	urpc_message* message = malloc(sizeof(urpc_message));
	
	message->refcount = 1;
	message->id = packet->message.id;
	message->payload = packet->message.payload;
	message->max_size = packet->message.length;
	message->shmem_path = strdup(packet->message.path);
	message->shmem_fd = shm_open(message->shmem_path, O_RDWR, 0644);
	message->shmem_memory = mmap(NULL, message->max_size, PROT_READ | PROT_WRITE, MAP_SHARED, message->shmem_fd, 0);
	message->shmem_next = message->shmem_memory + message->max_size;
	
	message->payload = urpc_msg_pointer(message, message->payload);
	
	return message;
}

urpc_handle* urpc_connect(int socket)
{
	urpc_handle* handle = malloc(sizeof (urpc_handle));
	
	handle->socket = socket;
	handle->shmem_count = 0;
	handle->send_queue = NULL;
	handle->recv_queue = NULL;
	handle->ack_queue = NULL;
	handle->id_count = 0;
	
	return handle;
}

void urpc_process(urpc_handle* handle)
{
	while (handle->send_queue)
	{
		urpc_message* message = handle->send_queue;
		handle->send_queue = message->next;
		
		send_message(message);
		
		message->next = handle->ack_queue;
		handle->ack_queue = message;
	}
	
	while (urpc_packet_available(handle->socket))
	{
		urpc_packet* packet = NULL;
		
		if (urpc_packet_recv(handle->socket, &packet), packet != NULL)
		{
			switch (packet->header.type)
			{
				case PACKET_ACK:
				{
					urpc_message** cur;
					
					for (cur = &handle->ack_queue; *cur; cur = &(*cur)->next)
					{
						if ((*cur)->id == packet->ack.message_id)
						{
							urpc_message* message = *cur;
							*cur = message->next;
							urpc_msg_free(message);
						}
					}
				}
				case PACKET_MESSAGE:
				{
					urpc_message* message = message_from_packet(packet);
					message->handle = handle;
					
					free(packet);
					
					if (!handle->recv_queue)
						handle->recv_queue = message;
					else
					{
						urpc_message* cur;
						
						for (cur = handle->recv_queue; cur->next; cur = cur->next);
						
						cur->next = message;
					}
					
					ack_message(message);
				}
			}
		}
	}
}

urpc_message*
urpc_read(urpc_handle* handle)
{
	if (!handle->recv_queue)
	{
		return NULL;
	}
	else
	{
		urpc_message* message = handle->recv_queue;
		handle->recv_queue = message->next;
		
		return message;
	}
}

urpc_message* 
urpc_msg_new(urpc_handle* handle, size_t max_size)
{
	urpc_message* message = malloc(sizeof (urpc_message));
	
	message->handle = handle;
	message->next = NULL;
	message->refcount = 1;
	message->max_size = max_size;
	
	asprintf((char **) &message->shmem_path, "/urpc_%i_%i_%i", getpid(), handle->socket, handle->shmem_count++);
	message->shmem_fd = shm_open(message->shmem_path, O_RDWR | O_CREAT | O_EXCL, 0644);
	ftruncate(message->shmem_fd, max_size);
	message->shmem_memory = mmap(NULL, max_size, PROT_READ | PROT_WRITE, MAP_SHARED, message->shmem_fd, 0);
	message->shmem_next = message->shmem_memory;
	
	return message;
}

void* urpc_msg_alloc(urpc_message* message, size_t size)
{
	if (message->shmem_next + size > message->shmem_memory + message->max_size)
		return NULL;
	else
	{
		void* result = message->shmem_next;
		message->shmem_next += size;
		return result;
	}
}

void urpc_msg_send(urpc_message* message)
{
	message->refcount++;
	queue_message(message);
}

void urpc_msg_free(urpc_message* message)
{
	if (--message->refcount == 0)
	{
		munmap(message->shmem_memory, message->max_size);
		free((void*) message->shmem_path);
		close(message->shmem_fd);
		free(message);
	}
}

void* urpc_msg_pointer(urpc_message* message, void* offset)
{
	return ((long) offset + message->shmem_memory);
}

void* urpc_msg_offset(urpc_message* message, void* pointer)
{
	return ((void*) (pointer - message->shmem_memory));
}

void* urpc_msg_payload_get(urpc_message* message, urpc_typeinfo* info)
{
	urpc_unmarshal_payload(message->shmem_memory, message->max_size, message->payload, info); 
	return message->payload;
}

void urpc_msg_payload_set(urpc_message* message, void* payload, urpc_typeinfo* info)
{
	message->payload = payload;
	urpc_marshal_payload(message->shmem_memory, message->max_size, message->payload, info); 
}
