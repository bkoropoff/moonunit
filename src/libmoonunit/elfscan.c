#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#undef _GNU_SOURCE
#include "elfscan.h"
#include <elf.h>
#include <libelf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

static void*
library_base_address(void *handle)
{
	void* func;
	Dl_info info;
	
	// Grab a function with dynamic linkage which is typically present
	if (!(func = dlsym(handle, "_init")))
		return NULL;
	
	// Use function pointer to get information on library
	if (!dladdr(func, &info))
		return NULL;
	
	// Return base address of library
	return info.dli_fbase;
}

static const char*
library_path(void *handle)
{
	void* func;
	Dl_info info;
	
	// Grab a function with dynamic linkage which is typically present
	if (!(func = dlsym(handle, "_init")))
		return NULL;
	
	// Use function pointer to get information on library
	if (!dladdr(func, &info))
		return NULL;
	
	// Return path of library
	return info.dli_fname;
}

static inline void
libelf_scan_symtab(void* handle, SymbolFilter filter, SymbolCallback callback, void* data,
				   Elf* elf, Elf_Scn* section, Elf32_Shdr *shdr, bool dynamic)
{
	Elf_Data *edata = NULL;
	Elf32_Sym* sym = NULL;
	Elf32_Sym* last_sym = NULL;
	void* base_address = dynamic ? NULL : library_base_address(handle);
	
	if (!dynamic && !base_address)
		return;
	
	if (!(edata = elf_getdata(section, edata)) || (edata->d_size == 0))
		return;
		
	sym = (Elf32_Sym*) edata->d_buf;
	last_sym = (Elf32_Sym*) ((char*) edata->d_buf + edata->d_size);
	
	for (; sym < last_sym; sym++)
	{
		symbol info;
		
		info.name = elf_strptr(elf, shdr->sh_link, (size_t) sym->st_name);
		
		if (filter(info.name, data))
		{
			if (dynamic)
			{
				info.addr = dlsym(handle, info.name);
			}
			else
			{
				info.addr = base_address + (unsigned long) sym->st_value;	
			}
			callback(&info, data);
		}
	}
}

static void
libelf_symbol_scanner(void* handle, SymbolFilter filter, SymbolCallback callback, void* data)
{
	const char* filename = library_path(handle);
	int fd = open(filename, O_RDONLY);
	Elf* elf;
	Elf_Scn* section = NULL;
	
	if (elf_version(EV_CURRENT) == EV_NONE ) 
		return;
		
	if (!filename || fd < 0)
		return;
	
	if (!(elf = elf_begin(fd, ELF_C_READ, NULL)))
	{
		close(fd);
		return;
	}
	
	while ((section = elf_nextscn(elf, section)))
	{
		Elf32_Shdr *shdr;
		
		if ((shdr = elf32_getshdr(section)))
		{
			switch (shdr->sh_type)
			{
				case SHT_SYMTAB:
					libelf_scan_symtab(handle, filter, callback, data, elf, section, shdr, false);
					continue;
				case SHT_DYNSYM:
					libelf_scan_symtab(handle, filter, callback, data, elf, section, shdr, true);
					continue;
			}
		}
	}
	
	elf_end(elf);
	close(fd);
	return;
}

SymbolScanner 
ElfScan_GetScanner()
{
	return libelf_symbol_scanner;
}
