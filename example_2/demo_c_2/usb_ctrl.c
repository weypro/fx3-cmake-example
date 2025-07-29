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

#include "app.h"

// Static context pointer for callbacks (SDK limitation workaround)
static AppContext_t *g_usbCallbackContext = NULL;

CyU3PReturnStatus_t UsbCtrl_Init(UsbCtrlContext_t *ctx)
{
    if (!ctx) {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    // Initialize all USB control related state
    ctx->ep0StatCount = 0;
    memset(ctx->ep0Buffer, 0, sizeof(ctx->ep0Buffer));
    ctx->setupdat0 = 0;
    ctx->setupdat1 = 0;
    ctx->appCtx = NULL;

    return CY_U3P_SUCCESS;
}

void UsbCtrl_Deinit(UsbCtrlContext_t *ctx)
{
    if (!ctx) {
        return;
    }

    // Clear context
    memset(ctx, 0, sizeof(UsbCtrlContext_t));
}

void UsbCtrl_SetAppContext(UsbCtrlContext_t *ctx, AppContext_t *appCtx)
{
    if (ctx) {
        ctx->appCtx = appCtx;
    }
}

void UsbCtrl_SetCallbackContext(AppContext_t *appCtx)
{
    g_usbCallbackContext = appCtx;
}

CyU3PReturnStatus_t UsbCtrl_RegisterCallbacks()
{
    if (!g_usbCallbackContext) {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    // Register setup callback
    CyU3PUsbRegisterSetupCallback(CyFxBulkSrcSinkApplnUSBSetupCB, CyTrue);

    return CY_U3P_SUCCESS;
}

uint32_t UsbCtrl_GetEp0StatCount(const UsbCtrlContext_t *ctx)
{
    return ctx ? ctx->ep0StatCount : 0;
}

uint8_t* UsbCtrl_GetEp0Buffer(UsbCtrlContext_t *ctx)
{
    return ctx ? ctx->ep0Buffer : NULL;
}

uint32_t UsbCtrl_GetSetupData0(const UsbCtrlContext_t *ctx)
{
    return ctx ? ctx->setupdat0 : 0;
}

uint32_t UsbCtrl_GetSetupData1(const UsbCtrlContext_t *ctx)
{
    return ctx ? ctx->setupdat1 : 0;
}

void UsbCtrl_SetSetupData(UsbCtrlContext_t *ctx, uint32_t setupdat0, uint32_t setupdat1)
{
    if (ctx) {
        ctx->setupdat0 = setupdat0;
        ctx->setupdat1 = setupdat1;
    }
}

void UsbCtrl_IncrementEp0StatCount(UsbCtrlContext_t *ctx)
{
    if (ctx) {
        ctx->ep0StatCount++;
    }
}

// Endpoint event callback
void CyFxBulkSrcSinkApplnEpEvtCB(CyU3PUsbEpEvtType evtype, CyU3PUSBSpeed_t speed, uint8_t epNum)
{
    if (!g_usbCallbackContext) {
        return;
    }

    // Hit an endpoint retry case. Need to stall and flush the endpoint for recovery.
    if (evtype == CYU3P_USBEP_SS_RETRY_EVT) {
        // Use DMA module API to trigger flush
        Dma_TriggerSrcEpFlush(&g_usbCallbackContext->dma);
    }
}

// USB setup request callback
CyBool_t CyFxBulkSrcSinkApplnUSBSetupCB(uint32_t setupdat0, uint32_t setupdat1)
{
    if (!g_usbCallbackContext) {
        return CyFalse;
    }

    uint8_t bRequest, bReqType;
    uint8_t bType, bTarget;
    uint16_t wValue, wIndex;
    CyBool_t isHandled = CyFalse;

    // Decode the fields from the setup request
    bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bType = (bReqType & CY_U3P_USB_TYPE_MASK);
    bTarget = (bReqType & CY_U3P_USB_TARGET_MASK);
    bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue = ((setupdat0 & CY_U3P_USB_VALUE_MASK) >> CY_U3P_USB_VALUE_POS);
    wIndex = ((setupdat1 & CY_U3P_USB_INDEX_MASK) >> CY_U3P_USB_INDEX_POS);

    if (bType == CY_U3P_USB_STANDARD_RQT) {
        // Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
        if ((bTarget == CY_U3P_USB_TARGET_INTF) &&
            ((bRequest == CY_U3P_USB_SC_SET_FEATURE) || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) &&
            (wValue == 0)) {
            if (g_usbCallbackContext->isApplnActive) {
                CyU3PUsbAckSetup();

                // Use power module API for link U2 control
                if (bRequest == CY_U3P_USB_SC_SET_FEATURE) {
                    Dma_SetDataTransferStarted(&g_usbCallbackContext->dma, CyFalse);
                    Power_SetForceLinkU2(&g_usbCallbackContext->power, CyTrue);
                } else {
                    Power_SetForceLinkU2(&g_usbCallbackContext->power, CyFalse);
                }
            } else {
                CyU3PUsbStall(0, CyTrue, CyFalse);
            }
            isHandled = CyTrue;
        }

        // CLEAR_FEATURE request for endpoint
        if ((bTarget == CY_U3P_USB_TARGET_ENDPT) && (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)
            && (wValue == CY_U3P_USBX_FS_EP_HALT)) {
            if (g_usbCallbackContext->isApplnActive) {
                if (wIndex == USB_EP_PRODUCER) {
                    CyU3PUsbSetEpNak(USB_EP_PRODUCER, CyTrue);
                    CyU3PBusyWait(125);

                    // Use DMA module API for channel operations
                    CyU3PDmaChannelReset(&g_usbCallbackContext->dma.chHandleBulkSink);
                    CyU3PUsbFlushEp(USB_EP_PRODUCER);
                    CyU3PUsbResetEp(USB_EP_PRODUCER);
                    CyU3PUsbSetEpNak(USB_EP_PRODUCER, CyFalse);

                    CyU3PDmaChannelSetXfer(&g_usbCallbackContext->dma.chHandleBulkSink, DMA_TRANSFER_SIZE_INFINITE);
                    CyU3PUsbStall(wIndex, CyFalse, CyTrue);
                    isHandled = CyTrue;
                    CyU3PUsbAckSetup();
                }

                if (wIndex == USB_EP_CONSUMER) {
                    CyU3PUsbSetEpNak(USB_EP_CONSUMER, CyTrue);
                    CyU3PBusyWait(125);

                    // Use DMA module API for channel operations
                    CyU3PDmaChannelReset(&g_usbCallbackContext->dma.chHandleBulkSrc);
                    CyU3PUsbFlushEp(USB_EP_CONSUMER);
                    CyU3PUsbResetEp(USB_EP_CONSUMER);
                    CyU3PUsbSetEpNak(USB_EP_CONSUMER, CyFalse);

                    CyU3PDmaChannelSetXfer(&g_usbCallbackContext->dma.chHandleBulkSrc, DMA_TRANSFER_SIZE_INFINITE);
                    CyU3PUsbStall(wIndex, CyFalse, CyTrue);
                    isHandled = CyTrue;
                    CyU3PUsbAckSetup();

                    // Use DMA module API to fill buffers
                    Dma_FillInBuffers(&g_usbCallbackContext->dma);
                }
            }
        }
    }

    if ((bType == CY_U3P_USB_VENDOR_RQT) && (bTarget == CY_U3P_USB_TARGET_DEVICE)) {
        // Set an event and let the application thread handle these requests
        isHandled = CyTrue;
        UsbCtrl_SetSetupData(&g_usbCallbackContext->usbCtrl, setupdat0, setupdat1);
        CyU3PEventSet(&g_usbCallbackContext->bulkLpEvent, CYFX_USB_CTRL_TASK, CYU3P_EVENT_OR);
    }

    return isHandled;
}
