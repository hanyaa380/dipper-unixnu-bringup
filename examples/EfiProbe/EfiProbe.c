#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/SimpleFileSystem.h>

STATIC CONST CHAR16 *mProbePaths[] = {
  L"\\EFI\\BOOT\\BOOTAA64.EFI",
  L"\\EFI\\UniXNU\\unixnu.toml",
  L"\\EFI\\UniXNU\\kernel.macho",
  L"\\EFI\\UniXNU\\kernel.macho.adt"
};

STATIC
VOID
ProbeVolume (
  IN UINTN                              VolumeIndex,
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    *FileSystem
  )
{
  EFI_STATUS          Status;
  EFI_FILE_PROTOCOL   *Root;
  EFI_FILE_PROTOCOL   *File;
  UINTN               PathIndex;

  Status = FileSystem->OpenVolume (FileSystem, &Root);
  if (EFI_ERROR (Status)) {
    Print (L"volume %u: OpenVolume failed: %r\r\n", VolumeIndex, Status);
    return;
  }

  Print (L"volume %u:\r\n", VolumeIndex);
  for (PathIndex = 0; PathIndex < ARRAY_SIZE (mProbePaths); PathIndex++) {
    File   = NULL;
    Status = Root->Open (
                     Root,
                     &File,
                     (CHAR16 *)mProbePaths[PathIndex],
                     EFI_FILE_MODE_READ,
                     0
                     );
    Print (L"  %s: %r\r\n", mProbePaths[PathIndex], Status);
    if (!EFI_ERROR (Status) && (File != NULL)) {
      File->Close (File);
    }
  }
  Root->Close (Root);
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_HANDLE                    *Handles;
  UINTN                          HandleCount;
  UINTN                          HandleIndex;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;

  (VOID)ImageHandle;
  (VOID)SystemTable;
  Handles     = NULL;
  HandleCount = 0;

  gST->ConOut->ClearScreen (gST->ConOut);
  Print (L"Dipper EFI filesystem probe\r\n");
  Print (L"===========================\r\n");

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  Print (L"LocateHandleBuffer(SimpleFileSystem): %r, count=%u\r\n", Status, HandleCount);
  if (EFI_ERROR (Status) || (Handles == NULL)) {
    gBS->Stall (15000000);
    return EFI_SUCCESS;
  }

  for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
    FileSystem = NULL;
    Status = gBS->HandleProtocol (
                    Handles[HandleIndex],
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID **)&FileSystem
                    );
    if (EFI_ERROR (Status) || (FileSystem == NULL)) {
      Print (L"volume %u: HandleProtocol failed: %r\r\n", HandleIndex, Status);
      continue;
    }
    ProbeVolume (HandleIndex, FileSystem);
  }

  FreePool (Handles);
  Print (L"\r\nProbe complete. Capture this screen.\r\n");
  gBS->Stall (20000000);
  return EFI_SUCCESS;
}
