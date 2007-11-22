#ifndef __MU_INTERNAL_ELF_H__
#define __MU_INTERNAL_ELF_H__

#include <stdbool.h>

typedef struct
{
	const char* name;
	void* addr;
} symbol;

typedef void (*SymbolCallback)(symbol*, void* data);
typedef bool (*SymbolFilter)(const char*, void* data);
typedef void (*SymbolScanner)(void *, SymbolFilter, SymbolCallback, void *);

SymbolScanner ElfScan_GetScanner(void);

#endif
