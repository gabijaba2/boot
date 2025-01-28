/*
 * Boot console support 
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

UINT8 *ConsoleBuffer = NULL;
UINTN BufferWidth = 0;
UINTN BufferHeight = 0;
UINTN CurrentLine = 0;
UINTN CurrentColumn = 0;
EFI_GRAPHICS_OUTPUT_PROTOCOL *GlobalGraphicsOutput = NULL;
FONT *GlobalFont = NULL;

EFI_STATUS boot_render_font(FONT *Font, UINT32 BackgroundColor, UINT32 ForegroundColor)
{
  EFI_STATUS Status = EFI_SUCCESS;

  /* Allocate memory for rendered font */
  Status = gBS->AllocatePool(EfiLoaderData, Font->Width * Font->Height * sizeof(UINT32) * Font->NumberOfSymbols, (VOID**)&Font->RenderedFont);
  if(EFI_ERROR(Status)) {
    Print(L"Error while allocating memory for rendered font\n");
    return Status;
  }
  
  /* Render font */
  for(UINTN Index = 0; Index < Font->NumberOfSymbols; Index++) {
    UINT8 *CurrentSymbol = Font->Bitmap + (Index * Font->BitmapHeight * Font->BitmapWidth);
    UINT32 *CurrentSymbolBuffer = Font->RenderedFont + (Index * Font->Height * Font->Width);

    for (UINT32 RowIndex = 0; RowIndex < Font->Height; RowIndex++) {
        UINT16 Row = *(UINT16*)((UINTN)CurrentSymbol + RowIndex * Font->BitmapWidth);

#if defined (_M_X64) || defined (_M_AMD64) || defined(__x86_64__)
        /* amd64 is little endian */
        
        Row = (Row >> 8) | (Row << 8);
#endif
        for (UINT32 ColumnIndex = 0; ColumnIndex < 16; ColumnIndex++) {
          if (Row & (1 << (15 - ColumnIndex))) {
            *(CurrentSymbolBuffer + (RowIndex * Font->Width) + ColumnIndex) = ForegroundColor;
          } else {
            *(CurrentSymbolBuffer + (RowIndex * Font->Width) + ColumnIndex) = BackgroundColor;
          }
        }
    }
  }
  
  return Status;
}

#if 0
VOID boot_render_char(FONT *Font, UINT32 ForegroundColor, UINT32 BackgroundColor, UINT32 CharacterIndex, UINT32 X, UINT32 Y)
{
    UINT8 *CurrentSymbol = Font->Bitmap + (CharacterIndex * 22 * 2);

    for (UINT32 RowIndex = 0; RowIndex < 22; RowIndex++) {
        UINT16 Row = *(UINT16*)((UINTN)CurrentSymbol + RowIndex * 2);

#if defined (_M_X64) || defined (_M_AMD64) || defined(__x86_64__)
        /* amd64 is little endian */
        
        Row = (Row >> 8) | (Row << 8);
#endif
        for (INT32 ColumnIndex = 0; ColumnIndex < 16; ColumnIndex++) {
            if (Row & (1 << (15 - ColumnIndex))) {
                *((UINT32*)GlobalGraphicsOutput->Mode->FrameBufferBase + (Y + RowIndex) * GlobalGraphicsOutput->Mode->Info->PixelsPerScanLine + (X + ColumnIndex)) = ForegroundColor;
            } else {
                *((UINT32*)GlobalGraphicsOutput->Mode->FrameBufferBase + (Y + RowIndex) * GlobalGraphicsOutput->Mode->Info->PixelsPerScanLine + (X + ColumnIndex)) = BackgroundColor;
            }
        }
    }
}
#endif

EFI_STATUS boot_console_init(EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput, FONT *Font)
{
  EFI_STATUS Status = EFI_SUCCESS;
  
  /* With a 12x22 font 1920x1080 would be 160x49*/
  BufferWidth = GraphicsOutput->Mode->Info->HorizontalResolution / Font->Width;
  BufferHeight = GraphicsOutput->Mode->Info->VerticalResolution / Font->Height;
  
  /* Allocate memory for console buffer */
  Status = gBS->AllocatePool(EfiLoaderData, BufferWidth * BufferHeight * sizeof(UINT8), (VOID**)&ConsoleBuffer);
  if(EFI_ERROR(Status) || !ConsoleBuffer) {
    Print(L"Error while allocating memory for the console buffer\n");
    return Status;
  }
  
  gBS->SetMem((VOID*)ConsoleBuffer, BufferWidth * BufferHeight * sizeof(UINT8), 17);
  
  Status = boot_render_font(Font, 0xffffffff, 0x0);
  if(EFI_ERROR(Status)) {
      Print(L"Error while rendering font\n");
      return Status;
  }
  
  GlobalFont = Font;
  GlobalGraphicsOutput = GraphicsOutput;
  
  return Status;
}

VOID boot_draw_console_buffer()
{
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL ClearColor = {0};
	ClearColor.Blue = 0xFF;
	ClearColor.Green = 0xFF;
	ClearColor.Red = 0xFF;

  GlobalGraphicsOutput->Blt(
    GlobalGraphicsOutput,
    &ClearColor,
    EfiBltVideoFill,
    0,
    0,
    0,
    0,
    GlobalGraphicsOutput->Mode->Info->HorizontalResolution,
    GlobalGraphicsOutput->Mode->Info->VerticalResolution,
    0);


  for(UINTN RowIndex = 0; RowIndex < BufferHeight; RowIndex++) {
    for(UINTN ColumnIndex = 0; ColumnIndex < BufferWidth; ColumnIndex++) {
    
      GlobalGraphicsOutput->Blt(
        GlobalGraphicsOutput,
        (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)(GlobalFont->RenderedFont + ConsoleBuffer[RowIndex * BufferWidth + ColumnIndex]*16*22),
        EfiBltBufferToVideo,
        0,
        0,
        ColumnIndex * GlobalFont->Width,
        RowIndex * GlobalFont->Height,
        GlobalFont->Width,
        GlobalFont->Height,
        0);
    
    }
  
  }

}

VOID boot_print(CHAR8 *Format, ...)
{
  UINTN StringLength = 0;
  VA_LIST Args;
  //CHAR8 Buffer[1024] = {0};
  CHAR8 *Buffer;
  gBS->AllocatePool(EfiLoaderData, 1024, (VOID**)&Buffer);
  
  if(!Buffer) {
    Print(L"Debug, Buffer is null");
    gBS->Stall(3000000);
    return;
  }
  AsciiPrint(Format);
  gBS->Stall(1000000);
  
  VA_START(Args, Format);
  UINTN Debug = AsciiVSPrint(Buffer, 1024, Format, Args);
  Print(L"Value of debug: %u\n", Debug);
  AsciiPrint(Buffer);
  gBS->Stall(3000000);
    
  VA_END(Args);
  
  StringLength = AsciiStrLen(Buffer);
  
  if( StringLength == 0 )
    return;

  for(UINTN Index = 0; Index < StringLength; Index++) {
    if(Buffer[Index] >= 32 && Buffer[Index] <= 127) {
      ConsoleBuffer[CurrentLine * BufferWidth + CurrentColumn] = Buffer[Index] - 15;
      CurrentColumn++;
      
    } else if(Buffer[Index] == 8) { /* Backspace */
      CurrentColumn--;
      ConsoleBuffer[CurrentLine * BufferWidth + CurrentColumn] = 17;
      
    } else if(Buffer[Index] == 10) { /* Line feed */
      CurrentColumn = 0;
      CurrentLine++;
      
    } else if(Buffer[Index] == 13) { /* Carriage feed */
      CurrentColumn = 0;
      
    } else if(Buffer[Index] >= 128 && Buffer[Index] <= 159) {
      ConsoleBuffer[CurrentLine * BufferWidth + CurrentColumn] = Buffer[Index] - 128;
      CurrentColumn++;
      
    } else if(Buffer[Index] >= 160 && Buffer[Index] <= 239) {
      ConsoleBuffer[CurrentLine * BufferWidth + CurrentColumn] = Buffer[Index] - 48;
      CurrentColumn++;
    }
  
    if(CurrentColumn > BufferWidth) {
      CurrentColumn = 0;
      CurrentLine++;
    }
    
    if(CurrentLine > BufferHeight) {
      gBS->CopyMem((VOID*)ConsoleBuffer, (VOID*)(ConsoleBuffer + BufferWidth), BufferWidth * BufferHeight - BufferWidth);
      gBS->SetMem((VOID*)(ConsoleBuffer + (BufferWidth * BufferHeight - BufferWidth)), BufferWidth, 17);
      CurrentLine--;
    }
  
  }
  
  boot_draw_console_buffer();

}
