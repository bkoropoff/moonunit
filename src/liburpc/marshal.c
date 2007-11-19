#include "marshal.h"
#include <stdio.h>

void
urpc_marshal_payload(void* membase, unsigned int memsize, void* payload, urpc_typeinfo* payload_type)
{
	int i;
	
	if (!payload_type)
		return;
	
	for (i = 0; i < payload_type->num_pointers; i++)
	{
		void** ppointer = (void**) (payload + payload_type->pointers[i].offset);
		void* pointer = *ppointer;
		
		if (pointer && pointer > membase && pointer < membase + memsize)
		{
			*ppointer -= (unsigned long) membase;
			urpc_marshal_payload(membase, memsize, pointer, payload_type->pointers[i].info);
		}
	}
}

void 
urpc_unmarshal_payload(void* membase, unsigned int memsize, void* payload, urpc_typeinfo* payload_type)
{
	int i;
	
	if (!payload_type)
		return;
	
	for (i = 0; i < payload_type->num_pointers; i++)
	{
		void** ppointer = (void**) (payload + payload_type->pointers[i].offset);
		void* pointer = *ppointer;
		
		if (pointer && pointer < (void*) memsize)
		{
			*ppointer += (unsigned long) membase;
			pointer = *ppointer;
			urpc_marshal_payload(membase, memsize, pointer, payload_type->pointers[i].info);
		}
	}
}
