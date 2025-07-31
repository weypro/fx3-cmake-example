#include "usb_descriptors.h"
#include "cyu3usbconst.h"

#include <stddef.h>

// Generic macros for byte extraction from 16-bit values
#define GET_LOW_BYTE(val)           ((val) & 0xFF)
#define GET_HIGH_BYTE(val)          (((val) >> 8) & 0xFF)

// USB packet size constants
#define USB_EP0_PACKET_SIZE_SS      0x09    // EP0 max packet size for SS (2^9 = 512)
#define USB_EP0_PACKET_SIZE_HS      0x40    // EP0 max packet size for HS (64 bytes)
#define USB_BULK_PACKET_SIZE_SS     1024    // Bulk EP packet size for SuperSpeed
#define USB_BULK_PACKET_SIZE_HS     512     // Bulk EP packet size for HighSpeed
#define USB_BULK_PACKET_SIZE_FS     64      // Bulk EP packet size for FullSpeed

// USB descriptor size constants
#define USB_DEVICE_DESCRIPTOR_SIZE          0x12    // 18 bytes
#define USB_CONFIG_DESCRIPTOR_SIZE          0x09    // 9 bytes
#define USB_INTERFACE_DESCRIPTOR_SIZE       0x09    // 9 bytes
#define USB_ENDPOINT_DESCRIPTOR_SIZE        0x07    // 7 bytes
#define USB_SS_EP_COMPANION_DESCRIPTOR_SIZE 0x06    // 6 bytes
#define USB_DEVICE_QUALIFIER_DESCRIPTOR_SIZE 0x0A   // 10 bytes
#define USB_BOS_DESCRIPTOR_SIZE             0x05    // 5 bytes
#define USB_USB2_EXT_CAPABILITY_SIZE        0x07    // 7 bytes
#define USB_SS_CAPABILITY_SIZE              0x0A    // 10 bytes

// Macros for splitting 16-bit values into bytes (Little Endian)
#define USB_VENDOR_ID_LOW_BYTE      GET_LOW_BYTE(USB_VENDOR_ID)
#define USB_VENDOR_ID_HIGH_BYTE     GET_HIGH_BYTE(USB_VENDOR_ID)
#define USB_PRODUCT_ID_LOW_BYTE     GET_LOW_BYTE(USB_PRODUCT_ID)
#define USB_PRODUCT_ID_HIGH_BYTE    GET_HIGH_BYTE(USB_PRODUCT_ID)
#define USB_DEVICE_VERSION_LOW_BYTE GET_LOW_BYTE(USB_DEVICE_VERSION)
#define USB_DEVICE_VERSION_HIGH_BYTE GET_HIGH_BYTE(USB_DEVICE_VERSION)

// Super Speed device descriptor
const uint8_t usb_device_descriptor_ss[] __attribute__ ((aligned (32))) = {
    USB_DEVICE_DESCRIPTOR_SIZE,     // Descriptor size
    CY_U3P_USB_DEVICE_DESCR,        // Device descriptor type
    0x20,0x03,                      // USB 3.2 Gen 1 (USB 5Gbps)
    0x00,                           // Device class
    0x00,                           // Device sub-class
    0x00,                           // Device protocol
    USB_EP0_PACKET_SIZE_SS,         // Maxpacket size for EP0 : 2^9
    USB_VENDOR_ID_LOW_BYTE, USB_VENDOR_ID_HIGH_BYTE,   // Vendor ID
    USB_PRODUCT_ID_LOW_BYTE, USB_PRODUCT_ID_HIGH_BYTE, // Product ID
    USB_DEVICE_VERSION_LOW_BYTE, USB_DEVICE_VERSION_HIGH_BYTE, // Device release number
    0x01,                           // Manufacture string index
    0x02,                           // Product string index
    0x00,                           // Serial number string index
    0x01                            // Number of configurations
};

// High Speed device descriptor
const uint8_t usb_device_descriptor_hs[] __attribute__ ((aligned (32))) = {
    USB_DEVICE_DESCRIPTOR_SIZE,     // Descriptor size
    CY_U3P_USB_DEVICE_DESCR,        // Device descriptor type
    0x10,0x02,                      // USB 2.10
    0x00,                           // Device class
    0x00,                           // Device sub-class
    0x00,                           // Device protocol
    USB_EP0_PACKET_SIZE_HS,         // Maxpacket size for EP0 : 64 bytes
    USB_VENDOR_ID_LOW_BYTE, USB_VENDOR_ID_HIGH_BYTE,   // Vendor ID
    USB_PRODUCT_ID_LOW_BYTE, USB_PRODUCT_ID_HIGH_BYTE, // Product ID
    USB_DEVICE_VERSION_LOW_BYTE, USB_DEVICE_VERSION_HIGH_BYTE, // Device release number
    0x01,                           // Manufacture string index
    0x02,                           // Product string index
    0x00,                           // Serial number string index
    0x01                            // Number of configurations
};

// Binary Object Store descriptor
const uint8_t usb_bos_descriptor[] __attribute__ ((aligned (32))) = {
    USB_BOS_DESCRIPTOR_SIZE,        // Descriptor size
    CY_U3P_BOS_DESCR,               // Device descriptor type
    0x16,0x00,                      // Length of this descriptor and all sub descriptors
    0x02,                           // Number of device capability descriptors

    // USB 2.0 extension capability
    USB_USB2_EXT_CAPABILITY_SIZE,   // Descriptor size
    CY_U3P_DEVICE_CAPB_DESCR,       // Device capability type descriptor
    CY_U3P_USB2_EXTN_CAPB_TYPE,     // USB 2.0 extension capability type
    0x1E,0x64,0x00,0x00,            // LPM support, BESL supported, Baseline=400us, Deep=1000us

    // SuperSpeed device capability
    USB_SS_CAPABILITY_SIZE,         // Descriptor size
    CY_U3P_DEVICE_CAPB_DESCR,       // Device capability type descriptor
    CY_U3P_SS_USB_CAPB_TYPE,        // SuperSpeed device capability type
    0x00,                           // Supported device level features
    0x0E,0x00,                      // Speeds supported: SS, HS and FS
    0x03,                           // Functionality support
    0x0A,                           // U1 Device Exit latency
    0xFF,0x07                       // U2 Device Exit latency
};

// Device qualifier descriptor
const uint8_t usb_device_qualifier_descriptor[] __attribute__ ((aligned (32))) = {
    USB_DEVICE_QUALIFIER_DESCRIPTOR_SIZE, // Descriptor size
    CY_U3P_USB_DEVQUAL_DESCR,       // Device qualifier descriptor type
    0x00,0x02,                      // USB 2.0
    0x00,                           // Device class
    0x00,                           // Device sub-class
    0x00,                           // Device protocol
    USB_EP0_PACKET_SIZE_HS,         // Maxpacket size for EP0 : 64 bytes
    0x01,                           // Number of configurations
    0x00                            // Reserved
};

// Super Speed configuration descriptor
const uint8_t usb_config_descriptor_ss[] __attribute__ ((aligned (32))) = {
    // Configuration descriptor
    USB_CONFIG_DESCRIPTOR_SIZE,     // Descriptor size
    CY_U3P_USB_CONFIG_DESCR,        // Configuration descriptor type
    0x2C,0x00,                      // Length of this descriptor and all sub descriptors
    0x01,                           // Number of interfaces
    0x01,                           // Configuration number
    0x00,                           // Configuration string index
    0x80,                           // Config characteristics - Bus powered
    0x32,                           // Max power consumption : 400mA (in 8mA units)

    // Interface descriptor
    USB_INTERFACE_DESCRIPTOR_SIZE,  // Descriptor size
    CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type
    0x00,                           // Interface number
    0x00,                           // Alternate setting number
    0x02,                           // Number of endpoints
    0xFF,                           // Interface class (vendor specific)
    0x00,                           // Interface sub class
    0x00,                           // Interface protocol code
    0x00,                           // Interface descriptor string index

    // Endpoint descriptor for producer EP (OUT)
    USB_ENDPOINT_DESCRIPTOR_SIZE,   // Descriptor size
    CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type
    USB_EP_PRODUCER,                // Endpoint address and description
    CY_U3P_USB_EP_BULK,             // Bulk endpoint type
    GET_LOW_BYTE(USB_BULK_PACKET_SIZE_SS), GET_HIGH_BYTE(USB_BULK_PACKET_SIZE_SS), // Max packet size = 1024 bytes
    0x00,                           // Servicing interval : 0 for bulk

    // Super Speed endpoint companion descriptor for producer EP
    USB_SS_EP_COMPANION_DESCRIPTOR_SIZE, // Descriptor size
    CY_U3P_SS_EP_COMPN_DESCR,       // SS endpoint companion descriptor type
    (DMA_BURST_LENGTH - 1),         // Max burst packets (0-15)
    0x00,                           // Max streams for bulk EP = 0
    0x00,0x00,                      // Service interval : 0 for bulk

    // Endpoint descriptor for consumer EP (IN)
    USB_ENDPOINT_DESCRIPTOR_SIZE,   // Descriptor size
    CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type
    USB_EP_CONSUMER,                // Endpoint address and description
    CY_U3P_USB_EP_BULK,             // Bulk endpoint type
    GET_LOW_BYTE(USB_BULK_PACKET_SIZE_SS), GET_HIGH_BYTE(USB_BULK_PACKET_SIZE_SS), // Max packet size = 1024 bytes
    0x00,                           // Servicing interval : 0 for bulk

    // Super Speed endpoint companion descriptor for consumer EP
    USB_SS_EP_COMPANION_DESCRIPTOR_SIZE, // Descriptor size
    CY_U3P_SS_EP_COMPN_DESCR,       // SS endpoint companion descriptor type
    (DMA_BURST_LENGTH - 1),         // Max burst packets (0-15)
    0x00,                           // Max streams for bulk EP = 0
    0x00,0x00                       // Service interval : 0 for bulk
};

// High Speed configuration descriptor
const uint8_t usb_config_descriptor_hs[] __attribute__ ((aligned (32))) = {
    // Configuration descriptor
    USB_CONFIG_DESCRIPTOR_SIZE,     // Descriptor size
    CY_U3P_USB_CONFIG_DESCR,        // Configuration descriptor type
    0x20,0x00,                      // Length of this descriptor and all sub descriptors
    0x01,                           // Number of interfaces
    0x01,                           // Configuration number
    0x00,                           // Configuration string index
    0x80,                           // Config characteristics - bus powered
    0x32,                           // Max power consumption : 100mA (in 2mA units)

    // Interface descriptor
    USB_INTERFACE_DESCRIPTOR_SIZE,  // Descriptor size
    CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type
    0x00,                           // Interface number
    0x00,                           // Alternate setting number
    0x02,                           // Number of endpoints
    0xFF,                           // Interface class (vendor specific)
    0x00,                           // Interface sub class
    0x00,                           // Interface protocol code
    0x00,                           // Interface descriptor string index

    // Endpoint descriptor for producer EP (OUT)
    USB_ENDPOINT_DESCRIPTOR_SIZE,   // Descriptor size
    CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type
    USB_EP_PRODUCER,                // Endpoint address and description
    CY_U3P_USB_EP_BULK,             // Bulk endpoint type
    GET_LOW_BYTE(USB_BULK_PACKET_SIZE_HS), GET_HIGH_BYTE(USB_BULK_PACKET_SIZE_HS), // Max packet size = 512 bytes
    0x00,                           // Servicing interval : 0 for bulk

    // Endpoint descriptor for consumer EP (IN)
    USB_ENDPOINT_DESCRIPTOR_SIZE,   // Descriptor size
    CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type
    USB_EP_CONSUMER,                // Endpoint address and description
    CY_U3P_USB_EP_BULK,             // Bulk endpoint type
    GET_LOW_BYTE(USB_BULK_PACKET_SIZE_HS), GET_HIGH_BYTE(USB_BULK_PACKET_SIZE_HS), // Max packet size = 512 bytes
    0x00                            // Servicing interval : 0 for bulk
};

// Full Speed configuration descriptor
const uint8_t usb_config_descriptor_fs[] __attribute__ ((aligned (32))) = {
    // Configuration descriptor
    USB_CONFIG_DESCRIPTOR_SIZE,     // Descriptor size
    CY_U3P_USB_CONFIG_DESCR,        // Configuration descriptor type
    0x20,0x00,                      // Length of this descriptor and all sub descriptors
    0x01,                           // Number of interfaces
    0x01,                           // Configuration number
    0x00,                           // Configuration string index
    0x80,                           // Config characteristics - bus powered
    0x32,                           // Max power consumption : 100mA (in 2mA units)

    // Interface descriptor
    USB_INTERFACE_DESCRIPTOR_SIZE,  // Descriptor size
    CY_U3P_USB_INTRFC_DESCR,        // Interface descriptor type
    0x00,                           // Interface number
    0x00,                           // Alternate setting number
    0x02,                           // Number of endpoints
    0xFF,                           // Interface class (vendor specific)
    0x00,                           // Interface sub class
    0x00,                           // Interface protocol code
    0x00,                           // Interface descriptor string index

    // Endpoint descriptor for producer EP (OUT)
    USB_ENDPOINT_DESCRIPTOR_SIZE,   // Descriptor size
    CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type
    USB_EP_PRODUCER,                // Endpoint address and description
    CY_U3P_USB_EP_BULK,             // Bulk endpoint type
    GET_LOW_BYTE(USB_BULK_PACKET_SIZE_FS), GET_HIGH_BYTE(USB_BULK_PACKET_SIZE_FS), // Max packet size = 64 bytes
    0x00,                           // Servicing interval : 0 for bulk

    // Endpoint descriptor for consumer EP (IN)
    USB_ENDPOINT_DESCRIPTOR_SIZE,   // Descriptor size
    CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type
    USB_EP_CONSUMER,                // Endpoint address and description
    CY_U3P_USB_EP_BULK,             // Bulk endpoint type
    GET_LOW_BYTE(USB_BULK_PACKET_SIZE_FS), GET_HIGH_BYTE(USB_BULK_PACKET_SIZE_FS), // Max packet size = 64 bytes
    0x00                            // Servicing interval : 0 for bulk
};

// String descriptor for language ID
const uint8_t usb_string_lang_descriptor[] __attribute__ ((aligned (32))) = {
    0x04,                           // Descriptor size
    CY_U3P_USB_STRING_DESCR,        // Device descriptor type
    0x09,0x04                       // Language ID: English (US)
};

// Manufacturer string descriptor
const uint8_t usb_string_manufacturer_descriptor[] __attribute__ ((aligned (32))) = {
    0x10,                           // Descriptor size
    CY_U3P_USB_STRING_DESCR,        // Device descriptor type
    'C',0x00, 'y',0x00, 'p',0x00, 'r',0x00,
    'e',0x00, 's',0x00, 's',0x00
};

// Product string descriptor
const uint8_t usb_string_product_descriptor[] __attribute__ ((aligned (32))) = {
    0x08,                           // Descriptor size
    CY_U3P_USB_STRING_DESCR,        // Device descriptor type
    'F',0x00, 'X',0x00, '3',0x00
};

// Microsoft OS descriptor
const uint8_t usb_ms_os_descriptor[] __attribute__ ((aligned (32))) = {
    0x0E,                           // Descriptor size
    CY_U3P_USB_STRING_DESCR,        // String descriptor type
    'O',0x00, 'S',0x00, ' ',0x00, 'D',0x00,
    'e',0x00, 's',0x00, 'c',0x00
};

// Cache alignment buffer
const uint8_t usb_descriptor_align_buffer[32] __attribute__ ((aligned (32)));

// Descriptor information table
static const usb_descriptor_info_t descriptor_table[USB_DESC_COUNT] = {
    [USB_DESC_DEVICE_SS] = {
        .data = usb_device_descriptor_ss,
        .length = sizeof(usb_device_descriptor_ss),
        .is_available = TRUE
    },
    [USB_DESC_DEVICE_HS] = {
        .data = usb_device_descriptor_hs,
        .length = sizeof(usb_device_descriptor_hs),
        .is_available = TRUE
    },
    [USB_DESC_DEVICE_QUALIFIER] = {
        .data = usb_device_qualifier_descriptor,
        .length = sizeof(usb_device_qualifier_descriptor),
        .is_available = TRUE
    },
    [USB_DESC_CONFIG_SS] = {
        .data = usb_config_descriptor_ss,
        .length = sizeof(usb_config_descriptor_ss),
        .is_available = TRUE
    },
    [USB_DESC_CONFIG_HS] = {
        .data = usb_config_descriptor_hs,
        .length = sizeof(usb_config_descriptor_hs),
        .is_available = TRUE
    },
    [USB_DESC_CONFIG_FS] = {
        .data = usb_config_descriptor_fs,
        .length = sizeof(usb_config_descriptor_fs),
        .is_available = TRUE
    },
    [USB_DESC_BOS] = {
        .data = usb_bos_descriptor,
        .length = sizeof(usb_bos_descriptor),
        .is_available = TRUE
    },
    [USB_DESC_STRING_LANG] = {
        .data = usb_string_lang_descriptor,
        .length = sizeof(usb_string_lang_descriptor),
        .is_available = TRUE
    },
    [USB_DESC_STRING_MANUFACTURER] = {
        .data = usb_string_manufacturer_descriptor,
        .length = sizeof(usb_string_manufacturer_descriptor),
        .is_available = TRUE
    },
    [USB_DESC_STRING_PRODUCT] = {
        .data = usb_string_product_descriptor,
        .length = sizeof(usb_string_product_descriptor),
        .is_available = TRUE
    },
    [USB_DESC_MS_OS] = {
        .data = usb_ms_os_descriptor,
        .length = sizeof(usb_ms_os_descriptor),
        .is_available = TRUE
    }
};

// Get descriptor information for specific type
const usb_descriptor_info_t* usb_descriptors_get_info(usb_descriptor_type_t type)
{
    if (type >= USB_DESC_COUNT) {
        return NULL;
    }

    return &descriptor_table[type];
}

// Get descriptor data pointer
const uint8_t* usb_descriptors_get_data(usb_descriptor_type_t type)
{
    const usb_descriptor_info_t* info = usb_descriptors_get_info(type);
    return info ? info->data : NULL;
}

// Get descriptor length
uint16_t usb_descriptors_get_length(usb_descriptor_type_t type)
{
    const usb_descriptor_info_t* info = usb_descriptors_get_info(type);
    return info ? info->length : 0;
}

// Register all descriptors with USB driver
fx3_result_t usb_descriptors_register_all()
{
    CyU3PReturnStatus_t status;

    // Register Super Speed device descriptor
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, 0,
                           (uint8_t*)usb_descriptors_get_data(USB_DESC_DEVICE_SS));
    FX3_CHECK_CYU3P(status);

    // Register High Speed device descriptor
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, 0,
                           (uint8_t*)usb_descriptors_get_data(USB_DESC_DEVICE_HS));
    FX3_CHECK_CYU3P(status);

    // Register BOS descriptor
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, 0,
                           (uint8_t*)usb_descriptors_get_data(USB_DESC_BOS));
    FX3_CHECK_CYU3P(status);

    // Register device qualifier descriptor
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, 0,
                           (uint8_t*)usb_descriptors_get_data(USB_DESC_DEVICE_QUALIFIER));
    FX3_CHECK_CYU3P(status);

    // Register configuration descriptors
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, 0,
                           (uint8_t*)usb_descriptors_get_data(USB_DESC_CONFIG_SS));
    FX3_CHECK_CYU3P(status);

    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, 0,
                           (uint8_t*)usb_descriptors_get_data(USB_DESC_CONFIG_HS));
    FX3_CHECK_CYU3P(status);

    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, 0,
                           (uint8_t*)usb_descriptors_get_data(USB_DESC_CONFIG_FS));
    FX3_CHECK_CYU3P(status);

    // Register string descriptors
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0,
                           (uint8_t*)usb_descriptors_get_data(USB_DESC_STRING_LANG));
    FX3_CHECK_CYU3P(status);

    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1,
                           (uint8_t*)usb_descriptors_get_data(USB_DESC_STRING_MANUFACTURER));
    FX3_CHECK_CYU3P(status);

    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2,
                           (uint8_t*)usb_descriptors_get_data(USB_DESC_STRING_PRODUCT));
    FX3_CHECK_CYU3P(status);

    return FX3_SUCCESS;
}

// Get endpoint configuration for current USB speed
usb_endpoint_config_t usb_descriptors_get_endpoint_config(usb_speed_t speed)
{
    usb_endpoint_config_t config = {0};

    switch (speed) {
        case USB_SPEED_SUPER:
            config.max_packet_size = USB_PACKET_SIZE_SUPER_SPEED;
            config.burst_length = DMA_BURST_LENGTH;
            config.interval = 0;
            break;

        case USB_SPEED_HIGH:
            config.max_packet_size = USB_PACKET_SIZE_HIGH_SPEED;
            config.burst_length = 1;
            config.interval = 0;
            break;

        case USB_SPEED_FULL:
            config.max_packet_size = USB_PACKET_SIZE_FULL_SPEED;
            config.burst_length = 1;
            config.interval = 0;
            break;

        default:
            config.max_packet_size = USB_PACKET_SIZE_FULL_SPEED;
            config.burst_length = 1;
            config.interval = 0;
            break;
    }

    return config;
}

// Validate descriptor integrity
fx3_result_t usb_descriptors_validate()
{
    // Check that all required descriptors are available
    for (int i = 0; i < USB_DESC_COUNT; i++) {
        if (!descriptor_table[i].is_available ||
            !descriptor_table[i].data ||
            descriptor_table[i].length == 0) {
            return FX3_ERROR_INIT_FAILED;
        }
    }

    return FX3_SUCCESS;
}

// Utility functions for descriptor information
BOOL usb_descriptors_is_super_speed_capable()
{
    return descriptor_table[USB_DESC_DEVICE_SS].is_available;
}

BOOL usb_descriptors_is_self_powered()
{
    // Check configuration descriptor for self-powered attribute
    // For this implementation, device is bus-powered
    return FALSE;
}

uint16_t usb_descriptors_get_vendor_id()
{
    return USB_VENDOR_ID;
}

uint16_t usb_descriptors_get_product_id()
{
    return USB_PRODUCT_ID;
}

uint16_t usb_descriptors_get_device_version()
{
    return USB_DEVICE_VERSION;
}