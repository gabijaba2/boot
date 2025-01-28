/*
 * Boot error handling
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


#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>

#include "Boot.h"

extern EFI_HANDLE BootImageHandle;
extern EFI_SYSTEM_TABLE *BootSystemTable;

VOID boot_error(EFI_STATUS ExitStatus, CHAR8 *Msg, ...)
{
	EFI_STATUS Status = EFI_SUCCESS;
	UINTN Index;
	EFI_INPUT_KEY Key;
	VA_LIST Args;
	CHAR8 Buffer[1024];
  
	VA_START(Args, Msg);
  
	AsciiSPrint(Buffer, 1024, Msg, Args);
  
	boot_print(Buffer);
	boot_print("Press any key to return to firmware\n");
  
	VA_END(Args);
  
	while(1) {
		gBS->WaitForEvent(1, BootSystemTable->ConIn->WaitForKey, &Index);
    
		Status = BootSystemTable->ConIn->ReadKeyStroke(BootSystemTable->ConIn, &Key); 
		if(EFI_ERROR(Status)) {
			if(Status == EFI_NOT_READY) {
				continue;
			} else {
				break;
			}
		}
    
		break;
	}
  
	gBS->Exit(BootImageHandle, ExitStatus, 0, NULL);

}
