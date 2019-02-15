#import <CoreFoundation/CoreFoundation.h>
#import <inttypes.h>
#import <sys/socket.h>
#import <netinet/in.h>

#ifndef __EMBEDDEDOS_SUPPPORT__
#define __EMBEDDEDOS_SUPPPORT__

#ifndef SO_INTCOPROC_ALLOW
#define SO_INTCOPROC_ALLOW 0x1118
#endif

#ifdef __cplusplus
extern "C" {
#endif

// @(#)PROGRAM:EmbeddedOSSupportHost  PROJECT:AppleEmbeddedOSSupport
extern const char* EmbeddedOSSupportHostVersionString;
// 1.0 by time this header was made
extern const double EmbeddedOSSupportHostVersionNumber;

typedef enum {
    kEOSErrorNone,
    kEOSErrorBadArgument,
    kEOSErrorCommunicationFailure,
    kEOSErrorDeviceNotFound,
    kEOSErrorDeviceNotSupported,
    kEOSErrorDriverFailure,
    kEOSErrorResourceAllocationFailure,
    kEOSErrorUnknown,
} eos_error_t;
const char *eos_strerror(eos_error_t err);

typedef enum {
    kEOSServiceInvalid,
    kEOSServiceStockholmUART,
    kEOSServiceStockholmRPC,
    kEOSServiceRVF,
    kEOSServiceABSE,
    kEOSServiceEostrace,
    kEOSServiceSysdiagnose,
    kEOSServiceLogging,
    kEOSServiceLASecureIO,
    kEOSServiceEOSSupport,
    kEOSServiceXARTStorage,
    kEOSServiceTimeSync,
    kEOSServiceDeviceQuery,
    kEOSServiceFDRd,
    kEOSServiceRepairTask,
    kEOSServiceReverseProxy,
    kEOSServiceXARTRecovery,
    kEOSServiceEcho,
    kEOSServiceFactoryProcess,
    kEOSServiceBiometricKit,
    kEOSServiceDFRBrightness,
    kEOSServiceAID,
    kEOSServiceMax
} eos_service_t;
const char *eos_strservice(eos_service_t a1);

bool eos_device_is_supported(void);

typedef enum {
    kBridge1_1 = 0x1000,
    kBridge2_maybe_that_iMac_thingy = 0x2000,
} eos_device_type_t;

/*
 * Get type of device avaliable on current machine
 * Returns:
 *  kEOSErrorNone
 *  kEOSErrorDeviceNotFound
 */
eos_error_t eos_device_get_type(eos_device_type_t *type);

/*
 * Put device to Recovery mode
 * Sets "DeviceConnected" to 0 on success
 * Returns:
 *  kEOSErrorNone
 *  kEOSErrorDeviceNotSupported
 *  kEOSErrorDriverFailure
 */
eos_error_t eos_device_force_reset(void);

/*
 * Put device to DFU mode
 * Sets "DeviceConnected" to 0 on success
 * Returns: see eos_device_force_reset
 */
eos_error_t eos_device_force_dfu(void);

/*
 * Set "DeviceIsHealed" from driver
 * Returns:
 *  kEOSErrorNone
 *  kEOSErrorDeviceNotSupported
 *  kEOSErrorDriverFailure
 */
eos_error_t eos_device_set_healed(bool ishealed);

/*
 * Get "DeviceIsHealed" from driver
 * Returns:
 *  kEOSErrorNone
 *  kEOSErrorBadArgument
 *  kEOSErrorDeviceNotSupported
 *  kEOSErrorDriverFailure
 */
eos_error_t eos_device_get_healed(bool *ishealed);

/*
 * eos_message_t serialization format:
 * ---------
 * uint32_t raw_header_len
 * raw_header:
 *   // raw_header has size of raw_header_len, but cant be larger than 0x200
 *   uint8_t  pad (yes, one byte, and everything is misaligned because of it aaa)
 *   uint32_t crc -- see _eos_message_calculcate_crc
 *   uint32_t payload_len
 * payload:
 *   // payload_len bytes, but max is (MAX_PAYLOAD_LEN - 1)
 *   // usually it's CFDictionary serialized as bplist v1.0
 *   raw bytes...
 * ---------
 */

struct eos_message_serialized {
    uint32_t raw_header_len;
    union {
        struct {
            uint8_t reserved;
            uint32_t crc;

// 0x200000 on first gen iBridge
#define MAX_PAYLOAD_LEN  0x600000
            uint32_t payload_len;
        } __attribute__((aligned(1),packed)) eos_msg;

        char raw[512];
    } header;

    uint8_t payload[/* payload_len */];
} __attribute__((aligned(1),packed));

typedef enum {
    kEOSMsgErrNone = 0,
    kEOSMsgErrUnknown = 100,
    kEOSMsgErrInvalidCommand = 200,
} eos_message_error_t;

typedef enum {
    /*
     * eos_message_t is a CFDictionary [CFString => CFType]
     * There are Query's and Reply's
     * Query:
     *   Command: CFNumber, see eos_cmd_t
     *   Version: Currently 1000 (0x3E8)
     *   More, depending on Command
     * Reply:
     *   Command: Same as Query command this is a reply to
     *   Success: CFBoolean
     *   Error: see eos_message_error_t (always exists, even on Success=kCFBooleanTrue)
     *   More, depending on Command
     */

    /*
     * host => kEOSServiceDeviceQuery
     * Query:
     *   GestaltKeys: CFArray of keys to fetch
     * Reply:
     *   GestaltKeysWithAnswers: CFDictionary [CFString key => CFType value]
     */
    kEOSMessageTag_EOSFetchGestaltKeys = 3,

    /*
     * host => kEOSServiceDeviceQuery
     * Query: Empty
     * Reply:
     *   GestaltKeys: CFArray of supported keys
     */
    kEOSMessageTag_EOSFetchSupportedGestaltKeysList = 4,

    /*
     * host => kEOSServiceDeviceQuery
     * Query: Empty
     * Reply:
     *   BootArgs: CFString with eOS's kern.bootargs
     */
    kEOSMessageTag_EOSFetchBootArgs = 5,

    /*
     * host => kEOSServiceDeviceQuery
     * Returns success if version is supported
     * Query: Empty
     * Reply: Empty
     */
    kEOSMessageTag_EOSGetServiceVersion = 6,

    /*
     * kEOSServiceEOSSupport => host
     * Query: Empty
     * Reply: Empty
     */
    kEOSMessageTag_EOSPing = 100,

    /*
     * kEOSServiceEOSSupport => host
     * Query:
     *   CrashContent: CFString with contents of crash log
     *   CrashProcName: Name of crashed process
     * Reply: Empty
     */
    kEOSMessageTag_EOSSubmitCrash = 110,

    // host => XXX
    kEOSMessageTag_EnableCnxnWatchdog = 200,

    // host => XXX
    kEOSMessageTag_WatchdogHeartbeet = 201,

    // host => XXX
    kEOSMessageTag_WatchdogDisableAck = 202,
    // kEOSServiceEOSSupport => host
    kEOSMessageTag_DisableCnxnWatchdog = 203,
} eos_cmd_t;


typedef enum {
    kEOSDriverEvent_DeviceAdded,
    kEOSDriverEvent_DeviceRemoved,
    kEOSDriverEvent_DevicePanicked, // DeviceTriggeredPanic
    kEOSDriverEvent_DeviceRestartedByHost,
    kEOSDriverEvent_DeviceWatchDog, // DeviceTriggeredHardwareReset
    kEOSDriverEvent_Max,
} eos_driver_event_t;

typedef CFDictionaryRef eos_message_t;

eos_message_t eos_message_create(eos_cmd_t command);
/*
 * Create message which a reply to request (eos_message_create(request=>Command))
 */
eos_message_t eos_message_create_reply(eos_message_t request);

// safe to call on NULL
void eos_message_destroy(eos_message_t msg);

void eos_message_set_value(eos_message_t msg, CFTypeRef key, CFTypeRef value);
CFTypeRef eos_message_get_value(eos_message_t msg, CFTypeRef key);

void eos_message_set_uint32(eos_message_t msg, CFTypeRef key, uint32_t value);
uint32_t eos_message_get_uint32(eos_message_t msg, CFTypeRef key);

// Calculate crc for message
uint32_t _eos_message_calculcate_crc(const uint8_t* buffer, uint32_t len);

// socket
typedef int eos_connection_t;

eos_error_t eos_message_send(eos_connection_t conn, eos_message_t msg);
eos_message_t eos_message_receive(eos_connection_t conn);

// Send message and return reply
eos_message_t eos_message_send_with_reply_sync(eos_connection_t conn, eos_message_t msg);

typedef enum {
    kSTREAM = SOCK_STREAM, // 1
    kDGRAM = SOCK_DGRAM, // 2
} eos_conntype_t;


// APPL calls conn "sockfd", and "addr" is "sa"
eos_error_t eos_device_connect(eos_connection_t *conn, eos_conntype_t type, struct sockaddr *addr, eos_service_t service, int *connect_errno);
eos_error_t eos_device_init(eos_connection_t *conn, eos_conntype_t type, struct sockaddr_in6 *addr, eos_service_t service);

// domain should be PF_INET6 and protocol should be 0
eos_error_t eos_device_init_socket_in6(eos_connection_t *conn, int domain, eos_conntype_t type, int protocol);

// initialize addr fo service
eos_error_t eos_device_init_sockaddr_in6(struct sockaddr_in6 *add, eos_service_t service);

// get eOS device's addr -- "fe80::aede:48ff:fe33:4455"
eos_error_t eos_device_get_addr_in6(struct in6_addr *addr);

// Internal versions of functions above
// then don't check for eos_device_is_supported
eos_error_t _eos_endpoint_get_addr_in6(struct in6_addr *addr);
eos_error_t _eos_endpoint_init_sockaddr_in6(struct sockaddr_in6 *add, eos_service_t service);
eos_error_t _eos_endpoint_init_socket_in6(eos_connection_t *conn, int domain, eos_conntype_t type, int protocol);
eos_error_t _eos_endpoint_init(eos_connection_t *conn, eos_conntype_t type, struct sockaddr_in6 *addr, eos_service_t service);
eos_error_t _eos_endpoint_connect(eos_connection_t *conn, eos_conntype_t type, struct sockaddr *addr, eos_service_t service, int *connect_errno);


// eOS network properties (use _get_ functions to ensure initialized)

// Initialize
extern bool s_eos_if_fetched;
eos_error_t _eos_network_fetch_properties(void);

// bsd interface name of eOS network 
extern const volatile char s_eos_ifname[16];
eos_error_t _eos_endpoint_get_ifname(char ifname[16]);

// if_nametoindex(s_eos_ifname)
extern unsigned int s_eos_ifindex;
eos_error_t _eos_endpoint_get_ifindex(unsigned int *ifindex);

// get eOS port for service (in host endianness)
extern int s_eos_service_ports[kEOSServiceMax];
eos_error_t _eos_endpoint_get_port(eos_service_t service, int* port);

// known/common gestalt keys
extern const CFStringRef kEOSGestaltKey_ApNonce;
extern const CFStringRef kEOSGestaltKey_BoardId;
extern const CFStringRef kEOSGestaltKey_BridgeBuild;
extern const CFStringRef kEOSGestaltKey_BuildVersion;
extern const CFStringRef kEOSGestaltKey_CertificateSecurityMode;
extern const CFStringRef kEOSGestaltKey_ChipID;
extern const CFStringRef kEOSGestaltKey_EffectiveProductionStatusAp;
extern const CFStringRef kEOSGestaltKey_EffectiveSecurityModeAp;
extern const CFStringRef kEOSGestaltKey_HardwarePlatform;
extern const CFStringRef kEOSGestaltKey_HasSEP;
extern const CFStringRef kEOSGestaltKey_HWModelStr;
extern const CFStringRef kEOSGestaltKey_Image4CryptoHashMethod;
extern const CFStringRef kEOSGestaltKey_Image4Supported;
extern const CFStringRef kEOSGestaltKey_IsAppleInternal;
extern const CFStringRef kEOSGestaltKey_SerialNumber;
extern const CFStringRef kEOSGestaltKey_SigningFuse;
extern const CFStringRef kEOSGestaltKey_SEPNonce;
extern const CFStringRef kEOSGestaltKey_UniqueChipID;

/*
 * Fetch keys specified by "keys" from eOS MobileGestalt
 * dict is retained
 * See eos_device_fetch_supported_gestalt_keys_list
 * Returns:
 *  kEOSErrorNone
 *  kEOSErrorBadArgument
 *  kEOSErrorCommunicationFailure
 *  kEOSErrorDeviceNotSupported
 */
eos_error_t eos_device_fetch_gestalt_keys(CFArrayRef keys, CFDictionaryRef *dict);

/*
 * Get list of eOS MobileGestalt allowed keys
 * arr is retained
 * See eos_device_fetch_gestalt_keys
 * Returns:
 *  kEOSErrorNone
 *  kEOSErrorBadArgument
 *  kEOSErrorCommunicationFailure
 *  kEOSErrorDeviceNotSupported
 */
eos_error_t eos_device_fetch_supported_gestalt_keys_list(CFArrayRef *arr);


/*
 * Get eOS boot args
 * str is retained
 * Returns:
 *  kEOSErrorNone
 *  kEOSErrorBadArgument
 *  kEOSErrorCommunicationFailure
 *  kEOSErrorDeviceNotSupported
 */
eos_error_t eos_device_fetch_boot_args(CFStringRef *str);


typedef enum {
    kEOSEventDeviceConnected,
    kEOSEventDeviceUnresponsive,
    kEOSEventMax
} eos_event_id_t;

// actually array size is kEOSEventMax - 1 but whatever
extern const char kEOSNotificationLabels[kEOSEventMax];

eos_error_t _eos_device_get_notify_state(const char* notification_label, bool* value);

/*
 * Get "DeviceConnected" from systemwide notify_state
 * Returns:
 *  kEOSErrorNone
 *  kEOSErrorBadArgument (?)
 *  kEOSErrorDeviceNotSupported
 *  kEOSErrorUnknown
 */
eos_error_t eos_device_is_connected(bool *isConnected);

/*
 * Get "DeviceUnresponsive" from systemwide notify_state
 * Returns: See eos_device_is_connected
 */
eos_error_t eos_device_is_unresponsive(bool *isUnresponsive);

// Registers darwin notify handlers for all kEOSNotificationLabels
// There can be only one handler set at the same time for one process
typedef void (^eos_event_handler_t)(eos_event_id_t event_id);
eos_error_t eos_device_register_event_handler(dispatch_queue_t queue, eos_event_handler_t handler);
eos_error_t eos_device_unregister_event_handler(void);

#ifdef __cplusplus
}
#endif

#endif // __EMBEDDEDOS_SUPPPORT__
