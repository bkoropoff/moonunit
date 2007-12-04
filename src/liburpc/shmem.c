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

#define _GNU_SOURCE
#include "rpc.h"
#include "wire.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>

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
    bool acked;
};

struct __urpc_handle
{
    bool readable;
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

static UrpcStatus
send_message(urpc_message* message)
{
    UrpcStatus result;

    do
    {
        result = urpc_packet_sendable(message->handle->socket, -1);
    } while (result == URPC_RETRY);

    if (result != URPC_SUCCESS)
    {
        return result;
    }
    else
    {
        unsigned int length = sizeof(urpc_packet_message) + strlen(message->shmem_path) + 1;
        urpc_packet* packet = malloc(sizeof (urpc_packet_header) + length);
        
        packet->header.type = PACKET_MESSAGE;
        packet->header.length = length;
        packet->message.id = message->id;
        packet->message.payload = urpc_msg_offset(message, message->payload);
        packet->message.length = message->max_size;
        strcpy(packet->message.path, message->shmem_path);
        do
        {
            result = urpc_packet_send(message->handle->socket, packet);
        } while (result == URPC_RETRY);
        
        free(packet);
        
        return result;
    }
}

static UrpcStatus
ack_message(urpc_message* message)
{
    UrpcStatus result;

    do
    {
        result = urpc_packet_sendable(message->handle->socket, -1);
    } while (result == URPC_RETRY);

    if (result != URPC_SUCCESS)
    {
        return result;
    }
    else
    {
		urpc_packet* packet = malloc(sizeof(urpc_packet) + sizeof(urpc_packet_ack));

		packet->header.type = PACKET_ACK;
		packet->header.length = sizeof(urpc_packet_ack);
		packet->ack.message_id = message->id;

        do
        {		
            result = urpc_packet_send(message->handle->socket, packet);
        } while (result == URPC_RETRY);
    
		free(packet);

        message->acked = true;
        return result;
	}
}

static urpc_message* 
message_from_packet(urpc_packet* packet)
{
	urpc_message* message = malloc(sizeof(urpc_message));
	
    if (!message)
        return NULL;

	message->refcount = 1;
	message->id = packet->message.id;
	message->payload = packet->message.payload;
	message->max_size = packet->message.length;
	message->shmem_path = strdup(packet->message.path);
    message->acked = false;
    message->next = NULL;

    if (!message->shmem_path)
    {
        free(message);
        return NULL;
    }

    message->shmem_fd = shm_open(message->shmem_path, O_RDWR, 0644);

    if (message->shmem_fd < 0)
    {
        free((void*) message->shmem_path);
        free(message);
        return NULL;
    }

	message->shmem_memory = mmap(NULL, message->max_size, PROT_READ | PROT_WRITE, MAP_SHARED, message->shmem_fd, 0);

    if (message->shmem_memory == (void*) -1)
    {
        free((void*) message->shmem_path);
        free(message);
        return NULL;
    }

	message->shmem_next = message->shmem_memory + message->max_size;
	
	message->payload = urpc_msg_pointer(message, message->payload);
	
	return message;
}

urpc_handle* 
urpc_connect(int socket)
{
	urpc_handle* handle = malloc(sizeof (urpc_handle));

    if (!handle)
        return NULL;
	
	handle->socket = socket;
	handle->shmem_count = 0;
	handle->send_queue = NULL;
	handle->recv_queue = NULL;
	handle->ack_queue = NULL;
	handle->id_count = 0;
	handle->readable = true;

	return handle;
}

UrpcStatus
urpc_process(urpc_handle* handle)
{
    UrpcStatus result = URPC_SUCCESS;

	while (handle->send_queue)
	{
		urpc_message* message = handle->send_queue;
		handle->send_queue = message->next;
		
		result = send_message(message);

        if (result != URPC_SUCCESS)
        {			
            return result;
        }
		
		message->next = handle->ack_queue;
		handle->ack_queue = message;
	}
	
	while (handle->readable && 
           (result = urpc_packet_available(handle->socket, 0)) == URPC_SUCCESS)
	{
		urpc_packet* packet = NULL;
		
        do
        {
            result = urpc_packet_recv(handle->socket, &packet);
        } while (result == URPC_RETRY);

        if (result == URPC_EOF)
        {
            handle->readable = false;
            return URPC_SUCCESS;
        }
        else if (result != URPC_SUCCESS)
        {
            handle->readable = false;
            return result;
        }
        else
		{
			switch (packet->header.type)
			{
            case PACKET_ACK:
            {
                urpc_message** cur;
				
                for (cur = &handle->ack_queue; *cur; cur = *cur ? &(*cur)->next : cur)
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
                
                if (!message)
                {
                    handle->readable = false;
                    return URPC_NOMEM;
                }

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
				
                result = ack_message(message);

                if (result != URPC_SUCCESS && result != URPC_EOF)
                {
                    return result;
                }
            }
			}
		}
	}

    return URPC_SUCCESS;
}

UrpcStatus
urpc_read(urpc_handle* handle, urpc_message** message)
{
	if (!handle->recv_queue)
	{
        if (!handle->readable)
        {
            return URPC_EOF;
        }
        else
        {
            return URPC_RETRY;
        }
	}
	else
	{
		*message = handle->recv_queue;
		handle->recv_queue = (*message)->next;
		
        return URPC_SUCCESS;
	}
}

UrpcStatus
urpc_waitread(urpc_handle* handle, urpc_message** message)
{
    UrpcStatus result = URPC_SUCCESS;

	while (!handle->recv_queue)
	{
        if (!handle->readable)
        {
            return URPC_EOF;
        }
        
        do
        {
            result = urpc_packet_available(handle->socket, -1);
        } while (result == URPC_RETRY);

        result = urpc_process(handle);      

        if (result != URPC_SUCCESS)
            return result;
	}
	
	return urpc_read(handle, message);
}

UrpcStatus
urpc_waitdone(urpc_handle* handle)
{
    UrpcStatus result = URPC_SUCCESS;

    while (handle->send_queue || handle->ack_queue)
    {
    
        if (handle->send_queue)
        {
            do
            {
                result = urpc_packet_sendable(handle->socket, -1);
            } while (result == URPC_RETRY);
            
            if (result == URPC_EOF)
                break;
            
            if (result != URPC_SUCCESS)
                return result;
            
            result = urpc_process(handle);
            
            if (result != URPC_SUCCESS)
                return result;
        }
        
        if (handle->ack_queue)
        {
            do
            {
                result = urpc_packet_available(handle->socket, -1);
            } while (result == URPC_RETRY);
            
            result = urpc_process(handle);
            
            if (result == URPC_EOF)
                break;
            
            if (result != URPC_SUCCESS)
                return result;
        }
    }

    return URPC_SUCCESS;
}

void
urpc_disconnect(urpc_handle* handle)
{
    urpc_message* cur, *next;

    for (cur = handle->send_queue; cur; cur = next)
    {
        next = cur->next;
        urpc_msg_free(cur);
    }

    for (cur = handle->recv_queue; cur; cur = next)
    {
        next = cur->next;
        urpc_msg_free(cur);
    }

    for (cur = handle->ack_queue; cur; cur = next)
    {
        next = cur->next;
        urpc_msg_free(cur);
    }

    free(handle);
}

urpc_message* 
urpc_msg_new(urpc_handle* handle, size_t max_size)
{
	urpc_message* message = malloc(sizeof (urpc_message));
	
    if (!message)
        return NULL;

	message->handle = handle;
	message->next = NULL;
	message->refcount = 1;
    message->acked = false;
	message->max_size = max_size;
	
    /* FIXME: don't use asprintf */
    if (asprintf(
            (char **) &message->shmem_path, 
            "/urpc_%i_%i_%i", getpid(), 
            handle->socket, 
            handle->shmem_count++) < 0)
    {
        free(message);
        return NULL;
    }
    
    message->shmem_fd = shm_open(message->shmem_path, O_RDWR | O_CREAT | O_EXCL, 0644);

    if (message->shmem_fd < 0)
    {
        free(message);
        return NULL;
    }

	if (ftruncate(message->shmem_fd, max_size))
    {
        close(message->shmem_fd);
        free(message);
        return NULL;
    }

	message->shmem_memory = mmap(NULL, max_size, PROT_READ | PROT_WRITE, MAP_SHARED, message->shmem_fd, 0);

    if (message->shmem_memory == (void*) -1)
    {
        close(message->shmem_fd);
        free(message);
        return NULL;
    }

	message->shmem_next = message->shmem_memory;
	
	return message;
}

void*
urpc_msg_alloc(urpc_message* message, size_t size)
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

UrpcStatus
urpc_msg_send(urpc_message* message)
{
	message->refcount++;
	queue_message(message);

    return URPC_SUCCESS;
}

void
urpc_msg_free(urpc_message* message)
{
	if (--message->refcount == 0)
	{
		munmap(message->shmem_memory, message->max_size);
        if (!message->acked)
        {
            shm_unlink(message->shmem_path);
        }
		free((void*) message->shmem_path);
		close(message->shmem_fd);
		free(message);
	}
}

void*
urpc_msg_pointer(urpc_message* message, void* offset)
{
	return ((long) offset + message->shmem_memory);
}

void*
urpc_msg_offset(urpc_message* message, void* pointer)
{
	return ((void*) (pointer - message->shmem_memory));
}

void*
urpc_msg_payload_get(urpc_message* message, urpc_typeinfo* info)
{
	urpc_unmarshal_payload(message->shmem_memory, message->max_size, message->payload, info); 
	return message->payload;
}

void
urpc_msg_payload_set(urpc_message* message, void* payload, urpc_typeinfo* info)
{
	message->payload = payload;
	urpc_marshal_payload(message->shmem_memory, message->max_size, message->payload, info); 
}
