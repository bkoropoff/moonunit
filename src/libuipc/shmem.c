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
#include "ipc.h"
#include "wire.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>

struct __uipc_message
{
	struct __uipc_handle* handle;
	struct __uipc_message* next;
	unsigned int refcount;
	size_t max_size;
	void* shmem_memory;
	void* shmem_next;
	const char* shmem_path;
	int shmem_fd;
	void* payload;
	unsigned int id;
    bool acked;
    UipcMessageType type;
};

struct __uipc_handle
{
    bool readable;
	int socket;
	unsigned int shmem_count;
	unsigned int id_count;
	uipc_message* send_queue;
	uipc_message* recv_queue;
	uipc_message* ack_queue;
};

static void
queue_message(uipc_message* message)
{
	uipc_handle* handle = message->handle;
	
	if (!handle->send_queue)
		handle->send_queue = message;
	else
	{
		uipc_message* cur;
		
		for (cur = handle->send_queue; cur->next; cur = cur->next);
		
		cur->next = message;
	}
}

static UipcStatus
send_message(uipc_message* message)
{
    UipcStatus result;

    do
    {
        result = uipc_packet_sendable(message->handle->socket, NULL);
    } while (result == UIPC_RETRY);

    if (result != UIPC_SUCCESS)
    {
        return result;
    }
    else
    {
        unsigned int length = sizeof(uipc_packet_message) + strlen(message->shmem_path) + 1;
        uipc_packet* packet = malloc(sizeof (uipc_packet_header) + length);
        
        memset(packet, 0, sizeof(uipc_packet_header) + length);

        packet->header.type = PACKET_MESSAGE;
        packet->header.length = length;
        packet->message.type = message->type;
        packet->message.id = message->id;
        packet->message.payload = uipc_msg_offset(message, message->payload);
        packet->message.length = message->max_size;
        strcpy(packet->message.path, message->shmem_path);
        do
        {
            result = uipc_packet_send(message->handle->socket, packet);
        } while (result == UIPC_RETRY);
        
        free(packet);
        
        return result;
    }
}

static UipcStatus
ack_message(uipc_message* message)
{
    UipcStatus result;

    do
    {
        result = uipc_packet_sendable(message->handle->socket, NULL);
    } while (result == UIPC_RETRY);

    if (result != UIPC_SUCCESS)
    {
        return result;
    }
    else
    {
		uipc_packet* packet = malloc(sizeof(uipc_packet) + sizeof(uipc_packet_ack));

		packet->header.type = PACKET_ACK;
		packet->header.length = sizeof(uipc_packet_ack);
		packet->ack.message_id = message->id;

        do
        {		
            result = uipc_packet_send(message->handle->socket, packet);
        } while (result == UIPC_RETRY);
    
		free(packet);

        message->acked = true;
        return result;
	}
}

static uipc_message* 
message_from_packet(uipc_packet* packet)
{
	uipc_message* message = malloc(sizeof(uipc_message));
	
    if (!message)
        return NULL;

	message->refcount = 1;
    message->type = packet->message.type;
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
	
	message->payload = uipc_msg_pointer(message, message->payload);
	
	return message;
}

uipc_handle* 
uipc_connect(int socket)
{
	uipc_handle* handle = malloc(sizeof (uipc_handle));

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

UipcStatus
uipc_process(uipc_handle* handle)
{
    UipcStatus result = UIPC_SUCCESS;
    long zero = 0;

	while (handle->send_queue)
	{
		uipc_message* message = handle->send_queue;
		handle->send_queue = message->next;
		
		result = send_message(message);

        if (result != UIPC_SUCCESS)
        {			
            return result;
        }
		
		message->next = handle->ack_queue;
		handle->ack_queue = message;
	}
	
	while (handle->readable && 
           (result = uipc_packet_available(handle->socket, &zero)) == UIPC_SUCCESS)
	{
		uipc_packet* packet = NULL;
		
        do
        {
            result = uipc_packet_recv(handle->socket, &packet);
        } while (result == UIPC_RETRY);

        if (result == UIPC_EOF)
        {
            handle->readable = false;
            return UIPC_SUCCESS;
        }
        else if (result != UIPC_SUCCESS)
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
                uipc_message** cur;
				
                for (cur = &handle->ack_queue; *cur; cur = *cur ? &(*cur)->next : cur)
                {
                    if ((*cur)->id == packet->ack.message_id)
                    {
                        uipc_message* message = *cur;
                        *cur = message->next;
                        uipc_msg_free(message);
                    }
                }

                free(packet);

                break;
            }
            case PACKET_MESSAGE:
            {
                uipc_message* message = message_from_packet(packet);
                
                free(packet);

                if (!message)
                {
                    handle->readable = false;
                    return UIPC_NOMEM;
                }

                message->handle = handle;
				
                if (!handle->recv_queue)
                    handle->recv_queue = message;
                else
                {
                    uipc_message* cur;
					
                    for (cur = handle->recv_queue; cur->next; cur = cur->next);
					
                    cur->next = message;
                }
				
                result = ack_message(message);

                if (result != UIPC_SUCCESS && result != UIPC_EOF)
                {
                    return result;
                }

                break;
            }
			}
		}
	}

    return UIPC_SUCCESS;
}

UipcStatus
uipc_read(uipc_handle* handle, uipc_message** message)
{
	if (!handle->recv_queue)
	{
        if (!handle->readable)
        {
            return UIPC_EOF;
        }
        else
        {
            return UIPC_RETRY;
        }
	}
	else
	{
		*message = handle->recv_queue;
		handle->recv_queue = (*message)->next;
		
        return UIPC_SUCCESS;
	}
}

UipcStatus
uipc_waitread(uipc_handle* handle, uipc_message** message, long* timeout)
{
    UipcStatus result = UIPC_SUCCESS;

	while (!handle->recv_queue)
	{
        if (!handle->readable)
        {
            return UIPC_EOF;
        }
        
        do
        {
            result = uipc_packet_available(handle->socket, timeout);
        } while (result == UIPC_RETRY);

        if (result != UIPC_SUCCESS)
            return result;

        result = uipc_process(handle);      

        if (result != UIPC_SUCCESS)
            return result;
	}
	
	return uipc_read(handle, message);
}

UipcStatus
uipc_waitdone(uipc_handle* handle, long* timeout)
{
    UipcStatus result = UIPC_SUCCESS;

    while (handle->send_queue || handle->ack_queue)
    {
    
        if (handle->send_queue)
        {
            do
            {
                result = uipc_packet_sendable(handle->socket, timeout);
            } while (result == UIPC_RETRY);
            
            if (result == UIPC_EOF)
                break;
            
            if (result != UIPC_SUCCESS)
                return result;
            
            result = uipc_process(handle);
            
            if (result != UIPC_SUCCESS)
                return result;
        }
        
        if (handle->ack_queue)
        {
            do
            {
                result = uipc_packet_available(handle->socket, timeout);
            } while (result == UIPC_RETRY);
            
            result = uipc_process(handle);
            
            if (result == UIPC_EOF)
                break;
            
            if (result != UIPC_SUCCESS)
                return result;
        }
    }

    return UIPC_SUCCESS;
}

void
uipc_disconnect(uipc_handle* handle)
{
    uipc_message* cur, *next;

    for (cur = handle->send_queue; cur; cur = next)
    {
        next = cur->next;
        uipc_msg_free(cur);
    }

    for (cur = handle->recv_queue; cur; cur = next)
    {
        next = cur->next;
        uipc_msg_free(cur);
    }

    for (cur = handle->ack_queue; cur; cur = next)
    {
        next = cur->next;
        uipc_msg_free(cur);
    }

    free(handle);
}

uipc_message* 
uipc_msg_new(uipc_handle* handle, UipcMessageType type, size_t max_size)
{
	uipc_message* message = malloc(sizeof (uipc_message));
	
    if (!message)
        return NULL;

	message->handle = handle;
	message->next = NULL;
	message->refcount = 1;
    message->acked = false;
	message->max_size = max_size;
    message->type = type;	

    /* FIXME: don't use asprintf */
    if (asprintf(
            (char **) &message->shmem_path, 
            "/uipc_%i_%i_%i", getpid(), 
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
uipc_msg_alloc(uipc_message* message, size_t size)
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

UipcStatus
uipc_msg_send(uipc_message* message)
{
	message->refcount++;
	queue_message(message);

    return UIPC_SUCCESS;
}

void
uipc_msg_free(uipc_message* message)
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
uipc_msg_pointer(uipc_message* message, void* offset)
{
	return ((long) offset + message->shmem_memory);
}

void*
uipc_msg_offset(uipc_message* message, void* pointer)
{
	return ((void*) (pointer - message->shmem_memory));
}

UipcMessageType
uipc_msg_get_type(uipc_message* message)
{
    return message->type;
}

void*
uipc_msg_payload_get(uipc_message* message, uipc_typeinfo* info)
{
	uipc_unmarshal_payload(message->shmem_memory, message->max_size, message->payload, info); 
	return message->payload;
}

void
uipc_msg_payload_set(uipc_message* message, void* payload, uipc_typeinfo* info)
{
	message->payload = payload;
	uipc_marshal_payload(message->shmem_memory, message->max_size, message->payload, info); 
}
