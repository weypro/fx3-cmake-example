#ifndef _DMA_H_
#define _DMA_H_

#include "cyu3types.h"
#include "cyu3dma.h"

// Global variables
extern CyU3PDmaChannel glChHandleBulkSink;      // DMA MANUAL_IN channel handle.
extern CyU3PDmaChannel glChHandleBulkSrc;       // DMA MANUAL_OUT channel handle.
extern uint32_t glDMARxCount;                   // Counter to track the number of buffers received.
extern uint32_t glDMATxCount;                   // Counter to track the number of buffers transmitted.
extern CyBool_t glDataTransStarted;             // Whether DMA transfer has been started after enumeration.
extern volatile CyBool_t glSrcEpFlush;

// Function declarations
void CyFxBulkSrcSinkDmaCallback(CyU3PDmaChannel *chHandle, CyU3PDmaCbType_t type, CyU3PDmaCBInput_t *input);
void CyFxBulkSrcSinkFillInBuffers(void);

#endif // _DMA_H_