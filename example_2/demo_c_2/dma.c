#include "dma.h"
#include "power.h"
#include "debug.h"
#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3utils.h"
#include "usb_descriptors.h"
#include "system_config.h"

#include "app.h"

CyU3PReturnStatus_t Dma_Init(DmaContext_t *ctx)
{
    if (!ctx) {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    // Initialize all DMA-related state
    memset(&ctx->chHandleBulkSink, 0, sizeof(CyU3PDmaChannel));
    memset(&ctx->chHandleBulkSrc, 0, sizeof(CyU3PDmaChannel));
    ctx->dmaRxCount = 0;
    ctx->dmaTxCount = 0;
    ctx->dataTransStarted = CyFalse;
    ctx->srcEpFlush = CyFalse;
    ctx->appCtx = NULL;

    return CY_U3P_SUCCESS;
}

void Dma_Deinit(DmaContext_t *ctx)
{
    if (!ctx) {
        return;
    }

    // Teardown channels if they were set up
    Dma_TeardownChannels(ctx);
}

CyU3PReturnStatus_t Dma_SetupChannels(DmaContext_t *ctx, uint16_t packetSize, 
                                       CyU3PUSBSpeed_t usbSpeed)
{
    if (!ctx) {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    CyU3PDmaChannelConfig_t dmaCfg;
    CyU3PReturnStatus_t apiRetStatus;

    // Calculate buffer size based on USB speed and configuration
    uint32_t bufferSize = (packetSize * DMA_BURST_LENGTH) * DMA_SIZE_MULTIPLIER;

    // Create DMA MANUAL_IN channel for the producer socket (OUT endpoint)
    CyU3PMemSet((uint8_t *)&dmaCfg, 0, sizeof(dmaCfg));
    dmaCfg.size = bufferSize;
    dmaCfg.count = DMA_BUFFER_COUNT;
    dmaCfg.prodSckId = USB_EP_PRODUCER_SOCKET;
    dmaCfg.consSckId = CY_U3P_CPU_SOCKET_CONS;
    dmaCfg.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.notification = CY_U3P_DMA_CB_PROD_EVENT;
    dmaCfg.cb = CyFxBulkSrcSinkDmaCallback;
    dmaCfg.prodHeader = 0;
    dmaCfg.prodFooter = 0;
    dmaCfg.consHeader = 0;
    dmaCfg.prodAvailCount = 0;

    apiRetStatus = CyU3PDmaChannelCreate(&ctx->chHandleBulkSink,
                                         CY_U3P_DMA_TYPE_MANUAL_IN, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        return apiRetStatus;
    }

    // Create DMA MANUAL_OUT channel for the consumer socket (IN endpoint)
    dmaCfg.notification = CY_U3P_DMA_CB_CONS_EVENT;
    dmaCfg.prodSckId = CY_U3P_CPU_SOCKET_PROD;
    dmaCfg.consSckId = USB_EP_CONSUMER_SOCKET;

    apiRetStatus = CyU3PDmaChannelCreate(&ctx->chHandleBulkSrc,
                                         CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        // Clean up the first channel on failure
        CyU3PDmaChannelDestroy(&ctx->chHandleBulkSink);
        return apiRetStatus;
    }

    return CY_U3P_SUCCESS;
}

void Dma_TeardownChannels(DmaContext_t *ctx)
{
    if (!ctx) {
        return;
    }

    // Destroy the DMA channels
    CyU3PDmaChannelDestroy(&ctx->chHandleBulkSink);
    CyU3PDmaChannelDestroy(&ctx->chHandleBulkSrc);
}

CyU3PReturnStatus_t Dma_StartTransfer(DmaContext_t *ctx)
{
    if (!ctx) {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    CyU3PReturnStatus_t apiRetStatus;

    // Set DMA channel transfer size for both channels
    apiRetStatus = CyU3PDmaChannelSetXfer(&ctx->chHandleBulkSink, 
                                          DMA_TRANSFER_SIZE_INFINITE);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        return apiRetStatus;
    }

    apiRetStatus = CyU3PDmaChannelSetXfer(&ctx->chHandleBulkSrc, 
                                          DMA_TRANSFER_SIZE_INFINITE);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        return apiRetStatus;
    }

    return CY_U3P_SUCCESS;
}

void Dma_FillInBuffers(DmaContext_t *ctx)
{
    if (!ctx) {
        return;
    }

    CyU3PReturnStatus_t stat;
    CyU3PDmaBuffer_t buf_p;
    uint16_t index = 0;

    // Preload all buffers in the MANUAL_OUT pipe with the required data
    for (index = 0; index < DMA_BUFFER_COUNT; index++) {
        stat = CyU3PDmaChannelGetBuffer(&ctx->chHandleBulkSrc, &buf_p, CYU3P_NO_WAIT);
        if (stat != CY_U3P_SUCCESS) {
            CyU3PDebugPrint(4, "CyU3PDmaChannelGetBuffer failed, Error code = %d\n", stat);
            // Handle error appropriately
            break;
        }

        CyU3PMemSet(buf_p.buffer, BULK_DATA_PATTERN, buf_p.size);
        stat = CyU3PDmaChannelCommitBuffer(&ctx->chHandleBulkSrc, buf_p.size, 0);
        if (stat != CY_U3P_SUCCESS) {
            CyU3PDebugPrint(4, "CyU3PDmaChannelCommitBuffer failed, Error code = %d\n", stat);
            // Handle error appropriately
            break;
        }
    }
}

void Dma_SetDataTransferStarted(DmaContext_t *ctx, CyBool_t started)
{
    if (ctx) {
        ctx->dataTransStarted = started;
    }
}

void Dma_TriggerSrcEpFlush(DmaContext_t *ctx)
{
    if (ctx) {
        ctx->srcEpFlush = CyTrue;
    }
}

void Dma_ClearSrcEpFlush(DmaContext_t *ctx)
{
    if (ctx) {
        ctx->srcEpFlush = CyFalse;
    }
}

void Dma_SetAppContext(DmaContext_t *ctx, AppContext_t *appCtx)
{
    if (ctx) {
        ctx->appCtx = appCtx;
    }
}

CyBool_t Dma_IsDataTransferStarted(const DmaContext_t *ctx)
{
    return ctx ? ctx->dataTransStarted : CyFalse;
}

CyBool_t Dma_ShouldFlushSrcEp(const DmaContext_t *ctx)
{
    return ctx ? ctx->srcEpFlush : CyFalse;
}

uint32_t Dma_GetRxCount(const DmaContext_t *ctx)
{
    return ctx ? ctx->dmaRxCount : 0;
}

uint32_t Dma_GetTxCount(const DmaContext_t *ctx)
{
    return ctx ? ctx->dmaTxCount : 0;
}

// Global pointer for DMA callback context (SDK limitation workaround)
static DmaContext_t *g_dmaCallbackContext = NULL;

void CyFxBulkSrcSinkDmaCallback(CyU3PDmaChannel *chHandle, CyU3PDmaCbType_t type, 
                                CyU3PDmaCBInput_t *input)
{
    if (!g_dmaCallbackContext || !g_dmaCallbackContext->appCtx) {
        return;
    }

    DmaContext_t *ctx = g_dmaCallbackContext;
    AppContext_t *appCtx = ctx->appCtx;
    CyU3PDmaBuffer_t buf_p;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    ctx->dataTransStarted = CyTrue;

    // Start/restart the timer and disable LPM
    CyU3PUsbLPMDisable();
    CyU3PTimerStop(&appCtx->power.lpmTimer);
    CyU3PTimerModify(&appCtx->power.lpmTimer, LPM_TIMER_TIMEOUT, 0);
    CyU3PTimerStart(&appCtx->power.lpmTimer);

    if (type == CY_U3P_DMA_CB_PROD_EVENT) {
        // Producer event: discard buffer to implement data sink
        status = CyU3PDmaChannelDiscardBuffer(chHandle);
        if (status != CY_U3P_SUCCESS) {
            CyU3PDebugPrint(4, "CyU3PDmaChannelDiscardBuffer failed, Error code = %d\n", status);
        }
        ctx->dmaRxCount++;
    }
    
    if (type == CY_U3P_DMA_CB_CONS_EVENT) {
        // Consumer event: commit new buffer to implement data source
        status = CyU3PDmaChannelGetBuffer(chHandle, &buf_p, CYU3P_NO_WAIT);
        if (status == CY_U3P_SUCCESS) {
            status = CyU3PDmaChannelCommitBuffer(chHandle, buf_p.size, 0);
            if (status != CY_U3P_SUCCESS) {
                CyU3PDebugPrint(4, "CyU3PDmaChannelCommitBuffer failed, Error code = %d\n", status);
            }
        } else {
            CyU3PDebugPrint(4, "CyU3PDmaChannelGetBuffer failed, Error code = %d\n", status);
        }
        ctx->dmaTxCount++;
    }
}

// Function to set the callback context (must be called before using DMA)
void Dma_SetCallbackContext(DmaContext_t *ctx)
{
    g_dmaCallbackContext = ctx;
}

// Missing function declaration that was referenced
extern void CyFxAppErrorHandler(CyU3PReturnStatus_t apiRetStatus);