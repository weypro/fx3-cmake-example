#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

#include "types.h"
#include "system_config.h"
#include "cyu3usb.h"

// USB descriptor type enumeration
typedef enum {
    USB_DESC_DEVICE_SS = 0,        // Super Speed device descriptor
    USB_DESC_DEVICE_HS,            // High Speed device descriptor  
    USB_DESC_DEVICE_QUALIFIER,     // Device qualifier descriptor
    USB_DESC_CONFIG_SS,            // Super Speed configuration descriptor
    USB_DESC_CONFIG_HS,            // High Speed configuration descriptor
    USB_DESC_CONFIG_FS,            // Full Speed configuration descriptor
    USB_DESC_BOS,                  // Binary Object Store descriptor
    USB_DESC_STRING_LANG,          // Language ID string descriptor
    USB_DESC_STRING_MANUFACTURER,  // Manufacturer string descriptor
    USB_DESC_STRING_PRODUCT,       // Product string descriptor
    USB_DESC_MS_OS,                // Microsoft OS descriptor
    USB_DESC_COUNT                 // Total number of descriptor types
} usb_descriptor_type_t;

// USB descriptor information structure
typedef struct {
    const uint8_t* data;           // Pointer to descriptor data
    uint16_t length;               // Descriptor length in bytes
    BOOL is_available;             // Whether descriptor is available
} usb_descriptor_info_t;

// USB endpoint configuration for different speeds
typedef struct {
    uint16_t max_packet_size;      // Maximum packet size for this speed
    uint8_t burst_length;          // Burst length (USB 3.0 only)
    uint8_t interval;              // Polling interval
} usb_endpoint_config_t;

// Get descriptor data for specific type
const usb_descriptor_info_t* usb_descriptors_get_info(usb_descriptor_type_t type);

// Get descriptor data pointer (for compatibility with CyU3P APIs)
const uint8_t* usb_descriptors_get_data(usb_descriptor_type_t type);

// Get descriptor length
uint16_t usb_descriptors_get_length(usb_descriptor_type_t type);

// Register all descriptors with USB driver
fx3_result_t usb_descriptors_register_all();

// Get endpoint configuration for current USB speed
usb_endpoint_config_t usb_descriptors_get_endpoint_config(usb_speed_t speed);

// Validate descriptor integrity
fx3_result_t usb_descriptors_validate();

// Descriptor utility functions
BOOL usb_descriptors_is_super_speed_capable();
BOOL usb_descriptors_is_self_powered();
uint16_t usb_descriptors_get_vendor_id();
uint16_t usb_descriptors_get_product_id();
uint16_t usb_descriptors_get_device_version();

// External descriptor declarations (defined in usb_descriptors.c)
extern const uint8_t usb_device_descriptor_ss[];
extern const uint8_t usb_device_descriptor_hs[];
extern const uint8_t usb_device_qualifier_descriptor[];
extern const uint8_t usb_config_descriptor_ss[];
extern const uint8_t usb_config_descriptor_hs[];
extern const uint8_t usb_config_descriptor_fs[];
extern const uint8_t usb_bos_descriptor[];
extern const uint8_t usb_string_lang_descriptor[];
extern const uint8_t usb_string_manufacturer_descriptor[];
extern const uint8_t usb_string_product_descriptor[];
extern const uint8_t usb_ms_os_descriptor[];

#endif // _USB_DESCRIPTORS_H_