#ifndef __URPC_RPC_H__
#define __URPC_RPC_H__

#include "marshal.h"

#include <stdlib.h>

struct __urpc_handle;
typedef struct __urpc_handle urpc_handle;

struct __urpc_message;
typedef struct __urpc_message urpc_message;

urpc_handle* urpc_connect(int socket);
void urpc_process(urpc_handle* handle);
urpc_message* urpc_read(urpc_handle* handle);
urpc_message* urpc_msg_new(urpc_handle* handle, size_t max_size);
void* urpc_msg_alloc(urpc_message* message, size_t size);
void urpc_msg_send(urpc_message* message);
void urpc_msg_free(urpc_message* message);
void* urpc_msg_pointer(urpc_message* message, void* offset);
void* urpc_msg_offset(urpc_message* message, void* pointer);
void* urpc_msg_payload_get(urpc_message* message, urpc_typeinfo* info);
void urpc_msg_payload_set(urpc_message* message, void* payload, urpc_typeinfo* info);

#endif
