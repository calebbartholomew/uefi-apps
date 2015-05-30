#include "DNSClientImpl.h"

/**
  Creates and initalizes the DNSClient's private data.

  @param[in] Instance  The Private data to be used.

  @retval EFI_SUCCESS  The Private variable has been initalized successfully.
  @retval other        An error occured. Private is unitalized.
  */
EFI_STATUS EFIAPI CreateDNSClient(DNSCLIENT_PRIVATE_DATA *Instance) {
  EFI_STATUS                               Status;
  EFI_HANDLE                               *HandleBuffer;
  UINTN                                    HandleCount;

  HandleBuffer = NULL;
  HandleCount  = 0;
  Status       = EFI_ABORTED;

  if(Instance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance->Udp4Sb     = NULL;
  Instance->IdIterator = 0;

  //
  // Retrieve the list of handles that support the Udp4ServiceBindingProtocol.
  //
  Status = gBS->LocateHandleBuffer(
    ByProtocol,
    &gEfiUdp4ServiceBindingProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
  );

  //
  // Exit if there was an error, or we don't have any handles.
  //
  if(EFI_ERROR(Status) || (HandleCount == 0) || (HandleBuffer == NULL)) {
    return EFI_ABORTED;
  }

  //
  // Select the first handle then release the buffer.
  //
  Instance->Udp4ServiceHandle = HandleBuffer[0];

  SafeRelease(HandleBuffer);

  //
  // Open the Udp4ServiceBindingProtocol so we can create child handles.
  //
  Status = gBS->OpenProtocol(
    Instance->Udp4ServiceHandle,
    &gEfiUdp4ServiceBindingProtocolGuid,
    (VOID **) &Instance->Udp4Sb,
    Instance->Image,
    Instance->Udp4ServiceHandle,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
  );

  if(EFI_ERROR(Status)) {
    goto ON_ERROR;
  }

  //
  // Crate a Udp4Protocol handle for reading.
  //
  Status = Instance->Udp4Sb->CreateChild(Instance->Udp4Sb, &Instance->Udp4Child);

  if(EFI_ERROR(Status)) {
    goto ON_ERROR;
  }

  //
  // Open our Udp4Protocol read handle.
  //
  Status = gBS->OpenProtocol(
    Instance->Udp4Child,
    &gEfiUdp4ProtocolGuid,
    (VOID **) &Instance->Udp4,
    Instance->Image,
    &Instance->Udp4ServiceHandle,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
  );

  if(EFI_ERROR(Status)) {
    goto ON_ERROR;
  }

  // For details on what these following values mean see the related 
  // definition section of EFI_UDP4_PROTOCOL.GetModeData() of the
  // UEFI spec document (pg 1408 in UEFI_2_4_Errata_B.pdf of April, 2014).
  ZeroMem (&Instance->Udp4CfgData, sizeof (EFI_UDP4_CONFIG_DATA));
  Instance->Udp4CfgData.AcceptBroadcast    = FALSE;
  Instance->Udp4CfgData.AcceptPromiscuous  = FALSE;
  Instance->Udp4CfgData.AcceptAnyPort      = FALSE;
  Instance->Udp4CfgData.AllowDuplicatePort = TRUE;
  Instance->Udp4CfgData.TypeOfService      = 0;
  Instance->Udp4CfgData.TimeToLive         = 16;
  Instance->Udp4CfgData.DoNotFragment      = FALSE;
  Instance->Udp4CfgData.ReceiveTimeout     = 50000;
  Instance->Udp4CfgData.UseDefaultAddress  = TRUE;
  Instance->Udp4CfgData.StationPort        = 53;
  Instance->Udp4CfgData.RemotePort         = 53;

  Status = Instance->Udp4->Configure(Instance->Udp4, &Instance->Udp4CfgData);

  if(EFI_ERROR(Status)){
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

 ON_ERROR:

  SafeRelease(HandleBuffer);

  if((Instance->Udp4Sb != NULL)  && (Instance->Udp4Child != NULL)) {
    Instance->Udp4Sb->DestroyChild(Instance->Udp4Sb, Instance->Udp4Child);
  }

  return Status;
} // End of DNSClient


/**
  Cleans up and destroys the DNSClient's private data.

  @param[in] Instance  The Private data to be used.

  @retval EFI_SUCCESS     The private variable has been destroyed successfully.
  @retval other           An error occured.  The private variable may not have been properly destroyed.
  */
EFI_STATUS EFIAPI DestroyDNSClient(DNSCLIENT_PRIVATE_DATA *Instance) {
  EFI_STATUS   Status;

  if(Instance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if(Instance->Udp4Child != NULL) {
    // These are hainging at the moment... not sure why...
    Status = Instance->Udp4->Configure(Instance->Udp4, NULL);

    if(!EFI_ERROR(Status)) {
      Instance->Udp4Sb->DestroyChild(Instance->Udp4Sb, Instance->Udp4Child);
    }
  }

  return EFI_SUCCESS;
} // End of DestoryDNSClient


/**
  Get's an ip address by a host name.

  @param[in]      Instance   The Private data to be used.
  @param[in]      Hostname   A null terminated string of the hostname to look up.
  @param[in/out]  IpAddress  The address of the hostname.

  @retval EFI_SUCCESS        The request completed successfully.
  @retval other              An error occured.
  */
EFI_STATUS EFIAPI GetHostByName(DNSCLIENT_PRIVATE_DATA *Instance, CHAR8 *Hostname, EFI_IPv4_ADDRESS *IpAddress) {
  EFI_STATUS   Status;
  DNS_PACKET   *Request;
  DNS_PACKET   *Response;
  DNS_ANSWER   *Answer;
  DNS_QUESTION Questions[0];

  if(Instance == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  if(Hostname == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if(IpAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem(IpAddress, sizeof(IpAddress));

  ZeroMem(&Questions[0], sizeof(DNS_QUESTION));

  Questions[0].QName  = HostnameToLabelFormat(Hostname, AsciiStrnLenS(Hostname, 255));
  Questions[0].QType  = HTONS(1);
  Questions[0].QClass = HTONS(1);

  Request = CreateDNSPacket(Questions, 1);

  if(Request == NULL) {
    return EFI_ABORTED;
  }

  Request->Header.Id = HTONS(++(Instance->IdIterator));
  Request->Header.Rd = 1;
  Request->Header.Ad = 1;

  // Send request
  Status = SendDNSPacket(Instance, Request, L"8.8.8.8");

  if(EFI_ERROR(Status)) {
    goto CLEANUP;
  }

  // ReceiveResponse
  Status = ReceiveDNSPacket(Instance, &Response);

  if(EFI_ERROR(Status)) {
    goto CLEANUP;
  }

  if(Response->Header.AnCount == 0) {
    GotoStatus(CLEANUP, EFI_ABORTED);
  }

  Answer = (DNS_ANSWER*)(Response->Data + sizeof(DNS_QUESTION) * Response->Header.QdCount);

  switch(Answer->Type) {
    case 1:
      CopyMem(IpAddress, &(((A_RECORD*)Answer->RData)->IpAddress), sizeof(A_RECORD));
    break;

    default:
      Print(L"Default \n");
    break;
  }
  

  Status = EFI_SUCCESS;

 CLEANUP:

  SafeRelease(Questions[0].QName);

  if(Request != NULL) {
    ReleaseDNSPacket(Request);
  }

  return Status;
} // End of GetHostByName


/**
  Creates a DNS_PACKET based off of the parameters provided.  Must call ReleaseDNSPacket to free up used memory.

  @param[in] Questions     Array of Questions in the dns request.
  @param[in] NumQuestions  Number of Questions in the array.

  @retval EFI_SUCCESS      Packet data has been successfully created.
  @retval other            An error occured.
  */
DNS_PACKET* EFIAPI CreateDNSPacket(DNS_QUESTION Questions[], UINTN NumQuestions) {
  DNS_PACKET  *Packet;
  UINTN       TotalStringSizes;
  UINTN       i, len;

  Packet      = AllocateZeroPool(sizeof(DNS_PACKET));

  if(Packet == NULL) {
    return NULL;
    //return EFI_OUT_OF_RESOURCES;
  }

  TotalStringSizes = 0;

  for(i = 0; i < NumQuestions; ++i) {
    TotalStringSizes += Questions[i].QName[0];
  }

  Packet->DataLength = TotalStringSizes + (sizeof(DNS_QUESTION) - sizeof(CHAR8*)) * NumQuestions;

  Packet->Data = AllocateZeroPool(Packet->DataLength);

  if(Packet->Data == NULL) {
    SafeRelease(Packet);

    return NULL;
    //return EFI_OUT_OF_RESOURCES;
  }

  len = 0;

  for(i = 0; i < NumQuestions; ++i) {
    CopyMem(Packet->Data + len, &Questions[i].QName[1], Questions[i].QName[0]);
    len += Questions[i].QName[0];

    CopyMem(Packet->Data + len, &Questions[i].QType, sizeof(DNS_QUESTION) - sizeof(Questions[i].QName));
    len += sizeof(DNS_QUESTION) - sizeof(Questions[i].QName); 
  }

  Packet->Header.QdCount = HTONS(NumQuestions);

  return Packet;
} // End of CreateDNSPacket


/**
  Release a DNS_PACKET.

  @param[in] Packet        The Packet to free data on.

  @retval EFI_SUCCESS      Data has been freed.
  @retval other            An error occured.
  */
EFI_STATUS EFIAPI ReleaseDNSPacket(DNS_PACKET *Packet) {
  if(Packet == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if(Packet->Data != NULL) {
    FreePool(Packet->Data);
  }

  FreePool(Packet);

  return EFI_SUCCESS;
} // End of ReleaseDNSPacket


/**
  Sends a DNS_PACKET synchronously.

  @param[in] Instance             Pointer to a DNSClient instance.
  @param[in] Packet               A pointer to the DNS packet to send.
  @param[in] Dst                  DNS server address.

  @retval EFI_SUCCESS             Packet sent successfully.
  @retval EFI_INVALID_PARAMETER   Instance or Packet is NULL.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
  @retval other                   The packet could not be received for some other reason.  Most likely do to a error that bubbled up from another function.
 */
EFI_STATUS EFIAPI SendDNSPacket(DNSCLIENT_PRIVATE_DATA *Instance, DNS_PACKET *Packet, CHAR16* Dst) {
  EFI_STATUS                    Status;
  EFI_UDP4_COMPLETION_TOKEN     TransmitToken;
  EFI_UDP4_TRANSMIT_DATA        TransmitData;
  EFI_UDP4_SESSION_DATA         SessionData;
  EFI_IPv4_ADDRESS              DstAddress, SrcAddress;
  EFI_UDP4_FRAGMENT_DATA        FragmentData;
  BOOLEAN                       IsDone;
  UINT8                         *Buffer;
  UINTN                         i;

  if(Instance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if(Packet == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Buffer = AllocateZeroPool(sizeof(DNS_HEADER) + Packet->DataLength);

  if(Packet == NULL) {
    GotoStatus(CLEANUP, EFI_OUT_OF_RESOURCES);
  }

  CopyMem(Buffer, Packet, sizeof(DNS_HEADER));
  CopyMem(Buffer + sizeof(DNS_HEADER), Packet->Data, Packet->DataLength);

  ZeroMem(&SrcAddress,    sizeof(EFI_IPv4_ADDRESS));
  ZeroMem(&DstAddress,   sizeof(EFI_IPv4_ADDRESS));
  ZeroMem(&TransmitToken, sizeof(EFI_UDP4_COMPLETION_TOKEN));
  ZeroMem(&SessionData,   sizeof(EFI_UDP4_SESSION_DATA));
  ZeroMem(&TransmitData,  sizeof(EFI_UDP4_TRANSMIT_DATA));

  Status = NetLibStrToIp4(Dst, &DstAddress);

  if(EFI_ERROR(Status)) {
    goto CLEANUP;
  }

  //
  // Prepare session data for transmission.
  //
  SessionData.SourceAddress      = SrcAddress;
  SessionData.SourcePort         = 53;
  SessionData.DestinationAddress = DstAddress;
  SessionData.DestinationPort    = 53;

  //
  // Prepare fragment data for transmission.
  //
  FragmentData.FragmentLength    = sizeof(DNS_HEADER) + Packet->DataLength;
  FragmentData.FragmentBuffer    = (VOID *) Buffer;

  //
  // Setup transmit data.
  //
  TransmitData.UdpSessionData    = &SessionData;
  TransmitData.FragmentCount     = 1;
  TransmitData.FragmentTable[0]  = FragmentData;
  TransmitData.DataLength        = FragmentData.FragmentLength;

  TransmitToken.Packet.TxData    = &TransmitData;

  Status = gBS->CreateEvent(
    EVT_NOTIFY_SIGNAL,
    TPL_CALLBACK,
    DNSImplGenericCallback,
    (VOID*) &IsDone,
    &TransmitToken.Event
  );

  if(EFI_ERROR(Status)) {
    goto CLEANUP;
  }

  IsDone = FALSE;

  Status = Instance->Udp4->Transmit(Instance->Udp4, &TransmitToken);

  //
  // Potential infinate loop.  Need some sort of check here.
  //
  while(!IsDone) {
    Status = Instance->Udp4->Poll(Instance->Udp4);
  }

 CLEANUP:
  SafeRelease(Buffer);

  if(TransmitToken.Event != NULL) {
    gBS->CloseEvent(TransmitToken.Event);
  }

  // CLEAN UP STUFF MADE HERE!

  return Status;
} // End of SendDNSPacket


/**
  Recieves a DNS_PACKET synchronously.
  Must call ReleaseDNSPacket when done.

  @param[in] Instance             Pointer to a DNSClient instance.
  @param[in] Packet               A pointer to the vairable that will contain the address of the received packet.
 
  @retval EFI_SUCCESS             Packet received successfully.
  @retval EFI_INVALID_PARAMETER   Instance or Packet is NULL.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
  @retval other                   The packet could not be received for some other reason.  Most likely do to a error that bubbled up from another function.
 */
EFI_STATUS EFIAPI ReceiveDNSPacket(DNSCLIENT_PRIVATE_DATA *Instance, DNS_PACKET **Packet) {
  EFI_STATUS                    Status;
  EFI_UDP4_COMPLETION_TOKEN     ReceiveToken;
  DNS_PACKET_DATA               *PacketData;
  DNS_QUESTION                  *Questions;
  DNS_ANSWER                    *Answers;
  UINT8                         *Buffer;
  UINT8                         *BufferIterator;
  BOOLEAN                       IsDone;
  UINTN                         i, len;

  Buffer = NULL;

  if(Instance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if(Packet == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem(&ReceiveToken,  sizeof(EFI_UDP4_COMPLETION_TOKEN));

  IsDone = FALSE;

  Status = gBS->CreateEvent(
    EVT_NOTIFY_SIGNAL,
    TPL_CALLBACK,
    DNSImplGenericCallback,
    (VOID*) &IsDone,
    &ReceiveToken.Event
  );

  if(EFI_ERROR(Status)) {
    goto CLEANUP;
  }

  Status = Instance->Udp4->Receive(Instance->Udp4, &ReceiveToken);

  if(EFI_ERROR(Status)) {
    goto CLEANUP;
  }

  //
  // Need to have some sort of timeout and retry if
  // we don't get a response.  Also we need to do
  // Instance->Udp4->Cancel(Private->Udp4, &receiveToken);
  //
  while(!IsDone) {
    //
    // Try to receive a packet.
    //
    Status = Instance->Udp4->Poll(Instance->Udp4);

    if(EFI_ERROR(Status) && Status != EFI_NOT_READY) {
      Print(L"  Poll failed!\n");
      goto CLEANUP;
    }
  }

  Status = EFI_SUCCESS;

  Buffer = AllocateZeroPool(sizeof(DNS_HEADER) + ReceiveToken.Packet.RxData->DataLength);

  if(Buffer == NULL) {
    gBS->SignalEvent(ReceiveToken.Packet.RxData->RecycleSignal);
    GotoStatus(CLEANUP, EFI_OUT_OF_RESOURCES);
  }

  len = 0;

  //
  // Copy our token data into our new buffer.
  //

  for(i = 0; i < ReceiveToken.Packet.RxData->FragmentCount; ++i) {
    UINTN j;

    for(j = 0; j < ReceiveToken.Packet.RxData->FragmentTable[i].FragmentLength; ++j, ++len) {
      Buffer[len] = ((UINT8*)ReceiveToken.Packet.RxData->FragmentTable[i].FragmentBuffer)[j];
    }
  }

  gBS->SignalEvent(ReceiveToken.Packet.RxData->RecycleSignal);

  *Packet = AllocateZeroPool(sizeof(DNS_PACKET));

  if(*Packet == NULL) {
    GotoStatus(CLEANUP, EFI_OUT_OF_RESOURCES);
  }

  CopyMem(*Packet, Buffer, sizeof(DNS_HEADER));

  //
  // If errors occur here, FreePool(*Packet) and set *Packet = NULL
  //

  (*Packet)->Header.Id      = HTONS((*Packet)->Header.Id);
  (*Packet)->Header.QdCount = HTONS((*Packet)->Header.QdCount);
  (*Packet)->Header.AnCount = HTONS((*Packet)->Header.AnCount);
  (*Packet)->Header.NsCount = HTONS((*Packet)->Header.NsCount);
  (*Packet)->Header.ArCount = HTONS((*Packet)->Header.ArCount);

  PacketData = AllocateZeroPool(
    sizeof(DNS_QUESTION) * (*Packet)->Header.QdCount +
    sizeof(DNS_ANSWER)   * (*Packet)->Header.AnCount
  );

  if(PacketData == NULL) {
    SafeRelease(*Packet);
    GotoStatus(CLEANUP, EFI_OUT_OF_RESOURCES);
  }

  (*Packet)->Data = PacketData;

  Questions = (DNS_QUESTION*)(PacketData);
  Answers   = (DNS_ANSWER*)(PacketData + (*Packet)->Header.QdCount * sizeof(DNS_QUESTION));

  BufferIterator = Buffer + sizeof(DNS_HEADER);

  for(i = 0; i < (*Packet)->Header.QdCount; ++i) {
    //
    // Check to see if the name is using pointer format.
    //
    if( (*BufferIterator & 0xC0) == 0xC0 ) {
      UINT16 namePtr = (((*BufferIterator << 8) ^ 0xC0) | *(BufferIterator+1));

      //
      // Name uses pointer format
      //
      Questions[i].QName = LabelFormatToHostname((CHAR8*)(Buffer + namePtr));
      
      //
      // Skip past the pointer format name.
      //
      BufferIterator += 2;
    } else {
      //
      // Name uses label format.
      //
      Questions[i].QName = LabelFormatToHostname((CHAR8*)BufferIterator);

      //
      // Skip past the label format name.
      //
      while(*(BufferIterator++) != (CHAR8) 0);
    }

    if(Questions[i].QName == NULL) {
      SafeRelease(*Packet);
      GotoStatus(CLEANUP, EFI_OUT_OF_RESOURCES);
    }

    Questions[i].QType = HTONS(*((UINT16*)BufferIterator));
    BufferIterator += 2;

    Questions[i].QClass = HTONS(*((UINT16*)BufferIterator));
    BufferIterator += 2;
  }

  for(i = 0; i < (*Packet)->Header.AnCount; ++i) {
    //
    // Check to see if the name is using pointer format.
    //
    if( (*BufferIterator & 0xC0) == 0xC0 ) {
      UINT16 namePtr = (((*BufferIterator << 8)) | *(BufferIterator+1));

      namePtr = namePtr ^ 0xC000;

      //
      // Name uses pointer format.
      //
      Answers[i].Name = LabelFormatToHostname((CHAR8*)(Buffer + namePtr));
      
      //
      // Skip past the pointer format name.
      //
      BufferIterator += 2;
    } else {
      //
      // Name uses label format.
      //
      Answers[i].Name = LabelFormatToHostname((CHAR8*)BufferIterator);

      //
      // Skip past the label format name.
      //
      while(*(BufferIterator++) != (CHAR8) 0);
    }

    if(Answers[i].Name == NULL) {
      SafeRelease(*Packet);
      GotoStatus(CLEANUP, EFI_OUT_OF_RESOURCES);
    }

    Answers[i].Type = HTONS(*((UINT16*)BufferIterator));
    BufferIterator += 2;

    Answers[i].Class = HTONS(*((UINT16*)BufferIterator));
    BufferIterator += 2;

    Answers[i].TTL = HTONS(*((UINT32*)BufferIterator));
    BufferIterator += 4;

    Answers[i].RdLength = HTONS(*((UINT16*)BufferIterator));
    BufferIterator += 2;


    // Handle RDATA based off of type.
    // Right now we're only going ot support A records.
    switch(Answers[i].Type) {
      case 1:
        Answers[i].RData = AllocateZeroPool(sizeof(A_RECORD));

        if(Answers[i].RData == NULL) {
          SafeRelease(*Packet);
          GotoStatus(CLEANUP, EFI_OUT_OF_RESOURCES);
        }

        CopyMem(Answers[i].RData, BufferIterator, sizeof(A_RECORD));
      break;

      default:
      break;
    }
  }

  Status = EFI_SUCCESS;

 CLEANUP:

 if(ReceiveToken.Event != NULL) {
    gBS->CloseEvent(ReceiveToken.Event);
  }

  SafeRelease(Buffer);

  return Status;
} // End of ReceiveDNSPacket


/**
  Sets a boolean to true.
  This function should not be called directly, but is intended to be used
  with the UEFI event system.

  @param[in] Event        The event that was triggered.
  @param[in] Context      The boolean to set.
 */
VOID EFIAPI DNSImplGenericCallback(IN EFI_EVENT Event, IN VOID *Context) {
  *((BOOLEAN*)Context) = TRUE;
} // End of DNSImplGenericCallback


/**
  Converts a hostname to DNS label format.
  Must call FreePool when done with the string.

  @param[in] Hostname     The hostname string to convert.
  @param[in] Length       The length of the Hostname string.

  @retval NULL            Hostname is NULL or out of memory.
  @retval CHAR8*          Pointer to newly created label format string.
  */
CHAR8* EFIAPI HostnameToLabelFormat(CHAR8* Hostname, UINTN Length) {
  CHAR8* LabelFormat;
  UINT8  i, len;

  if(Hostname == NULL) {
    return NULL;
  }

  LabelFormat = AllocateZeroPool(sizeof(CHAR8) * (Length+3));

  if(LabelFormat == NULL) {
    return NULL;
  }

  for(i = Length, len = 0; i > 0; --i) {
    if(Hostname[i-1] == '.') {
      LabelFormat[i+1] = len;
      len = 0;
      continue;
    }

    ++len;
    LabelFormat[i+1] = Hostname[i-1];
  }

  LabelFormat[Length+2] = (CHAR8)  0;
  LabelFormat[0]        = Length + 2;
  LabelFormat[1]        = len;

  return LabelFormat;
}


/**
  Converts a DNS label format to a Hostname string.  
  Must call FreePool when done with the string.

  @param[in] LabelFormat  The label format string to convert.

  @retval NULL            LabelFormat is NULL or out of memory.
  @retval CHAR8*          Pointer to newly created string.
  */
CHAR8* EFIAPI LabelFormatToHostname(CHAR8* LabelFormat) {
  CHAR8  *Hostname;
  CHAR8  *Label;
  UINTN  Length, NumLables, LabelSize;
  UINTN  i, j, k;

  if(LabelFormat == NULL) {
    return NULL;
  }

  Length    = 0;
  NumLables = 0;
  
  i = 0;
  do {
    ++NumLables;

    Length += LabelFormat[i];

    i += LabelFormat[i] + 1;
  } while(LabelFormat[i] != (CHAR8) 0);

  Hostname = AllocateZeroPool(Length + NumLables);

  if(Hostname == NULL) {
    return NULL;
  }

  Hostname[Length] = (CHAR8) 0;

  Label = LabelFormat;
  for(i = 0, j = -1; i < NumLables; ++i) {
    LabelSize = Label[0];

    if(i != 0) {
      Hostname[++j] = '.';
    }

    for(k = 0; k < LabelSize; ++k) {
      Hostname[++j] = Label[k + 1];
    }

    Label += LabelSize + 1;
  }

  return Hostname;
}