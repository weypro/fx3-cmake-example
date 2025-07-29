#include "usb_ctrl.h"
#include "dma.h"
#include "power.h"
#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3utils.h"
#include "usb_descriptors.h"
#include "system_config.h"
#include "cyfxbulksrcsink.h"

// External variables from main.c
extern CyBool_t glIsApplnActive;

volatile uint32_t glEp0StatCount = 0;           // Number of EP0 status events received.
uint8_t glEp0Buffer[VENDOR_CMD_BUFFER_ALIGN] __attribute__ ((aligned (32))); // Local buffer used for vendor command handling.

// Control request related variables.
CyU3PEvent glBulkLpEvent;       // Event group used to signal the thread that there is a pending request.
uint32_t   gl_setupdat0;        // Variable that holds the setupdat0 value (bmRequestType, bRequest and wValue).
uint32_t   gl_setupdat1;        // Variable that holds the setupdat1 value (wIndex and wLength).

void
CyFxBulkSrcSinkApplnEpEvtCB (
        CyU3PUsbEpEvtType evtype,
        CyU3PUSBSpeed_t   speed,
        uint8_t           epNum)
{
    // Hit an endpoint retry case. Need to stall and flush the endpoint for recovery.
    if (evtype == CYU3P_USBEP_SS_RETRY_EVT)
    {
        glSrcEpFlush = CyTrue;
    }
}

// Callback to handle the USB setup requests.
CyBool_t
CyFxBulkSrcSinkApplnUSBSetupCB (
        uint32_t setupdat0, // SETUP Data 0
        uint32_t setupdat1  // SETUP Data 1
)
{
    // Fast enumeration is used. Only requests addressed to the interface, class,
    // vendor and unknown control requests are received by this function.
    // This application does not support any class or vendor requests.

    uint8_t  bRequest, bReqType;
    uint8_t  bType, bTarget;
    uint16_t wValue, wIndex;
    CyBool_t isHandled = CyFalse;

    // Decode the fields from the setup request.
    bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bType    = (bReqType & CY_U3P_USB_TYPE_MASK);
    bTarget  = (bReqType & CY_U3P_USB_TARGET_MASK);
    bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue   = ((setupdat0 & CY_U3P_USB_VALUE_MASK)   >> CY_U3P_USB_VALUE_POS);
    wIndex   = ((setupdat1 & CY_U3P_USB_INDEX_MASK)   >> CY_U3P_USB_INDEX_POS);

    if (bType == CY_U3P_USB_STANDARD_RQT)
    {
        // Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
        // requests here. It should be allowed to pass if the device is in configured
        // state and failed otherwise.
        if ((bTarget == CY_U3P_USB_TARGET_INTF) && ((bRequest == CY_U3P_USB_SC_SET_FEATURE)
                                                    || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) && (wValue == 0))
        {
            if (glIsApplnActive)
            {
                CyU3PUsbAckSetup ();

                // As we have only one interface, the link can be pushed into U2 state as soon as
                // this interface is suspended.
                if (bRequest == CY_U3P_USB_SC_SET_FEATURE)
                {
                    glDataTransStarted = CyFalse;
                    glForceLinkU2      = CyTrue;
                }
                else
                {
                    glForceLinkU2 = CyFalse;
                }
            }
            else
                CyU3PUsbStall (0, CyTrue, CyFalse);

            isHandled = CyTrue;
        }

        // CLEAR_FEATURE request for endpoint is always passed to the setup callback
        // regardless of the enumeration model used. When a clear feature is received,
        // the previous transfer has to be flushed and cleaned up. This is done at the
        // protocol level. Since this is just a loopback operation, there is no higher
        // level protocol. So flush the EP memory and reset the DMA channel associated
        // with it. If there are more than one EP associated with the channel reset both
        // the EPs. The endpoint stall and toggle / sequence number is also expected to be
        // reset. Return CyFalse to make the library clear the stall and reset the endpoint
        // toggle. Or invoke the CyU3PUsbStall (ep, CyFalse, CyTrue) and return CyTrue.
        // Here we are clearing the stall.
        if ((bTarget == CY_U3P_USB_TARGET_ENDPT) && (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)
            && (wValue == CY_U3P_USBX_FS_EP_HALT))
        {
            if (glIsApplnActive)
            {
                if (wIndex == USB_EP_PRODUCER)
                {
                    CyU3PUsbSetEpNak (USB_EP_PRODUCER, CyTrue);
                    CyU3PBusyWait (125);

                    CyU3PDmaChannelReset (&glChHandleBulkSink);
                    CyU3PUsbFlushEp(USB_EP_PRODUCER);
                    CyU3PUsbResetEp (USB_EP_PRODUCER);
                    CyU3PUsbSetEpNak (USB_EP_PRODUCER, CyFalse);

                    CyU3PDmaChannelSetXfer (&glChHandleBulkSink, CY_FX_BULKSRCSINK_DMA_TX_SIZE);
                    CyU3PUsbStall (wIndex, CyFalse, CyTrue);
                    isHandled = CyTrue;
                    CyU3PUsbAckSetup ();
                }

                if (wIndex == USB_EP_CONSUMER)
                {
                    CyU3PUsbSetEpNak (USB_EP_CONSUMER, CyTrue);
                    CyU3PBusyWait (125);

                    CyU3PDmaChannelReset (&glChHandleBulkSrc);
                    CyU3PUsbFlushEp(USB_EP_CONSUMER);
                    CyU3PUsbResetEp (USB_EP_CONSUMER);
                    CyU3PUsbSetEpNak (USB_EP_CONSUMER, CyFalse);

                    CyU3PDmaChannelSetXfer (&glChHandleBulkSrc, CY_FX_BULKSRCSINK_DMA_TX_SIZE);
                    CyU3PUsbStall (wIndex, CyFalse, CyTrue);
                    isHandled = CyTrue;
                    CyU3PUsbAckSetup ();

                    CyFxBulkSrcSinkFillInBuffers ();
                }
            }
        }
    }

    if ((bType == CY_U3P_USB_VENDOR_RQT) && (bTarget == CY_U3P_USB_TARGET_DEVICE))
    {
        // We set an event here and let the application thread below handle these requests.
        // isHandled needs to be set to True, so that the driver does not stall EP0.
        isHandled = CyTrue;
        gl_setupdat0 = setupdat0;
        gl_setupdat1 = setupdat1;
        CyU3PEventSet (&glBulkLpEvent, CYFX_USB_CTRL_TASK, CYU3P_EVENT_OR);
    }

    return isHandled;
}