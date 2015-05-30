/** @file DNSClientImpl.h
  Defines all the functions and structures needed to send and receive DNS packets.

  ************************
  * DNS Packet Structure *
  ************************

  +--------------------+
  |       Header       |
  +--------------------+
  |      Question      |
  +--------------------+
  |       Answer       |
  +--------------------+
  |     Authority      |
  +--------------------+
  |     Additional     |
  +--------------------+

  Header      Message Header.  See following section for details.

  Question    The DNS question being asked (aka Question Section).

  Answer      The Resource Record(s) which answer the question (aka Answer Section)

  Authority   The Resource Record(s) which point to the domain authority (aka Authority Section).

  Additional  The Resource Records(s) which may hod additional information (aka Additional Section).


  ************************
  * DNS Header Structure *
  ************************

    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                      ID                       |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |QR|   OPCODE  |AA|TC|RD|RA| Z|AD|CD|   RCODE   |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                    QDCOUNT                    |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                    ANCOUNT                    |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                    NSCOUNT                    |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                    ARCOUNT                    |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

  ID        A 16 bit identifier assigned by the requesition (the questioner) and reflected
            back unchanged by the responder (answerer).  Identifies the transaction.

  QR        Query - Response bit.  Set to 0 by the questioner (query) an dto 1 in the response
            (answer).

  OPCODE    Identifies the request/operation type.  Currently assigned values are:
              0 - QUERY.  Standard query.
              1 - IQUERY. Inverse query.  Optional support by DNS.
              2 - STATUS.  DNS status request.

  AA        Authoritative Answer.  Valid in responses only.  Because of aliases multiple owners
            may exist so the AA bit corresponds to the name which matches the query name, OR
            the first owner name in the answer section.

  TC        TrunCation - Specifies taht this message was truncated due to length grater than that
            permitted on the transmission channel.  Set on all truncated messages except the last
            one.

  RD        Recursion Desired - This bit may be sest in a query and is copied into the response
            if recursion supported.  If rejected the resopnse (answer) does not have this bit set.
            Recursive query support is optional.

  RA        Recursion Available - This bit is valid in a response (answer) and denotes whether
            recursive support is available (1) or not (0) in the name server.

  Z         Rezerved for future use.  Must be 0.

  RCODE     Response Code - This 4 bit field is set as part of responses.  The values are:
              0 - No error condition
              1 - Format error. The name server was unable to interpret the query.
              2 - Server failure. The name server was unable to process this query due to a
                  problem with the name server.
              3 - Name Error.  Meaningful only for responses from an authoritative name server.
                  This code signifies that the domain name referenced in the query does not exist.
              4 - Not Implemented.  The name server does not support the requested kind of query.
              5 - Refused.  The name server refuses to perform the specified operation for policy
                  reasons.

  QDCOUNT   Unsigned 16 bit integer specifying the nubmer of resource records in the Question Section.

  ANCOUNT   Unsigned 16 bit integer specifying the number of resource records in the Answer Section.
            May be 0 in which case no answer record is present in the message.

  NSCOUNT   Unsigned 16 bit ingerger specifying the number of name server resource records in the
            Authority Section.  May be 0 in which case no authority record(s) is(are) present in the
            message.

  ARCOUNT   Unsigned 16 bit integer specifying the number of name server resource records in the
            Additional Section.  May be 0 in which case no additional record(s) is(are) present in
            the message.



  ************************
  *     DNS Questions    *
  ************************

  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                                               |
  /                     QNAME                     /
  /                                               /
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                     QTYPE                     |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                     QCLASS                    |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

  QNAME   A domain name represented as a sequence of labels, where each label
          consists of a length octet followed by that number of octets.  The
          domain name terminates with the zero length octet for the null label
          of the root.

  QTYPE   A two octet code whcich specifies the type of the query.

  QCLASS  A two octet code taht specifies the class of the query.



  ************************
  *      DNS Answers     *
  ************************

    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                                               |
  /                                               /
  /                      NAME                     /
  |                                               |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                      TYPE                     |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                     CLASS                     |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                      TTL                      |
  |                                               |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                    RDLENGTH                   |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
  /                     RDATA                     /
  /                                               /
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+


  NAME        The domain name taht was queried.  May take one of TWO formats.
              The first format is the label format defined for QNAME above.
              The second format is a pointer (in the interests of data
              compression which to be fair to the original authors was far
              more important then than now).  A pointer is an unsigned 16-bit
              value with the following format (the top two bits of 11 indicate
              the pointer format):

                  0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
                +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
                | 1 | 1 |    Offset in bytes from the start of the message.     |
                +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

              Must point to a label format record to derive name length.

  TYPE        Unsigned 16 bit value.  The resource record types - determines the
              content of the RDATA field.  These values are assigned by IANA and
              a complete list of values may be obtained from them.  The following
              are the most commonly used values:
                x'0001  (1) = An A record for the domain name
                x'0002  (2) = A NS record for the domain name
                x'0005  (5) = A CNAME record for the domain name
                x'0006  (6) = A SOA record for the domain name
                x'000B (11) = A WKS record for the domain name
                x'000C (12) = A PTR record for the domain name
                x'000F (15) = A MX record for the domain name
                x'0021 (33) = A SRV record for the domain name
                x'0026 (38) = An A6 record for the domain name

  CLASS       Unsigned 16 bit value.  The CLASS of resource records be requested,
              for example, Internet, CHAOS etc.  These values are assigned by IANA
              and a complete list of values may be obtained from them.  The following
              are the most commonly used values:
                x'0001 (1) - IN or Internet

  TTL         Unsigned 32 bit value.  The time in seconds that the record may be
              cached.  A value of 0 indicates the record should not be cached.

  RDLENGTH    Unsigned 16 bit value that defines the length in bytes of the RDATA
              record.

  RDATA       Each (or rather most) resoruce record types have a specific RDATA
              format which reflect their resource record format as defined below:
                -------------------------------------------------------------------------------
                SOA
                  Primary NS       - Variable length.  The name of the Primary Master for
                                     the domain.  May be a label, pointer or any combination.
                  Admin MB         - Variable length.  The administrator's mailbox.  May be a
                                     label, pointer or any combination.
                  Serial Number    - Unsigned 32-bit integer.
                  Refresh Interval - Unsigned 32-bit integer.
                  Retry Interval   - Unsigned 32-bit integer.
                  Expiration Limit - Unsigned 32-bit integer.
                  Minimum TTL      - Unsigned 32-bit integer.
                -------------------------------------------------------------------------------
                MX
                  Preference       - A 16-bit integer.
                  Mail Exchanger   - The name host name that provides the service.  May be a
                                     label, pointer or any combination.
                -------------------------------------------------------------------------------
                A
                  IP Address       - Unsigned 32-bit value representing the IP address.
                -------------------------------------------------------------------------------
                PTR, NS
                  Name             - The host name that represents the supplied IP address (in 
                                     the case of a PTR) or the NS name for the supplied domain
                                     (in the case of NS).  May be a label, pointer or any 
                                     combintaiton.



  References:
  http://www.ccs.neu.edu/home/amislove/teaching/cs4700/fall09/handouts/project1-primer.pdf
  https://www.ietf.org/rfc/rfc1035.txt
  http://www.zytrax.com/books/dns/ch15/
 */

#ifndef __DNSClientImpl_h__
#define __DNSClientImpl_h__

typedef struct _DNSCLIENT_PRIVATE_DATA DNSCLIENT_PRIVATE_DATA;

#include <Uefi.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/Dhcp4.h>
#include <Protocol/Udp4.h>
#include <Protocol/Ip4.h>
#include <Protocol/Ip4Config.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/ServiceBinding.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/NetLib.h>

#define DNSCLIENT_PRIVATE_DATA_SIGNATURE SIGNATURE_64 ('C','A','B','D','N','S','C','l')

/**
  Safely destroys a pointer if the pointer is not null.

  @param  x  Pointer to free.
 */
#define SafeRelease(x)   if(x != NULL){ FreePool(x); x = NULL;}

/**
  Set status to y and goto x.

  @param  x  Label to go to.
  @param  y  EFI_STATUS to set Status to.
 */
#define GotoStatus(x,y) {Status = y; goto x;}

struct _DNSCLIENT_PRIVATE_DATA {
  UINT64                         Signature;
  EFI_HANDLE                     Image;

  EFI_HANDLE                     Udp4ServiceHandle;
  EFI_HANDLE                     Udp4Child;

  EFI_SERVICE_BINDING_PROTOCOL   *Udp4Sb;
  EFI_UDP4_PROTOCOL              *Udp4;
  
  EFI_UDP4_CONFIG_DATA           Udp4CfgData;

  UINT16                         IdIterator;
};

typedef UINT8 DNS_PACKET_DATA;

typedef struct _SOA_RECORD {
  CHAR8                          *PrimaryNS;
  CHAR8                          *AdminMB;
  UINT32                         SerialNumber;
  UINT32                         RefreshInterval;
  UINT32                         RetryInterval;
  UINT32                         ExpirationLimit;
  UINT32                         MinimumTTL;
} SOA_RECORD;

typedef struct _MX_RECORD {
  UINT16                         Preference;
  CHAR8                          *MailExchanger;
} MX_RECORD;

typedef struct _A_RECORD {
  EFI_IPv4_ADDRESS               IpAddress;
} A_RECORD;

typedef struct _PTR_RECORD {
  CHAR8                          *Name;
} PTR_RECORD;

typedef struct _NS_RECORD {
  CHAR8                          *Name;
} NS_RECORD;

typedef struct _DNS_HEADER {
  UINT16                         Id;         // 16 bit identifer assigned by the client.

  UINT16                         Rd:1;       //
  UINT16                         Tc:1;
  UINT16                         Aa:1;
  UINT16                         Opcode:4;
  UINT16                         Qr:1;

  UINT16                         RCode:4;
  UINT16                         Cd:1;
  UINT16                         Ad:1;
  UINT16                         Z:1;
  UINT16                         Ra:1;

  UINT16                         QdCount;
  UINT16                         AnCount;
  UINT16                         NsCount;
  UINT16                         ArCount;
} DNS_HEADER;

typedef struct _DNS_PACKET {
  DNS_HEADER                     Header;

  UINT16                         DataLength;

  DNS_PACKET_DATA                *Data;
} DNS_PACKET;

typedef struct _DNS_QUESTION {
  CHAR8                          *QName;
  UINT16                         QType;
  UINT16                         QClass;
} DNS_QUESTION;

typedef struct _DNS_ANSWER {
  CHAR8                          *Name;
  UINT16                         Type;
  UINT16                         Class;
  UINT32                         TTL;
  UINT32                         RdLength;
  VOID*                          RData;
} DNS_ANSWER;

/**
  Creates and initalizes the DNSClient's private data.

  @param[in] Instance  The Private data to be used.

  @retval EFI_SUCCESS  The Private variable has been initalized successfully.
  @retval other        An error occured. Private is unitalized.
  */
EFI_STATUS EFIAPI CreateDNSClient(DNSCLIENT_PRIVATE_DATA *Instance);

/**
  Cleans up and destroys the DNSClient's private data.

  @param[in] Instance  The Private data to be used.

  @retval EFI_SUCCESS     The private variable has been destroyed successfully.
  @retval other           An error occured.  The private variable may not have been properly destroyed.
  */
EFI_STATUS EFIAPI DestroyDNSClient(DNSCLIENT_PRIVATE_DATA *Instance);

/**
  Get's an ip address by a host name.

  @param[in]      Instance   The Private data to be used.
  @param[in]      Hostname   A null terminated string of the hostname to look up.
  @param[in/out]  IpAddress  The address of the hostname.

  @retval EFI_SUCCESS        The request completed successfully.
  @retval other              An error occured.
  */
EFI_STATUS EFIAPI GetHostByName(DNSCLIENT_PRIVATE_DATA *Instance, CHAR8 *Hostname, EFI_IPv4_ADDRESS *IpAddress);

/**
  Creates a DNS_PACKET based off of the parameters provided.  Must call ReleaseDNSPacket to free up used memory.

  @param[in] Questions     Array of Questions in the dns request.
  @param[in] NumQuestions  Number of Questions in the array.

  @retval EFI_SUCCESS      Packet data has been successfully created.
  @retval other            An error occured.
  */
DNS_PACKET* EFIAPI CreateDNSPacket(DNS_QUESTION Questions[], UINTN NumQuestions);

/**
  Release a DNS_PACKET.

  @param[in] Packet        The Packet to free data on.

  @retval EFI_SUCCESS      Data has been freed.
  @retval other            An error occured.
  */
EFI_STATUS EFIAPI ReleaseDNSPacket(DNS_PACKET *Packet);

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
EFI_STATUS EFIAPI SendDNSPacket(DNSCLIENT_PRIVATE_DATA *Instance, DNS_PACKET *Packet, CHAR16* Dst);

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
EFI_STATUS EFIAPI ReceiveDNSPacket(DNSCLIENT_PRIVATE_DATA *Instance, DNS_PACKET **Packet);

/**
  Sets a boolean to true.
  This function should not be called directly, but is intended to be used
  with the UEFI event system.

  @param[in] Event        The event that was triggered.
  @param[in] Context      The boolean to set.
 */
VOID EFIAPI DNSImplGenericCallback(IN EFI_EVENT Event, IN VOID *Context);

/**
  Converts a hostname to DNS label format.
  Must call FreePool when done with the string.

  @param[in] Hostname     The hostname string to convert.
  @param[in] Length       The length of the Hostname string.

  @retval NULL            Hostname is NULL or out of memory.
  @retval CHAR8*          Pointer to newly created label format string.
  */
CHAR8* EFIAPI HostnameToLabelFormat(CHAR8* Hostname, UINTN Length);

/**
  Converts a DNS label format to a Hostname string.  
  Must call FreePool when done with the string.

  @param[in] LabelFormat  The label formatted string to convert.

  @retval NULL            LabelFormat is NULL or out of memory.
  @retval CHAR8*          Pointer to newly created string.
  */
CHAR8* EFIAPI LabelFormatToHostname(CHAR8* LabelFormat);
#endif