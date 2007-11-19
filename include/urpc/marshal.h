#ifndef __URPC_MARSHAL_H__
#define __URPC_MARSHAL_H__

typedef struct __urpc_typeinfo
{
	unsigned int num_pointers;
	struct { 
		unsigned long offset; 
		struct __urpc_typeinfo* info;
	} pointers[];
} urpc_typeinfo;

#define URPC_OFFSET(type, field) ((unsigned long) &((type*)0)->field)
#define URPC_POINTER(type, field, info) {URPC_OFFSET(type, field), info}

void urpc_marshal_payload(void* membase, unsigned int memsize, void* payload, urpc_typeinfo* payload_type);
void urpc_unmarshal_payload(void* membase, unsigned int memsize, void* payload, urpc_typeinfo* payload_type);

#endif
