#include "dma.h"
#include "debug.h"
#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3utils.h"
#include "usb_descriptors.h"
#include "system_config.h"


// Internal DMA callback function
static void DmaCb(CyU3PDmaChannel *chHandle, CyU3PDmaCbType_t type,
                  CyU3PDmaCBInput_t *input);

// Fixed strings for data and event channels
static const char kDataStr[] = "data";
static const char kEventStr[] = "event";

CyU3PReturnStatus_t Dma_SendFixedStr(CyU3PDmaChannel *ch, const char *s)
{
    CyU3PDmaBuffer_t buf;
    uint16_t len;
    CyU3PReturnStatus_t status;

    if (!ch || !s) {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    len = (uint16_t)strlen(s);
    status = CyU3PDmaChannelGetBuffer(ch, &buf, CYU3P_NO_WAIT);
    if (status != CY_U3P_SUCCESS) {
        return status;
    }

    CyU3PMemCopy(buf.buffer, (uint8_t*)s, len);
    return CyU3PDmaChannelCommitBuffer(ch, len, 0);
}

CyU3PReturnStatus_t Dma_Init(DmaContext_t *ctx)
{
    if (!ctx) {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    // Initialize all DMA-related state
    memset(&ctx->sink.fx3, 0, sizeof(CyU3PDmaChannel));
    memset(&ctx->src.fx3, 0, sizeof(CyU3PDmaChannel));
    memset(&ctx->dataSrc.fx3, 0, sizeof(CyU3PDmaChannel));
    memset(&ctx->eventSrc.fx3, 0, sizeof(CyU3PDmaChannel));
    ctx->sink.owner = NULL;
    ctx->src.owner = NULL;
    ctx->dataSrc.owner = NULL;
    ctx->eventSrc.owner = NULL;
    ctx->dmaRxCount = 0;
    ctx->dmaTxCount = 0;
    ctx->dataTransStarted = CyFalse;
    ctx->srcEpFlush = CyFalse;
    ctx->cb = NULL;
    ctx->cb_user = NULL;

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
    uint32_t singleBufferSize = packetSize;

    // Set up owner references before creating channels
    ctx->sink.owner = ctx;
    ctx->src.owner = ctx;
    ctx->dataSrc.owner = ctx;
    ctx->eventSrc.owner = ctx;

    // Create DMA MANUAL_IN channel for the producer socket (OUT endpoint)
    CyU3PMemSet((uint8_t *)&dmaCfg, 0, sizeof(dmaCfg));
    dmaCfg.size = bufferSize;
    dmaCfg.count = DMA_BUFFER_COUNT;
    dmaCfg.prodSckId = USB_EP_PRODUCER_SOCKET;
    dmaCfg.consSckId = CY_U3P_CPU_SOCKET_CONS;
    dmaCfg.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.notification = CY_U3P_DMA_CB_PROD_EVENT;
    dmaCfg.cb = DmaCb;
    dmaCfg.prodHeader = 0;
    dmaCfg.prodFooter = 0;
    dmaCfg.consHeader = 0;
    dmaCfg.prodAvailCount = 0;

    apiRetStatus = CyU3PDmaChannelCreate(&ctx->sink.fx3,
                                         CY_U3P_DMA_TYPE_MANUAL_IN, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        return apiRetStatus;
    }

    // Create DMA MANUAL_OUT channel for the control consumer socket (IN endpoint)
    dmaCfg.notification = CY_U3P_DMA_CB_CONS_EVENT;
    dmaCfg.prodSckId = CY_U3P_CPU_SOCKET_PROD;
    dmaCfg.consSckId = USB_EP_CONSUMER_SOCKET;

    apiRetStatus = CyU3PDmaChannelCreate(&ctx->src.fx3,
                                         CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        // Clean up the first channel on failure
        CyU3PDmaChannelDestroy(&ctx->sink.fx3);
        return apiRetStatus;
    }

    dmaCfg.size = singleBufferSize;
    dmaCfg.count = 2;

    // Create DMA MANUAL_OUT channel for the data consumer socket (IN endpoint)
    dmaCfg.consSckId = USB_EP_DATA_SOCKET;

    apiRetStatus = CyU3PDmaChannelCreate(&ctx->dataSrc.fx3,
                                         CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        // Clean up the previous channels on failure
        CyU3PDmaChannelDestroy(&ctx->sink.fx3);
        CyU3PDmaChannelDestroy(&ctx->src.fx3);
        return apiRetStatus;
    }

    // Create DMA MANUAL_OUT channel for the event consumer socket (IN endpoint)
    dmaCfg.consSckId = USB_EP_EVENT_SOCKET;

    apiRetStatus = CyU3PDmaChannelCreate(&ctx->eventSrc.fx3,
                                         CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        // Clean up the previous channels on failure
        CyU3PDmaChannelDestroy(&ctx->sink.fx3);
        CyU3PDmaChannelDestroy(&ctx->src.fx3);
        CyU3PDmaChannelDestroy(&ctx->dataSrc.fx3);
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
    CyU3PDmaChannelDestroy(&ctx->sink.fx3);
    CyU3PDmaChannelDestroy(&ctx->src.fx3);
    CyU3PDmaChannelDestroy(&ctx->dataSrc.fx3);
    CyU3PDmaChannelDestroy(&ctx->eventSrc.fx3);
}

CyU3PReturnStatus_t Dma_StartTransfer(DmaContext_t *ctx)
{
    if (!ctx) {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    CyU3PReturnStatus_t apiRetStatus;

    // Set DMA channel transfer size for all channels
    apiRetStatus = CyU3PDmaChannelSetXfer(&ctx->sink.fx3,
                                          DMA_TRANSFER_SIZE_INFINITE);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        return apiRetStatus;
    }

    apiRetStatus = CyU3PDmaChannelSetXfer(&ctx->src.fx3,
                                          DMA_TRANSFER_SIZE_INFINITE);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        return apiRetStatus;
    }

    apiRetStatus = CyU3PDmaChannelSetXfer(&ctx->dataSrc.fx3,
                                          DMA_TRANSFER_SIZE_INFINITE);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        return apiRetStatus;
    }

    apiRetStatus = CyU3PDmaChannelSetXfer(&ctx->eventSrc.fx3,
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

    // Preload all buffers in the control MANUAL_OUT pipe with the required data
    for (index = 0; index < DMA_BUFFER_COUNT; index++) {
        stat = CyU3PDmaChannelGetBuffer(&ctx->src.fx3, &buf_p, CYU3P_NO_WAIT);
        if (stat != CY_U3P_SUCCESS) {
            CyU3PDebugPrint(4, "CyU3PDmaChannelGetBuffer failed, Error code = %d\n", stat);
            // Handle error appropriately
            CyFxAppErrorHandler(stat);
            break;
        }

        CyU3PMemSet(buf_p.buffer, BULK_DATA_PATTERN, buf_p.size);
        stat = CyU3PDmaChannelCommitBuffer(&ctx->src.fx3, buf_p.size, 0);
        if (stat != CY_U3P_SUCCESS) {
            CyU3PDebugPrint(4, "CyU3PDmaChannelCommitBuffer failed, Error code = %d\n", stat);
            // Handle error appropriately
            CyFxAppErrorHandler(stat);
            break;
        }
    }
}

void Dma_RegisterListener(DmaContext_t *ctx, dma_evt_cb_t cb, void *user)
{
    if (ctx) {
        ctx->cb = cb;
        ctx->cb_user = user;
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

// Internal DMA callback function using container_of pattern
static void DmaCb(CyU3PDmaChannel *chHandle, CyU3PDmaCbType_t type,
                  CyU3PDmaCBInput_t *input)
{
    // Get the wrapper structure (fx3 is first member, so address is same)
    DmaContext_t *ctx = CONTAINER_OF(chHandle, Fx3DmaCh_t, fx3)->owner;

    if (!ctx) {
        return;
    }

    CyU3PDmaBuffer_t buf_p;
    CyU3PDmaBuffer_t rsp_buf;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    // Mark data transfer as started
    ctx->dataTransStarted = CyTrue;

    if (type == CY_U3P_DMA_CB_PROD_EVENT) {
        // Producer event: parse command and generate responses
        status = CyU3PDmaChannelGetBuffer(chHandle, &buf_p, CYU3P_NO_WAIT);
        if (status == CY_U3P_SUCCESS) {
            uint16_t cmdLen = buf_p.count;
            if (cmdLen > (buf_p.size - 1)) {
                cmdLen = buf_p.size - 1;
            }
            buf_p.buffer[cmdLen] = '\0'; // Null terminate for safety

            // Send response to control-IN channel: "ok:{cmd}"
            status = CyU3PDmaChannelGetBuffer(&ctx->src.fx3, &rsp_buf, CYU3P_NO_WAIT);
            if (status == CY_U3P_SUCCESS) {
                uint16_t rspLen = 0;
                // Copy "ok:" prefix
                CyU3PMemCopy(rsp_buf.buffer, (uint8_t*)"ok:", 3);
                rspLen = 3;
                // Copy command
                if (cmdLen > 0 && (rspLen + cmdLen) < rsp_buf.size) {
                    CyU3PMemCopy(rsp_buf.buffer + rspLen, buf_p.buffer, cmdLen);
                    rspLen += cmdLen;
                }
                status = CyU3PDmaChannelCommitBuffer(&ctx->src.fx3, rspLen, 0);
                if (status != CY_U3P_SUCCESS) {
                    CyU3PDebugPrint(4, "Control-IN CommitBuffer failed, Error code = %d\n", status);
                }
            }

            // Send fixed data to data-IN channel
            status = Dma_SendFixedStr(&ctx->dataSrc.fx3, kDataStr);
            if (status != CY_U3P_SUCCESS) {
                CyU3PDebugPrint(4, "Data-IN SendFixedStr failed, Error code = %d\n", status);
            }

            // Send fixed event to event-IN channel
            status = Dma_SendFixedStr(&ctx->eventSrc.fx3, kEventStr);
            if (status != CY_U3P_SUCCESS) {
                CyU3PDebugPrint(4, "Event-IN SendFixedStr failed, Error code = %d\n", status);
            }

            // Discard the original command buffer
            status = CyU3PDmaChannelDiscardBuffer(chHandle);
            if (status != CY_U3P_SUCCESS) {
                CyU3PDebugPrint(4, "CyU3PDmaChannelDiscardBuffer failed, Error code = %d\n", status);
            }
        } else {
            CyU3PDebugPrint(4, "CyU3PDmaChannelGetBuffer failed, Error code = %d\n", status);
        }

        ctx->dmaRxCount++;

        // Notify application layer through callback
        if (ctx->cb) {
            ctx->cb(DMA_EVT_PROD, ctx->cb_user);
        }
    }

    if (type == CY_U3P_DMA_CB_CONS_EVENT) {
        // Consumer event: commit new buffer to implement data source
        status = CyU3PDmaChannelGetBuffer(chHandle, &buf_p, CYU3P_NO_WAIT);
        if (status == CY_U3P_SUCCESS) {
            // Determine which channel this is and fill appropriate data
            if (chHandle == &ctx->src.fx3) {
                // Control-IN channel: fill with pattern
                CyU3PMemSet(buf_p.buffer, BULK_DATA_PATTERN, buf_p.size);
                status = CyU3PDmaChannelCommitBuffer(chHandle, buf_p.size, 0);
            } else if (chHandle == &ctx->dataSrc.fx3) {
                // Data-IN channel: fill with "data"
                CyU3PMemCopy(buf_p.buffer, (uint8_t*)kDataStr, sizeof(kDataStr) - 1);
                status = CyU3PDmaChannelCommitBuffer(chHandle, sizeof(kDataStr) - 1, 0);
            } else if (chHandle == &ctx->eventSrc.fx3) {
                // Event-IN channel: fill with "event"
                CyU3PMemCopy(buf_p.buffer, (uint8_t*)kEventStr, sizeof(kEventStr) - 1);
                status = CyU3PDmaChannelCommitBuffer(chHandle, sizeof(kEventStr) - 1, 0);
            } else {
                // Unknown channel, fill with pattern
                CyU3PMemSet(buf_p.buffer, BULK_DATA_PATTERN, buf_p.size);
                status = CyU3PDmaChannelCommitBuffer(chHandle, buf_p.size, 0);
            }

            if (status != CY_U3P_SUCCESS) {
                CyU3PDebugPrint(4, "CyU3PDmaChannelCommitBuffer failed, Error code = %d\n", status);
            }
        } else {
            CyU3PDebugPrint(4, "CyU3PDmaChannelGetBuffer failed, Error code = %d\n", status);
        }
        ctx->dmaTxCount++;

        // Notify application layer through callback
        if (ctx->cb) {
            ctx->cb(DMA_EVT_CONS, ctx->cb_user);
        }
    }
}