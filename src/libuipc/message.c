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
    uipc_message_type type;
	unsigned long size;
    void* payload;
};

struct __uipc_handle
{
    bool readable, writeable;
	int socket;
};

uipc_packet*
packet_from_message(uipc_message* message)
{
    unsigned int length = sizeof(uipc_packet_message) + message->size;
    uipc_packet* packet = malloc(sizeof (uipc_packet_header) + length);
      
    if (!packet)
        return NULL;

    packet->header.type = PACKET_MESSAGE;
    packet->header.length = length;
    packet->message.type = message->type;
    packet->message.length = message->size;
    memcpy(packet->message.payload, message->payload, message->size);
	
    return packet;
}

static uipc_message* 
message_from_packet(uipc_packet* packet)
{
	uipc_message* message = malloc(sizeof(uipc_message));
	
    if (!message)
        return NULL;

    message->type = packet->message.type;
	message->payload = malloc(packet->message.length);
    memcpy(message->payload, packet->message.payload, packet->message.length);
	message->size = packet->message.length;

    return message;
}

uipc_handle* 
uipc_attach(int socket)
{
	uipc_handle* handle = malloc(sizeof (uipc_handle));

    if (!handle)
        return NULL;
	
	handle->socket = socket;
    handle->readable = handle->writeable = true;

	return handle;
}

uipc_status
uipc_read(uipc_handle* handle, uipc_message** message)
{
    uipc_packet* packet = NULL;
    uipc_status result;

    if (!handle->readable)
        return UIPC_EOF;

    result = uipc_packet_recv(handle->socket, &packet);
        
    if (result == UIPC_EOF)
    {
        handle->readable = false;
        return UIPC_EOF;
    }
    else if (result != UIPC_SUCCESS)
    {
        return result;
    }
    else
    {
        switch (packet->header.type)
        {
        case PACKET_MESSAGE:
        {
            *message = message_from_packet(packet);
            
            free(packet);
            
            if (!*message)
            {
                handle->readable = false;
                return UIPC_NOMEM;
            }
            return UIPC_SUCCESS;
        }
        default:
            return UIPC_ERROR;
        }
    }    
}

uipc_status
uipc_waitread(uipc_handle* handle, uipc_message** message, long* timeout)
{
    uipc_status result = UIPC_SUCCESS;
    
    do
    {
        result = uipc_packet_available(handle->socket, timeout);
    } while (result == UIPC_RETRY);
    
    if (result != UIPC_SUCCESS)
        return result;
    
	return uipc_read(handle, message);
}

uipc_status
uipc_write(uipc_handle* handle, uipc_message* message)
{
    uipc_status result = UIPC_SUCCESS;
    uipc_packet* packet = NULL;

    if (!handle->writeable)
    {
        result = UIPC_EOF;
        goto cleanup;
    }

    packet = packet_from_message(message);

    result = uipc_packet_send(handle->socket, packet);
        
    if (result == UIPC_EOF)
    {
        handle->writeable = false;
        goto cleanup;
    }
    else if (result != UIPC_SUCCESS)
    {
        goto cleanup;
    }

cleanup:
    if (packet)
        free(packet);

    return result;
}

uipc_status
uipc_waitwrite(uipc_handle* handle, uipc_message* message, long* timeout)
{
    uipc_status result = UIPC_SUCCESS;
    
    do
    {
        result = uipc_packet_sendable(handle->socket, timeout);
    } while (result == UIPC_RETRY);
    
    if (result != UIPC_SUCCESS)
        return result;
    
	return uipc_write(handle, message);
}

uipc_status
uipc_detach(uipc_handle* handle)
{
    uipc_status result = UIPC_SUCCESS;

    if (!result)
        return result;
    
    free(handle);
    
    return result;
}

uipc_message* 
uipc_msg_new(uipc_message_type type)
{
    uipc_message* message = malloc(sizeof (uipc_message));
	
    if (!message)
        return NULL;

    message->type = type;
    message->payload = NULL;
    message->size = 0;

	return message;
}

void
uipc_msg_free(uipc_message* message)
{
    if (message->payload)
        free(message->payload);
    free(message);
}

uipc_message_type
uipc_msg_get_type(uipc_message* message)
{
    return message->type;
}

void*
uipc_msg_get_payload(uipc_message* message, uipc_typeinfo* info)
{
    void* object;
	uipc_unmarshal_payload(&object, message->payload, info); 

    return object;
}

void
uipc_msg_free_payload(void* payload, uipc_typeinfo* info)
{
    uipc_free_object(payload, info);
}

void
uipc_msg_set_payload(uipc_message* message, const void* payload, uipc_typeinfo* info)
{
    unsigned long actual, size = 512;
    void* buffer = malloc(size);

    actual = uipc_marshal_payload(buffer, size, payload, info); 
    
    if (actual > size)
    {
        buffer = realloc(buffer, actual);
        uipc_marshal_payload(buffer, actual, payload, info);
    }

    message->payload = buffer;
    message->size = actual;
}
