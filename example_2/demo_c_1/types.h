#ifndef _TYPES_H_
#define _TYPES_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "error_codes.h"

#include <cyu3usb.h>

// Standard bool type for compatibility
#ifndef bool
#define bool CyBool_t
#define true CyTrue
#define false CyFalse
#endif

// USB speed enumeration
typedef enum {
    USB_SPEED_UNKNOWN = 0,
    USB_SPEED_FULL,                     // USB 1.1 Full Speed (12 Mbps)
    USB_SPEED_HIGH,                     // USB 2.0 High Speed (480 Mbps)
    USB_SPEED_SUPER                     // USB 3.0 Super Speed (5 Gbps)
} usb_speed_t;

// USB connection state
typedef enum {
    USB_STATE_DISCONNECTED = 0,
    USB_STATE_CONNECTED,
    USB_STATE_CONFIGURED,
    USB_STATE_SUSPENDED
} usb_state_t;

// USB event types for application callbacks
typedef enum {
    USB_EVENT_CONNECT = 0,
    USB_EVENT_DISCONNECT,
    USB_EVENT_CONFIGURE,
    USB_EVENT_SUSPEND,
    USB_EVENT_RESUME,
    USB_EVENT_RESET,
    USB_EVENT_EP0_SETUP,
    USB_EVENT_VBUS_REMOVED
} usb_event_type_t;

// DMA event types
typedef enum {
    DMA_EVENT_PRODUCE = 0,              // Data received from host
    DMA_EVENT_CONSUME,                  // Data sent to host
    DMA_EVENT_ERROR                     // DMA error occurred
} dma_event_type_t;

// Application thread events
typedef enum {
    APP_EVENT_USB_CONTROL = (1 << 0),   // USB control request pending
    APP_EVENT_HOST_WAKE = (1 << 1),     // Host wake request
    APP_EVENT_STANDBY = (1 << 2),       // Standby mode request
    APP_EVENT_RESET = (1 << 3)          // Reset request
} app_event_mask_t;

// Generic callback function types
typedef void (*usb_event_callback_t)(usb_event_type_t event, uint16_t data);
typedef void (*dma_event_callback_t)(dma_event_type_t event, void* user_data);
typedef bool (*setup_request_callback_t)(uint32_t setupdat0, uint32_t setupdat1);

// Forward declarations for handles
struct dma_channel_handle;
struct usb_manager_handle;

typedef struct dma_channel_handle dma_channel_handle_t;
typedef struct usb_manager_handle usb_manager_handle_t;

// USB packet size constants based on speed
#define USB_PACKET_SIZE_FULL_SPEED      64
#define USB_PACKET_SIZE_HIGH_SPEED      512
#define USB_PACKET_SIZE_SUPER_SPEED     1024

// Convert CyU3P USB speed to our enum
static inline usb_speed_t usb_speed_from_cyu3p(CyU3PUSBSpeed_t cy_speed)
{
    switch (cy_speed) {
        case CY_U3P_FULL_SPEED:  return USB_SPEED_FULL;
        case CY_U3P_HIGH_SPEED:  return USB_SPEED_HIGH;
        case CY_U3P_SUPER_SPEED: return USB_SPEED_SUPER;
        default:                 return USB_SPEED_UNKNOWN;
    }
}

// Get packet size for USB speed
static inline uint16_t usb_get_packet_size(usb_speed_t speed)
{
    switch (speed) {
        case USB_SPEED_FULL:  return USB_PACKET_SIZE_FULL_SPEED;
        case USB_SPEED_HIGH:  return USB_PACKET_SIZE_HIGH_SPEED;
        case USB_SPEED_SUPER: return USB_PACKET_SIZE_SUPER_SPEED;
        default:              return USB_PACKET_SIZE_FULL_SPEED;
    }
}

#endif // _TYPES_H_