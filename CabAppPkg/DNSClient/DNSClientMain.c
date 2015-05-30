#include "DNSClientMain.h"

//
// Global Variables
//
STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {NULL, TypeMax}
};

/**
  Entry point for the DNSClient application.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
  */
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS                       Status;                    // Used to get, validate, and return status.
  DNSCLIENT_PRIVATE_DATA           *Private;                  // Stores Session data for this instance.
  EFI_IPv4_ADDRESS                 IpAddress;
  CHAR8                            *Hostname;
  LIST_ENTRY                       *Package;
  CONST CHAR16                     *Param;
  CHAR16                           *ProblemParam;

  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);

  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      Print(L"Invalid arguments.");
      //ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      GotoStatus(EXIT, SHELL_INVALID_PARAMETER);
    } else {
      ASSERT(FALSE);
    }
  } else {

  if (ShellCommandLineGetCount(Package) < 1) {
    Print(L"To few arguments.");
    //ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
    GotoStatus(EXIT, SHELL_INVALID_PARAMETER);
   } else if (ShellCommandLineGetCount(Package) > 1) {
    Print(L"To many arguments.");
    //ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle);
    GotoStatus(EXIT, SHELL_INVALID_PARAMETER);
   } 

    Param = ShellCommandLineGetRawValue(Package, 1);
    if(Param != NULL) {
      Hostname = AllocateZeroPool(StrnLenS(Param, 255));

      if(Hostname == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      UnicodeStrToAsciiStr(Param, Hostname);
    } else {
      GotoStatus(EXIT, SHELL_INVALID_PARAMETER);
    }
  }

  Private = AllocateZeroPool(sizeof(DNSCLIENT_PRIVATE_DATA));

  if(Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature = DNSCLIENT_PRIVATE_DATA_SIGNATURE;
  Private->Image     = ImageHandle;

  Status = CreateDNSClient(Private);

  if(EFI_ERROR(Status)) {
    GotoStatus(CLEANUP, EFI_ABORTED);
  }

  Status = GetHostByName(Private, Hostname, &IpAddress);

  if(EFI_ERROR(Status)) {
    goto CLEANUP;
  }

  Print(L"%a->%d.%d.%d.%d\n", Hostname, IpAddress.Addr[0], IpAddress.Addr[1], IpAddress.Addr[2], IpAddress.Addr[3]);

  Status = EFI_SUCCESS;
CLEANUP:


  DestroyDNSClient(Private);

  SafeRelease(Private);
  SafeRelease(Hostname);

  if(EFI_ERROR(Status)) {
  	Print(L"Exiting with status: (0x%X) ", Status);
  	PrintStatus(Status);
  }

EXIT:
  return Status;
}

/**
  Helper function to print EFI Statuses.
 */
VOID EFIAPI PrintStatus(EFI_STATUS status) {
  switch(status) {
  case EFI_SUCCESS:
    Print(L"EFI_SUCCESS\n");
    break;
  case EFI_LOAD_ERROR:
    Print(L"EFI_LOAD_ERROR\n");
    break;
  case EFI_INVALID_PARAMETER:
    Print(L"EFI_INVALID_PARAMETER\n");
    break;
  case EFI_UNSUPPORTED:
    Print(L"EFI_UNSUPPORTED\n");
    break;
  case EFI_BAD_BUFFER_SIZE:
    Print(L"EFI_BAD_BUFFER_SIZE\n");
    break;
  case EFI_BUFFER_TOO_SMALL:
    Print(L"EFI_BUFFER_TOO_SMALL\n");
    break;
  case EFI_NOT_READY:
    Print(L"EFI_NOT_READY\n");
    break;
  case EFI_DEVICE_ERROR:
    Print(L"EFI_DEVICE_ERROR\n");
    break;
  case EFI_WRITE_PROTECTED:
    Print(L"EFI_WRITE_PROTECTED\n");
    break;
  case EFI_OUT_OF_RESOURCES:
    Print(L"EFI_OUT_OF_RESOURCES\n");
    break;
  case EFI_VOLUME_CORRUPTED:
    Print(L"EFI_VOLUME_CORRUPTED\n");
    break;
  case EFI_VOLUME_FULL:
    Print(L"EFI_VOLUME_FULL\n");
    break;
  case EFI_NO_MEDIA:
    Print(L"EFI_NO_MEDIA\n");
    break;
  case EFI_MEDIA_CHANGED:
    Print(L"EFI_MEDIA_CHANGED\n");
    break;
  case EFI_NOT_FOUND:
    Print(L"EFI_NOT_FOUND\n");
    break;
  case EFI_ACCESS_DENIED:
    Print(L"EFI_ACCESS_DENIED\n");
    break;
  case EFI_NO_RESPONSE:
    Print(L"EFI_NO_RESPONSE\n");
    break;
  case EFI_NO_MAPPING:
    Print(L"EFI_NO_MAPPING\n");
    break;
  case EFI_TIMEOUT:
    Print(L"EFI_TIMEOUT\n");
    break;
  case EFI_NOT_STARTED:
    Print(L"EFI_NOT_STARTED\n");
    break;
  case EFI_ALREADY_STARTED:
    Print(L"EFI_ALREADY_STARTED\n");
    break;
  case EFI_ABORTED:
    Print(L"EFI_ABORTED\n");
    break;
  case EFI_ICMP_ERROR:
    Print(L"EFI_ICMP_ERROR\n");
    break;
  case EFI_TFTP_ERROR:
    Print(L"EFI_TFTP_ERROR\n");
    break;
  case EFI_PROTOCOL_ERROR:
    Print(L"EFI_PROTOCOL_ERROR\n");
    break;
  case EFI_INCOMPATIBLE_VERSION:
    Print(L"EFI_INCOMPATIBLE_VERSION\n");
    break;
  case EFI_SECURITY_VIOLATION:
    Print(L"EFI_SECURITY_VIOLATION\n");
    break;
  case EFI_CRC_ERROR:
    Print(L"EFI_CRC_ERROR\n");
    break;
  case EFI_END_OF_MEDIA:
    Print(L"EFI_END_OF_MEDIA\n");
    break;
  case EFI_END_OF_FILE:
    Print(L"EFI_END_OF_FILE\n");
    break;
  case EFI_INVALID_LANGUAGE:
    Print(L"EFI_INVALID_LANGUAGE\n");
    break;
  case EFI_COMPROMISED_DATA:
    Print(L"EFI_COMPROMISED_DATA\n");
    break;
  case EFI_WARN_UNKNOWN_GLYPH:
    Print(L"EFI_WARN_UNKNOWN_GLYPH\n");
    break;
  case EFI_WARN_DELETE_FAILURE:
    Print(L"EFI_WARN_DELETE_FAILURE\n");
    break;
  case EFI_WARN_WRITE_FAILURE:
    Print(L"EFI_WARN_WRITE_FAILURE\n");
    break;
  case EFI_WARN_BUFFER_TOO_SMALL:
    Print(L"EFI_WARN_BUFFER_TOO_SMALL\n");
    break;
  case EFI_WARN_STALE_DATA:
    Print(L"EFI_WARN_STALE_DATA\n");
    break;

  default:
    Print(L"[Unknown Error]\n");
    break;
  }
}