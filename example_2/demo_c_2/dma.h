#ifndef _DMA_H_
#define _DMA_H_

#include "cyu3types.h"
#include "cyu3dma.h"
#include "cyu3usb.h"


// DMA event types for callback notification
typedef enum {
    DMA_EVT_PROD,  // Producer event (data received)
    DMA_EVT_CONS   // Consumer event (data transmitted)
} dma_evt_t;

// DMA event callback function type
typedef void (*dma_evt_cb_t)(dma_evt_t evt, void *user);

// Wrapper structure for FX3 DMA channel with owner reference
typedef struct Fx3DmaCh_t {
    CyU3PDmaChannel fx3;        // Must be first member for address compatibility
    struct DmaContext_t *owner; // Back reference to owning context
} Fx3DmaCh_t;

// DMA management context structure
typedef struct DmaContext_t {
    Fx3DmaCh_t sink;                        // DMA MANUAL_IN channel (Producer endpoint)
    Fx3DmaCh_t src;                         // DMA MANUAL_OUT channel (Consumer endpoint)
    uint32_t dmaRxCount;                    // Counter to track buffers received
    uint32_t dmaTxCount;                    // Counter to track buffers transmitted
    CyBool_t dataTransStarted;              // Whether DMA transfer has started after enumeration
    volatile CyBool_t srcEpFlush;           // Source endpoint flush trigger
    dma_evt_cb_t cb;                        // Event callback function
    void *cb_user;                          // User data for callback
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

// Event callback registration
void Dma_RegisterListener(DmaContext_t *ctx, dma_evt_cb_t cb, void *user);

// State management
void Dma_SetDataTransferStarted(DmaContext_t *ctx, CyBool_t started);
void Dma_TriggerSrcEpFlush(DmaContext_t *ctx);
void Dma_ClearSrcEpFlush(DmaContext_t *ctx);

// State query functions
CyBool_t Dma_IsDataTransferStarted(const DmaContext_t *ctx);
CyBool_t Dma_ShouldFlushSrcEp(const DmaContext_t *ctx);
uint32_t Dma_GetRxCount(const DmaContext_t *ctx);
uint32_t Dma_GetTxCount(const DmaContext_t *ctx);

#endif // _DMA_H_