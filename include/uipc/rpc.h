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

#ifndef __URPC_RPC_H__
#define __URPC_RPC_H__

#include "marshal.h"
#include "status.h"

#include <stdlib.h>

struct __urpc_handle;
typedef struct __urpc_handle urpc_handle;

struct __urpc_message;
typedef struct __urpc_message urpc_message;

urpc_handle* urpc_connect(int socket);
UrpcStatus urpc_process(urpc_handle* handle);
UrpcStatus urpc_read(urpc_handle* handle, urpc_message** message);
UrpcStatus urpc_waitread(urpc_handle* handle, urpc_message** message, long* timeout);
UrpcStatus urpc_waitdone(urpc_handle* handle, long* timeout);
void urpc_disconnect(urpc_handle* handle);
urpc_message* urpc_msg_new(urpc_handle* handle, size_t max_size);
void* urpc_msg_alloc(urpc_message* message, size_t size);
UrpcStatus urpc_msg_send(urpc_message* message);
void urpc_msg_free(urpc_message* message);
void* urpc_msg_pointer(urpc_message* message, void* offset);
void* urpc_msg_offset(urpc_message* message, void* pointer);
void* urpc_msg_payload_get(urpc_message* message, urpc_typeinfo* info);
void urpc_msg_payload_set(urpc_message* message, void* payload, urpc_typeinfo* info);

#endif
