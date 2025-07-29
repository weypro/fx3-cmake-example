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

CyU3PDmaChannel glChHandleBulkSink;      // DMA MANUAL_IN channel handle.
CyU3PDmaChannel glChHandleBulkSrc;       // DMA MANUAL_OUT channel handle.

uint32_t glDMARxCount = 0;               // Counter to track the number of buffers received.
uint32_t glDMATxCount = 0;               // Counter to track the number of buffers transmitted.
CyBool_t glDataTransStarted = CyFalse;   // Whether DMA transfer has been started after enumeration.

volatile CyBool_t glSrcEpFlush = CyFalse;

// Callback funtion for the DMA event notification.
void
CyFxBulkSrcSinkDmaCallback (
        CyU3PDmaChannel   *chHandle, // Handle to the DMA channel.
        CyU3PDmaCbType_t  type,      // Callback type.
        CyU3PDmaCBInput_t *input)    // Callback status.
{
    CyU3PDmaBuffer_t buf_p;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    glDataTransStarted = CyTrue;

    // Start/restart the timer and disable LPM
    CyU3PUsbLPMDisable();
    CyU3PTimerStop (&glLpmTimer);
    CyU3PTimerModify(&glLpmTimer, LPM_TIMER_TIMEOUT, 0);
    CyU3PTimerStart(&glLpmTimer);

    if (type == CY_U3P_DMA_CB_PROD_EVENT)
    {
        // This is a produce event notification to the CPU. This notification is
        // received upon reception of every buffer. We have to discard the buffer
        // as soon as it is received to implement the data sink.
        status = CyU3PDmaChannelDiscardBuffer (chHandle);
        if (status != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelDiscardBuffer failed, Error code = %d\n", status);
        }

        // Increment the counter.
        glDMARxCount++;
    }
    if (type == CY_U3P_DMA_CB_CONS_EVENT)
    {
        // This is a consume event notification to the CPU. This notification is
        // received when a buffer is sent out from the device. We have to commit
        // a new buffer as soon as a buffer is available to implement the data
        // source. The data is preloaded into the buffer at that start. So just
        // commit the buffer.
        status = CyU3PDmaChannelGetBuffer (chHandle, &buf_p, CYU3P_NO_WAIT);
        if (status == CY_U3P_SUCCESS)
        {
            // Commit the full buffer with default status.
            status = CyU3PDmaChannelCommitBuffer (chHandle, buf_p.size, 0);
            if (status != CY_U3P_SUCCESS)
            {
                CyU3PDebugPrint (4, "CyU3PDmaChannelCommitBuffer failed, Error code = %d\n", status);
            }
        }
        else
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelGetBuffer failed, Error code = %d\n", status);
        }

        // Increment the counter.
        glDMATxCount++;
    }
}

// Fill all DMA buffers on the IN endpoint with data. This gets data moving after an endpoint reset.
void
CyFxBulkSrcSinkFillInBuffers (
        void)
{
    CyU3PReturnStatus_t stat;
    CyU3PDmaBuffer_t    buf_p;
    uint16_t            index = 0;

    // Now preload all buffers in the MANUAL_OUT pipe with the required data.
    for (index = 0; index < DMA_BUFFER_COUNT; index++)
    {
        stat = CyU3PDmaChannelGetBuffer (&glChHandleBulkSrc, &buf_p, CYU3P_NO_WAIT);
        if (stat != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelGetBuffer failed, Error code = %d\n", stat);
            CyFxAppErrorHandler(stat);
        }

        CyU3PMemSet (buf_p.buffer, BULK_DATA_PATTERN, buf_p.size);
        stat = CyU3PDmaChannelCommitBuffer (&glChHandleBulkSrc, buf_p.size, 0);
        if (stat != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelCommitBuffer failed, Error code = %d\n", stat);
            CyFxAppErrorHandler(stat);
        }
    }
}