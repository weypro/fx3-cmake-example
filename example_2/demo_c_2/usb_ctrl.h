#ifndef _USB_CTRL_H_
#define _USB_CTRL_H_

#include "cyu3os.h"
#include "cyu3types.h"
#include "cyu3usb.h"
#include "system_config.h"

// Forward declaration
typedef struct AppContext_t AppContext_t;

// USB Control module context structure
typedef struct UsbCtrlContext_t {
    volatile uint32_t ep0StatCount;           // Number of EP0 status events received
    uint8_t ep0Buffer[VENDOR_CMD_BUFFER_ALIGN]; // Local buffer for vendor command handling
    uint32_t setupdat0;                       // Variable that holds setupdat0 value (bmRequestType, bRequest and wValue)
    uint32_t setupdat1;                       // Variable that holds setupdat1 value (wIndex and wLength)
    AppContext_t *appCtx;                     // Pointer to main app context for callbacks
} UsbCtrlContext_t;

// Event definitions
#define CYFX_USB_CTRL_TASK      (1 << 0)        // Event that indicates that there is a pending USB control request
#define CYFX_USB_HOSTWAKE_TASK  (1 << 1)        // Event that indicates the a Remote Wake should be attempted

// Initialize the USB Control module context
CyU3PReturnStatus_t UsbCtrl_Init(UsbCtrlContext_t *ctx);

// Deinitialize the USB Control module
void UsbCtrl_Deinit(UsbCtrlContext_t *ctx);

// Set the application context for callbacks
void UsbCtrl_SetAppContext(UsbCtrlContext_t *ctx, AppContext_t *appCtx);

// Set the callback context for SDK callbacks (workaround for SDK limitation)
void UsbCtrl_SetCallbackContext(AppContext_t *appCtx);

// Register USB callbacks with the SDK
CyU3PReturnStatus_t UsbCtrl_RegisterCallbacks(void);

// State query and modification functions
uint32_t UsbCtrl_GetEp0StatCount(const UsbCtrlContext_t *ctx);
uint8_t* UsbCtrl_GetEp0Buffer(UsbCtrlContext_t *ctx);
uint32_t UsbCtrl_GetSetupData0(const UsbCtrlContext_t *ctx);
uint32_t UsbCtrl_GetSetupData1(const UsbCtrlContext_t *ctx);
void UsbCtrl_SetSetupData(UsbCtrlContext_t *ctx, uint32_t setupdat0, uint32_t setupdat1);

// Increment EP0 status count (called from USB event callback)
void UsbCtrl_IncrementEp0StatCount(UsbCtrlContext_t *ctx);

// USB callback functions (to be registered with the SDK)
CyBool_t CyFxBulkSrcSinkApplnUSBSetupCB(uint32_t setupdat0, uint32_t setupdat1);
void CyFxBulkSrcSinkApplnEpEvtCB(CyU3PUsbEpEvtType evtype, CyU3PUSBSpeed_t speed, uint8_t epNum);

#endif // _USB_CTRL_H_