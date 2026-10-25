#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  (VOID)ImageHandle;
  (VOID)SystemTable;

  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->SetAttribute (gST->ConOut, EFI_LIGHTGREEN | EFI_BACKGROUND_BLACK);
  gST->ConOut->OutputString (
                       gST->ConOut,
                       L"Dipper UEFI smoke test\r\n"
                       L"BOOTAA64.EFI executed successfully.\r\n"
                       L"\r\n"
                       L"If you can read this, FAT and EFI application loading work.\r\n"
                       );
  gST->ConOut->SetAttribute (gST->ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK);
  gBS->Stall (15000000);
  return EFI_SUCCESS;
}
