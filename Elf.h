/*
 * ELF file definitions
 *
 * Copyright (c) 2025 gabijaba.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __ELF_H__
#define __ELF_H__


#define PT_NULL 0x0
#define PT_LOAD 0x1
#define PT_DYNAMIC 0x2
#define PT_INTERP 0x3
#define PT_NOTE 0x4
#define PT_SHLIB 0x5
#define PT_PHDR 0x6
#define PT_TLS 0x7
#define PT_LOOS 0x60000000
#define PT_HIOS 0x6FFFFFFF
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7FFFFFFF

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

typedef struct {

	/*
	 *	0x0 		PT_NULL Entry unused
	 *	0x1		PT_LOAD Loadable segment
	 *	0x2		PT_DYNAMIC Dynamic linking info
	 *	0x3		PT_INTERP Interpreter info
	 *	0x4		PT_NOTE Auxilary info
	 *	0x5		PT_SHLIB Reserved
	 *	0x6		PT_PHDR Program header table
	 *	0x7		PT_TLS Thread local storage template
	 *	0x60000000	PT_LOOS OS specific range
	 *	0x6FFFFFFF	PT_HIOS ...
	 *	0x70000000	PT_LOPROC Processor specific range
	 *	0x7FFFFFFF	PT_HIPROC ...
	 */
	UINT32 Type;

	/*
	 *	0x1 PF_X Executable segment
	 *	0x2 PF_W Writeable segment
	 *	0x4 PF_R Readable segment
	 */
	UINT32 Flags;
	UINT64 Offset;
	UINT64 VAddr;
	UINT64 PAddr;
	UINT64 FileSz;
	UINT64 MemSz;
	UINT64 Align;
} ELF_PH64;

#define ET_NONE 0x0
#define ET_REL 0x1
#define ET_EXEC 0x2
#define ET_DYN 0x3
#define ET_CORE 0x4
#define ET_LOOS 0xFE00
#define ET_HIOS 0xFEFF
#define ET_LOPROC 0xFF00
#define ET_HIPROC 0xFFFF

typedef struct {
	CHAR8 	IdentMag[4];
	UINT8 	IdentClass;
	UINT8 	IdentData;
	UINT8 	IdentVersion;
	UINT8 	IdentOsAbi;
	UINT8 	IdentAbiVersion;
	UINT8 	IdentPad[7];

	/*
	 *	0x00 ET_NONE 	Unknown
	 *	0x01 ET_REL 	Relocatable file
	 *	0x02 ET_EXEC 	Executable file
	 *	0x03 ET_DYN 	Shared object
	 *	0x04 ET_CORE 	Core file
	 *	0xFE00 ET_LOOS
	 *	...		Reserved OS specific range
	 *	0xFEFF ET_HIOS
	 *	0xFF00 ET_LOPROC
	 *	...		Reserved processor specific range
	 *	0xFFFF ET_HIPROC
	 */
	UINT16	Type;

	/*
	 *	0x03 i386
	 *	0x3E amd64
	 */
	UINT16 Machine;

	UINT32 Version;
	UINT64 Entry;
	UINT64 PhOff;
	UINT64 ShOff;
	UINT32 Flags;
	UINT16 EhSize;
	UINT16 EhEntSize;
	UINT16 PhNum;
	UINT16 ShEntSize;
	UINT16 ShNum;
	UINT16 ShStrNdx;
} ELF_HEADER64;



#endif /* __ELF_H__ */
