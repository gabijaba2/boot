/*
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
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>

#include "Elf.h"
#include "Boot.h"

typedef struct {
  UINTN Pa;             /* Physical address of page range */
  UINTN Va;             /* Virtual address of page range */
  UINTN NumberOfPages;  /* Number of pages in range */
  UINTN AddrOffset;     /* Offset from image base */
  UINTN ElfPermFlags;   /* Page range permission flags */
} ELF_SEGMENT_DESCRIPTOR;

EFI_HANDLE BootImageHandle;
EFI_SYSTEM_TABLE  *BootSystemTable;

EFI_STATUS
boot_load_elf(
  CHAR16 *FileName,
  EFI_FILE_PROTOCOL *Volume
  )
{
	EFI_STATUS Status = EFI_SUCCESS;
	EFI_FILE *ElfFile;
	VOID *FileBuffer;
	UINTN FileSize;
	ELF_HEADER64 *ElfHeader;
	ELF_PH64 *ElfProgramHeader;

  return Status;

	Status = Volume->Open(
			Volume,
			&ElfFile,
			FileName,
			EFI_FILE_MODE_READ,
			0);
	if(EFI_ERROR(Status))
	{
		Print(L"Error while opening ELF file: %S, %lx\n", FileName, Status);
		return Status;
	}
	
	ElfFile->SetPosition(ElfFile, 0xFFFFFFFFFFFFFFFF);
	ElfFile->GetPosition(ElfFile, &FileSize);
	ElfFile->SetPosition(ElfFile, 0x0);
	
	/* EfiLoaderCode perhaps? */
	Status = gBS->AllocatePool(EfiLoaderData, FileSize, &FileBuffer);
	if(EFI_ERROR(Status))
	{
		Print(L"Error while allocating memory: %S, %lx\n", FileName, Status);
		return Status;
	}

	UINTN FileSizeCopy = FileSize;

	Status = ElfFile->Read(ElfFile, &FileSize, FileBuffer);
	if(EFI_ERROR(Status))
	{
		Print(L"Error while reading ELF file: %S, %lx\n", FileName, Status);
		return Status;
	}

	if(FileSize != FileSizeCopy)
	{
		Print(L"Error while reading ELF file, not all bytes were read, expected: %lx, actually read: %lx, %S, %lx\n", FileSizeCopy, FileSize, FileName, Status);
		return EFI_LOAD_ERROR;
	}


	ElfHeader = (ELF_HEADER64*)FileBuffer;

	if(ElfHeader->IdentMag[0] != 0x7F ||
	   ElfHeader->IdentMag[1] != 0x45 ||
	   ElfHeader->IdentMag[2] != 0x4C ||
	   ElfHeader->IdentMag[3] != 0x46
	   )
	{
		Print(L"Invalid ELF header format: %S\n", FileName);
		return EFI_LOAD_ERROR;
	}

	if(ElfHeader->IdentClass != 2 || ElfHeader->IdentData != 1)
	{
		Print(L"Unsupported ELF type: %S\n", FileName);
		return EFI_LOAD_ERROR;
	}

	if(ElfHeader->IdentVersion != 1 || ElfHeader->Version != 1)
	{
		Print(L"Unsupported ELF version: %S\n", FileName);
		return EFI_LOAD_ERROR;
	}

	if(ElfHeader->IdentOsAbi != 0)
	{
		Print(L"Unsupported OS ABI: %S\n", FileName);
		return EFI_LOAD_ERROR;
	}

	if(ElfHeader->Type < 0x1 || ElfHeader->Type > 0x3)
	{
		Print(L"Invalid ELF object file type: %S\n", FileName);
		return EFI_LOAD_ERROR;
	}

	if(ElfHeader->Machine != 0x3E)
	{
		Print(L"Unsupported ELF file architecture: %S\n", FileName);
		return EFI_LOAD_ERROR;
	}

	ElfProgramHeader = (ELF_PH64*)((UINTN)ElfHeader + ElfHeader->PhOff);
	
	ELF_SEGMENT_DESCRIPTOR *SegmentArray = NULL;
	UINTN SegmentArrayUsedSize = 0;
	
	Status = gBS->AllocatePool(EfiLoaderData, ElfHeader->PhNum * sizeof(ELF_SEGMENT_DESCRIPTOR), (VOID**)&SegmentArray);
	if(EFI_ERROR(Status) || !SegmentArray)
	{
		Print(L"Error while allocating memory: %S, %lx\n", FileName, Status);
		return Status;
	}
	
	
	for(UINTN Index = 0; Index < ElfHeader->PhNum; Index++) {
		Print(L"Type: %x, Flags: %x, Offset: %lx, Vaddr: %lx, Paddr: %lx, FileSz: %lu, MemSz: %lu, Align: %lu\n", 
				ElfProgramHeader[Index].Type,
				ElfProgramHeader[Index].Flags,
				ElfProgramHeader[Index].Offset,
				ElfProgramHeader[Index].VAddr,
				ElfProgramHeader[Index].PAddr,
				ElfProgramHeader[Index].FileSz,
				ElfProgramHeader[Index].MemSz,
				ElfProgramHeader[Index].Align);
				
		UINTN BytesToWrite = ElfProgramHeader[Index].FileSz;
		UINTN BytesToZero = ElfProgramHeader[Index].MemSz - ElfProgramHeader[Index].FileSz;
		UINTN SegmentPages = (ElfProgramHeader[Index].MemSz + 4096 - 1) / 4096;
		
		Status = gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, SegmentPages, (EFI_PHYSICAL_ADDRESS *)&SegmentArray[SegmentArrayUsedSize].Pa);
		if(EFI_ERROR(Status)) {
		  Print(L"Error while allocating image memory\n");
		  return Status;
		}
		SegmentArray[SegmentArrayUsedSize].NumberOfPages = SegmentPages;
		SegmentArray[SegmentArrayUsedSize].AddrOffset = ElfProgramHeader[Index].Offset;
		SegmentArray[SegmentArrayUsedSize].ElfPermFlags = ElfProgramHeader[Index].Flags;
		
		gBS->CopyMem((VOID*)SegmentArray[SegmentArrayUsedSize].Pa, (VOID*)((UINTN)ElfHeader + ElfProgramHeader[Index].Offset), BytesToWrite);
		gBS->SetMem((VOID*)(SegmentArray[SegmentArrayUsedSize].Pa + BytesToWrite), BytesToZero, 0);
		
		SegmentArrayUsedSize++;
	}


	return Status;
}

extern UINT8 FONTDATA_12x22[];

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
	EFI_STATUS Status = EFI_SUCCESS;

	EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput;
	EFI_FILE_PROTOCOL *Volume;
	FONT Gallant;
	
	Gallant.Width = 16;
  Gallant.Height = 22;
  Gallant.BitmapWidth = 2;
  Gallant.BitmapHeight = 22;
  Gallant.NumberOfSymbols = 113 + 95;
  Gallant.Bitmap = FONTDATA_12x22;
  Gallant.RenderedFont = NULL;
	
	BootImageHandle = ImageHandle;
	BootSystemTable = SystemTable;
	
	Status = gBS->LocateProtocol(
      &gEfiGraphicsOutputProtocolGuid,
      NULL,
      (VOID**) &GraphicsOutput);
      
  if(EFI_ERROR(Status)) {
    Print(L"Error while locating EFI_GRAPHICS_OUTPUT_PROTOCOL: %lx\n", Status);
		return Status;		
  }

  Status = boot_console_init(GraphicsOutput, &Gallant);
  if(EFI_ERROR(Status)) {
    Print(L"Error while initializing boot console: %lx\n", Status);
		return Status;	
  }
  //boot_error(EFI_SUCCESS, "Error: error test\n");
  
  boot_print("Test: %d\n", 50);
  //boot_print(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\nNew line test\n");
  
  for(UINTN i = 0; i < 40; i++)
  {
    //boot_print("console scroll test :3\n");
  }
#if 0
  ConsoleBuffer[20 * CharWidth + 0] = 0;
  ConsoleBuffer[20 * CharWidth + 1] = 1;
  ConsoleBuffer[20 * CharWidth + 2] = 2;
  ConsoleBuffer[20 * CharWidth + 3] = 3;
  ConsoleBuffer[20 * CharWidth + 4] = 4;
  ConsoleBuffer[20 * CharWidth + 5] = 5;
  ConsoleBuffer[20 * CharWidth + 6] = 6;
  ConsoleBuffer[20 * CharWidth + 7] = 7;
  ConsoleBuffer[20 * CharWidth + 8] = 8;
  ConsoleBuffer[20 * CharWidth + 9] = 9;
  ConsoleBuffer[20 * CharWidth + 10] = 10;
  ConsoleBuffer[20 * CharWidth + 11] = 11;
  ConsoleBuffer[20 * CharWidth + 12] = 12;
  ConsoleBuffer[20 * CharWidth + 13] = 13;
  ConsoleBuffer[20 * CharWidth + 14] = 14;
  ConsoleBuffer[20 * CharWidth + 15] = 15;
  ConsoleBuffer[20 * CharWidth + 16] = 16;
  
  ConsoleBuffer[20 * CharWidth + 17] = 113;
  ConsoleBuffer[20 * CharWidth + 18] = 114;
  ConsoleBuffer[20 * CharWidth + 19] = 115;
  ConsoleBuffer[20 * CharWidth + 20] = 116;
  ConsoleBuffer[20 * CharWidth + 21] = 117;
  ConsoleBuffer[20 * CharWidth + 22] = 118;
  ConsoleBuffer[20 * CharWidth + 23] = 119;
  ConsoleBuffer[20 * CharWidth + 24] = 120;
  ConsoleBuffer[20 * CharWidth + 25] = 121;
  ConsoleBuffer[20 * CharWidth + 26] = 122;
  ConsoleBuffer[20 * CharWidth + 27] = 123;
  ConsoleBuffer[20 * CharWidth + 28] = 124;
  ConsoleBuffer[20 * CharWidth + 29] = 125;
  ConsoleBuffer[20 * CharWidth + 30] = 126;
  
  ConsoleBuffer[20 * CharWidth + 31] = 127;
  ConsoleBuffer[20 * CharWidth + 32] = 128;
  ConsoleBuffer[20 * CharWidth + 33] = 129;
  ConsoleBuffer[20 * CharWidth + 34] = 130;
  ConsoleBuffer[20 * CharWidth + 35] = 131;
  ConsoleBuffer[20 * CharWidth + 36] = 132;
  ConsoleBuffer[20 * CharWidth + 37] = 133;
  ConsoleBuffer[20 * CharWidth + 38] = 134;
  ConsoleBuffer[20 * CharWidth + 39] = 135;
  ConsoleBuffer[20 * CharWidth + 40] = 136;
  
  boot_draw_console_buffer();
  #endif
	Status = gBS->HandleProtocol(
			ImageHandle, 
			&gEfiLoadedImageProtocolGuid, 
			(VOID **)&LoadedImage);

	if(EFI_ERROR(Status))
	{
		Print(L"Error while locating EFI_LOADED_IMAGE_PROTOCOL: %lx\n", Status);
		return Status;		
  }

	Status = gBS->HandleProtocol(
			LoadedImage->DeviceHandle, 
			&gEfiSimpleFileSystemProtocolGuid, 
			(VOID **)&SimpleFileSystem);

	if(EFI_ERROR(Status))
	{
		Print(L"Error while locating EFI_SIMPLE_FILE_SYSTEM_PROTOCOL: %lx\n", Status);
		return Status;
	}

	Status = SimpleFileSystem->OpenVolume(
			SimpleFileSystem,
			&Volume);
	if(EFI_ERROR(Status))
	{
		Print(L"Error while opening boot volume: %lx\n", Status);
		return Status;
	}

	Status = boot_load_elf(L"kernel", Volume);
  if(EFI_ERROR(Status)) {
    Print(L"Loading ELF image %S failed, status: %lx\n", L"kernel", Status);
  }

	return Status;
}
