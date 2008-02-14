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

#ifndef __UIPC_IPC_H__
#define __UIPC_IPC_H__

#include "marshal.h"
#include "status.h"

#include <stdlib.h>

struct __uipc_handle;
typedef struct __uipc_handle uipc_handle;

struct __uipc_message;
typedef struct __uipc_message uipc_message;

uipc_handle* uipc_connect(int socket);
UipcStatus uipc_process(uipc_handle* handle);
UipcStatus uipc_read(uipc_handle* handle, uipc_message** message);
UipcStatus uipc_waitread(uipc_handle* handle, uipc_message** message, long* timeout);
UipcStatus uipc_waitdone(uipc_handle* handle, long* timeout);
void uipc_disconnect(uipc_handle* handle);
uipc_message* uipc_msg_new(uipc_handle* handle, size_t max_size);
void* uipc_msg_alloc(uipc_message* message, size_t size);
UipcStatus uipc_msg_send(uipc_message* message);
void uipc_msg_free(uipc_message* message);
void* uipc_msg_pointer(uipc_message* message, void* offset);
void* uipc_msg_offset(uipc_message* message, void* pointer);
void* uipc_msg_payload_get(uipc_message* message, uipc_typeinfo* info);
void uipc_msg_payload_set(uipc_message* message, void* payload, uipc_typeinfo* info);

#endif
