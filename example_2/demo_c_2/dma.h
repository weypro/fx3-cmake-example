#ifndef _DMA_H_
#define _DMA_H_

#include "cyu3types.h"
#include "cyu3dma.h"
#include "cyu3usb.h"


typedef struct AppContext_t AppContext_t;

// DMA management context structure
typedef struct DmaContext_t {
    CyU3PDmaChannel chHandleBulkSink;       // DMA MANUAL_IN channel handle
    CyU3PDmaChannel chHandleBulkSrc;        // DMA MANUAL_OUT channel handle
    uint32_t dmaRxCount;                    // Counter to track buffers received
    uint32_t dmaTxCount;                    // Counter to track buffers transmitted
    CyBool_t dataTransStarted;              // Whether DMA transfer has started after enumeration
    volatile CyBool_t srcEpFlush;           // Source endpoint flush trigger
    AppContext_t *appCtx;              // Pointer to main app context for callbacks
} DmaContext_t;

// DMA module API functions
CyU3PReturnStatus_t Dma_Init(DmaContext_t *ctx);
void Dma_Deinit(DmaContext_t *ctx);

// DMA channel management
CyU3PReturnStatus_t Dma_SetupChannels(DmaContext_t *ctx, uint16_t packetSize, 
                                       CyU3PUSBSpeed_t usbSpeed);
void Dma_TeardownChannels(DmaContext_t *ctx);
CyU3PReturnStatus_t Dma_StartTransfer(DmaContext_t *ctx);

// Buffer management
void Dma_FillInBuffers(DmaContext_t *ctx);

// State management
void Dma_SetDataTransferStarted(DmaContext_t *ctx, CyBool_t started);
void Dma_TriggerSrcEpFlush(DmaContext_t *ctx);
void Dma_ClearSrcEpFlush(DmaContext_t *ctx);
void Dma_SetAppContext(DmaContext_t *ctx, AppContext_t *appCtx);

// State query functions
CyBool_t Dma_IsDataTransferStarted(const DmaContext_t *ctx);
CyBool_t Dma_ShouldFlushSrcEp(const DmaContext_t *ctx);
uint32_t Dma_GetRxCount(const DmaContext_t *ctx);
uint32_t Dma_GetTxCount(const DmaContext_t *ctx);

void Dma_SetCallbackContext(DmaContext_t *ctx);

// DMA callback function
void CyFxBulkSrcSinkDmaCallback(CyU3PDmaChannel *chHandle, CyU3PDmaCbType_t type, 
                                CyU3PDmaCBInput_t *input);

#endif // _DMA_H_