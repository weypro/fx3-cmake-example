#ifndef _USB_CTRL_H_
#define _USB_CTRL_H_

#include "cyu3os.h"
#include "cyu3types.h"
#include "cyu3usb.h"
#include "system_config.h"

// Event definitions
#define CYFX_USB_CTRL_TASK      (1 << 0)        // Event that indicates that there is a pending USB control request.
#define CYFX_USB_HOSTWAKE_TASK  (1 << 1)        // Event that indicates the a Remote Wake should be attempted.

// Global variables
extern volatile uint32_t glEp0StatCount;           // Number of EP0 status events received.
extern uint8_t glEp0Buffer[VENDOR_CMD_BUFFER_ALIGN]; // Local buffer used for vendor command handling.
extern CyU3PEvent glBulkLpEvent;       // Event group used to signal the thread that there is a pending request.
extern uint32_t   gl_setupdat0;        // Variable that holds the setupdat0 value (bmRequestType, bRequest and wValue).
extern uint32_t   gl_setupdat1;        // Variable that holds the setupdat1 value (wIndex and wLength).

// Function declarations
CyBool_t CyFxBulkSrcSinkApplnUSBSetupCB(uint32_t setupdat0, uint32_t setupdat1);
void CyFxBulkSrcSinkApplnEpEvtCB(CyU3PUsbEpEvtType evtype, CyU3PUSBSpeed_t speed, uint8_t epNum);

#endif // _USB_CTRL_H_