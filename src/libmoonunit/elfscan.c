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
#include <config.h>

#if SIZEOF_VOIDP == 4
#define ELF_SYM_T Elf32_Sym
#define ELF_SHDR_T Elf32_Shdr
#define ELF_GETSHDR_F elf32_getshdr
#elif SIZEOF_VOIDP == 8
#define ELF_SYM_T Elf64_Sym
#define ELF_SHDR_T Elf64_Shdr
#define ELF_GETSHDR_F elf64_getshdr
#else
#error Unhandled pointer width
#endif

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
				   Elf* elf, Elf_Scn* section, ELF_SHDR_T *shdr, bool dynamic)
{
	Elf_Data *edata = NULL;
	ELF_SYM_T* sym = NULL;
	ELF_SYM_T* last_sym = NULL;
	void* base_address = dynamic ? NULL : library_base_address(handle);
	
	if (!dynamic && !base_address)
		return;
	
	if (!(edata = elf_getdata(section, edata)) || (edata->d_size == 0))
		return;
		
	sym = (ELF_SYM_T*) edata->d_buf;
	last_sym = (ELF_SYM_T*) ((char*) edata->d_buf + edata->d_size);
	
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
		ELF_SHDR_T *shdr;
		
		if ((shdr = ELF_GETSHDR_F(section)))
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
