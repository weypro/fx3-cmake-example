// This file illustrates the bulk source sink application example using the DMA MANUAL_IN
// and DMA MANUAL_OUT mode

// This example illustrates USB endpoint data source and data sink mechanism. The example
// comprises of vendor class USB enumeration descriptors with 2 bulk endpoints. A bulk OUT
// endpoint acts as the producer of data and acts as the sink to the host. A bulk IN endpoint
// acts as the consumer of data and acts as the source to the host.

// The data source and sink is achieved with the help of a DMA MANUAL IN channel and a DMA
// MANUAL OUT channel. A DMA MANUAL IN channel is created between the producer USB bulk
// endpoint and the CPU. A DMA MANUAL OUT channel is created between the CPU and the consumer
// USB bulk endpoint. Data is received in the IN channel DMA buffer from the host through the
// producer endpoint. CPU is signaled of the data reception using DMA callbacks. The CPU
// discards this buffer. This leads to the sink mechanism. A constant pattern data is loaded
// onto the OUT Channel DMA buffer whenever the buffer is available. CPU issues commit of
// the DMA data transfer to the consumer endpoint which then gets transferred to the host.
// This leads to a constant source mechanism.

// The DMA buffer size is defined based on the USB speed. 64 for full speed, 512 for high speed
// and 1024 for super speed. DMA_BUFFER_COUNT in the header file defines the
// number of DMA buffers.

#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyfxbulksrcsink.h"
#include "cyu3usb.h"
#include "cyu3uart.h"
#include "cyu3gpio.h"
#include "cyu3utils.h"

// Include the split module headers
#include "debug.h"
#include "power.h"
#include "dma.h"
#include "usb_ctrl.h"

CyU3PThread     bulkSrcSinkAppThread;    // Application thread structure

CyBool_t glIsApplnActive = CyFalse;      // Whether the source sink application is active or not.

// Buffer used for USB event logs.
uint8_t *gl_UsbLogBuffer = NULL;

// This function starts the application. This is called
// when a SET_CONF event is received from the USB host. The endpoints
// are configured and the DMA pipe is setup in this function.
void
CyFxBulkSrcSinkApplnStart (
        void)
{
    uint16_t size = 0;
    CyU3PEpConfig_t epCfg;
    CyU3PDmaChannelConfig_t dmaCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();

    // First identify the usb speed. Once that is identified,
    // create a DMA channel and start the transfer on this.

    // Based on the Bus Speed configure the endpoint packet size
    switch (usbSpeed)
    {
        case CY_U3P_FULL_SPEED:
            size = USB_PACKET_SIZE_FULL_SPEED;
            break;

        case CY_U3P_HIGH_SPEED:
            size = USB_PACKET_SIZE_HIGH_SPEED;
            break;

        case  CY_U3P_SUPER_SPEED:
            size = USB_PACKET_SIZE_SUPER_SPEED;
            break;

        default:
            CyU3PDebugPrint (4, "Error! Invalid USB speed.\n");
            CyFxAppErrorHandler (CY_U3P_ERROR_FAILURE);
            break;
    }

    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyTrue;
    epCfg.epType = CY_U3P_USB_EP_BULK;
    epCfg.burstLen = (usbSpeed == CY_U3P_SUPER_SPEED) ?
                                                      (DMA_BURST_LENGTH) : 1;
    epCfg.streams = 0;
    epCfg.pcktSize = size;

    // Producer endpoint configuration
    apiRetStatus = CyU3PSetEpConfig(USB_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    // Consumer endpoint configuration
    apiRetStatus = CyU3PSetEpConfig(USB_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    // Flush the endpoint memory
    CyU3PUsbFlushEp(USB_EP_PRODUCER);
    CyU3PUsbFlushEp(USB_EP_CONSUMER);

    // Create a DMA MANUAL_IN channel for the producer socket.
    CyU3PMemSet ((uint8_t *)&dmaCfg, 0, sizeof (dmaCfg));
    // The buffer size will be same as packet size for the
    // full speed, high speed and super speed non-burst modes.
    // For super speed burst mode of operation, the buffers will be
    // 1024 * burst length so that a full burst can be completed.
    // This will mean that a buffer will be available only after it
    // has been filled or when a short packet is received.
    dmaCfg.size  = (size * DMA_BURST_LENGTH);
    // Multiply the buffer size with the multiplier
    // for performance improvement.
    dmaCfg.size *= DMA_SIZE_MULTIPLIER;
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

    apiRetStatus = CyU3PDmaChannelCreate (&glChHandleBulkSink,
                                         CY_U3P_DMA_TYPE_MANUAL_IN, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelCreate failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    // Create a DMA MANUAL_OUT channel for the consumer socket.
    dmaCfg.notification = CY_U3P_DMA_CB_CONS_EVENT;
    dmaCfg.prodSckId = CY_U3P_CPU_SOCKET_PROD;
    dmaCfg.consSckId = USB_EP_CONSUMER_SOCKET;
    apiRetStatus = CyU3PDmaChannelCreate (&glChHandleBulkSrc,
                                         CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelCreate failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    // Set DMA Channel transfer size
    apiRetStatus = CyU3PDmaChannelSetXfer (&glChHandleBulkSink, CY_FX_BULKSRCSINK_DMA_TX_SIZE);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelSetXfer failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PDmaChannelSetXfer (&glChHandleBulkSrc, CY_FX_BULKSRCSINK_DMA_TX_SIZE);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelSetXfer failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    CyU3PUsbRegisterEpEvtCallback (CyFxBulkSrcSinkApplnEpEvtCB, CYU3P_USBEP_SS_RETRY_EVT, 0x00, 0x02);
    CyFxBulkSrcSinkFillInBuffers ();

    // Update the flag so that the application thread is notified of this.
    glIsApplnActive = CyTrue;
}

// This function stops the application. This shall be called whenever a RESET
// or DISCONNECT event is received from the USB host. The endpoints are
// disabled and the DMA pipe is destroyed by this function.
void
CyFxBulkSrcSinkApplnStop (
        void)
{
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    // Update the flag so that the application thread is notified of this.
    glIsApplnActive = CyFalse;

    // Destroy the channels
    CyU3PDmaChannelDestroy (&glChHandleBulkSink);
    CyU3PDmaChannelDestroy (&glChHandleBulkSrc);

    // Flush the endpoint memory
    CyU3PUsbFlushEp(USB_EP_PRODUCER);
    CyU3PUsbFlushEp(USB_EP_CONSUMER);

    // Disable endpoints.
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyFalse;

    // Producer endpoint configuration.
    apiRetStatus = CyU3PSetEpConfig(USB_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    // Consumer endpoint configuration.
    apiRetStatus = CyU3PSetEpConfig(USB_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }
}

// This is the callback function to handle the USB events.
void
CyFxBulkSrcSinkApplnUSBEventCB (
        CyU3PUsbEventType_t evtype, // Event type
        uint16_t            evdata  // Event data
)
{
    CyU3PDebugPrint (2, "USB EVENT: %d %d\r\n", evtype, evdata);

    switch (evtype)
    {
        case CY_U3P_USB_EVENT_CONNECT:
            break;

        case CY_U3P_USB_EVENT_SETCONF:
            // If the application is already active
            // stop it before re-enabling.
            if (glIsApplnActive)
            {
                CyFxBulkSrcSinkApplnStop ();
            }

            // Start the source sink function.
            CyFxBulkSrcSinkApplnStart ();
            break;

        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_DISCONNECT:
            glForceLinkU2 = CyFalse;

            // Stop the source sink function.
            if (glIsApplnActive)
            {
                CyFxBulkSrcSinkApplnStop ();
            }

            glDataTransStarted = CyFalse;
            break;

        case CY_U3P_USB_EVENT_EP0_STAT_CPLT:
            glEp0StatCount++;
            break;

        case CY_U3P_USB_EVENT_VBUS_REMOVED:
            if (StandbyModeEnable)
            {
                TriggerStandbyMode = CyTrue;
                StandbyModeEnable  = CyFalse;
            }
            break;

        default:
            break;
    }
}

// This function initializes the USB Module, sets the enumeration descriptors.
// This function does not start the bulk streaming and this is done only when
// SET_CONF event is received.
void
CyFxBulkSrcSinkApplnInit (void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyBool_t no_renum = CyFalse;

    // The fast enumeration is the easiest way to setup a USB connection,
    // where all enumeration phase is handled by the library. Only the
    // class / vendor requests need to be handled by the application.
    CyU3PUsbRegisterSetupCallback(CyFxBulkSrcSinkApplnUSBSetupCB, CyTrue);

    // Setup the callback to handle the USB events.
    CyU3PUsbRegisterEventCallback(CyFxBulkSrcSinkApplnUSBEventCB);

    // Register a callback to handle LPM requests from the USB 3.0 host.
    CyU3PUsbRegisterLPMRequestCallback(CyFxApplnLPMRqtCB);

    // Start the USB functionality.
    apiRetStatus = CyU3PUsbStart();
    if (apiRetStatus == CY_U3P_ERROR_NO_REENUM_REQUIRED)
        no_renum = CyTrue;
    else if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PUsbStart failed to Start, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    // Change GPIO state again.
    CyU3PGpioSimpleSetValue (FX3_GPIO_TEST_OUT, CyTrue);

    // Set the USB Enumeration descriptors using new descriptor module
    if (usb_descriptors_register_all() != FX3_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB descriptor registration failed\n");
        CyFxAppErrorHandler(CY_U3P_ERROR_FAILURE);
    }

    // Register a buffer into which the USB driver can log relevant events.
    gl_UsbLogBuffer = (uint8_t *)CyU3PDmaBufferAlloc (USB_LOG_BUFFER_SIZE);
    if (gl_UsbLogBuffer)
        CyU3PUsbInitEventLog (gl_UsbLogBuffer, USB_LOG_BUFFER_SIZE);

    CyU3PDebugPrint (4, "About to connect to USB host\r\n");

    // Connect the USB Pins with super speed operation enabled.
    if (!no_renum) {

        apiRetStatus = CyU3PConnectState(CyTrue, CyTrue);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "USB Connect failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }
    }
    else
    {
        // USB connection is already active. Configure the endpoints and DMA channels.
        CyFxBulkSrcSinkApplnStart ();
    }

    CyU3PDebugPrint (8, "CyFxBulkSrcSinkApplnInit complete\r\n");
}

// De-initialize function for the USB block. Used to test USB Stop/Start functionality.
static void
CyFxBulkSrcSinkApplnDeinit (
        void)
{
    if (glIsApplnActive)
        CyFxBulkSrcSinkApplnStop ();

    CyU3PConnectState (CyFalse, CyTrue);
    CyU3PThreadSleep (1000);
    CyU3PUsbStop ();
    CyU3PThreadSleep (1000);
}

// Entry function for the BulkSrcSinkAppThread.
void
BulkSrcSinkAppThread_Entry (
        uint32_t input)
{
    CyU3PReturnStatus_t stat;
    uint32_t eventMask = CYFX_USB_CTRL_TASK | CYFX_USB_HOSTWAKE_TASK;   // Events that we are interested in.
    uint32_t eventStat;                                                 // Current status of the events.
    uint8_t  vendorRqtCnt = 0;

    uint16_t prevUsbLogIndex = 0, tmp1, tmp2;
    CyU3PUsbLinkPowerMode curState;

    // Initialize the debug module
    CyFxBulkSrcSinkApplnDebugInit();
    CyU3PDebugPrint (1, "\n\ndebug initialized\r\n");

    // Initialize the application
    CyFxBulkSrcSinkApplnInit();

    // Create a timer with 100 ms expiry to enable/disable LPM transitions
    CyU3PTimerCreate (&glLpmTimer, TimerCb, 0, LPM_TIMER_TIMEOUT, LPM_TIMER_TIMEOUT, CYU3P_NO_ACTIVATE);

    for (;;)
    {
        // The following call will block until at least one of the events enabled in eventMask is received.
        // The eventStat variable will hold the events that were active at the time of returning from this API.
        // The CLEAR flag means that all events will be atomically cleared before this function returns.

        // We cause this event wait to time out every 10 milli-seconds, so that we can periodically get the FX3
        // device out of low power modes.
        stat = CyU3PEventGet (&glBulkLpEvent, eventMask, CYU3P_EVENT_OR_CLEAR, &eventStat, APP_EVENT_WAIT_TIMEOUT);
        if (stat == CY_U3P_SUCCESS)
        {
            // If the HOSTWAKE task is set, send a DEV_NOTIFICATION (FUNCTION_WAKE) or remote wakeup signalling
            // based on the USB connection speed.
            if (eventStat & CYFX_USB_HOSTWAKE_TASK)
            {
                CyU3PThreadSleep (1000);
                if (CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED)
                    stat = CyU3PUsbSendDevNotification (1, 0, 0);
                else
                    stat = CyU3PUsbDoRemoteWakeup ();

                if (stat != CY_U3P_SUCCESS)
                    CyU3PDebugPrint (2, "Remote wake attempt failed with code: %d\r\n", stat);
            }

            // If there is a pending control request, handle it here.
            if (eventStat & CYFX_USB_CTRL_TASK)
            {
                uint8_t  bRequest, bReqType;
                uint16_t wLength, temp;
                uint16_t wValue, wIndex;

                // Decode the fields from the setup request.
                bReqType = (gl_setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
                bRequest = ((gl_setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
                wLength  = ((gl_setupdat1 & CY_U3P_USB_LENGTH_MASK)  >> CY_U3P_USB_LENGTH_POS);
                wValue   = ((gl_setupdat0 & CY_U3P_USB_VALUE_MASK) >> CY_U3P_USB_VALUE_POS);
                wIndex   = ((gl_setupdat1 & CY_U3P_USB_INDEX_MASK) >> CY_U3P_USB_INDEX_POS);

                if ((bReqType & CY_U3P_USB_TYPE_MASK) == CY_U3P_USB_VENDOR_RQT)
                {
                    switch (bRequest)
                    {
                        case 0x76:
                            glEp0Buffer[0] = vendorRqtCnt;
                            glEp0Buffer[1] = ~vendorRqtCnt;
                            glEp0Buffer[2] = 1;
                            glEp0Buffer[3] = 5;
                            CyU3PUsbSendEP0Data (wLength, glEp0Buffer);
                            vendorRqtCnt++;
                            break;

                        case 0x77:      // Trigger remote wakeup.
                            CyU3PUsbAckSetup ();
                            CyU3PEventSet (&glBulkLpEvent, CYFX_USB_HOSTWAKE_TASK, CYU3P_EVENT_OR);
                            break;

                        case 0x78:      // Get count of EP0 status events received.
                            CyU3PMemCopy ((uint8_t *)glEp0Buffer, ((uint8_t *)&glEp0StatCount), 4);
                            CyU3PUsbSendEP0Data (4, glEp0Buffer);
                            break;

                        case 0x79:      // Request with no data phase. Insert a delay and then ACK the request.
                            CyU3PThreadSleep (5);
                            CyU3PUsbAckSetup ();
                            break;

                        case 0x80:      // Request with OUT data phase. Just get the data and ignore it for now.
                            CyU3PUsbGetEP0Data (sizeof (glEp0Buffer), (uint8_t *)glEp0Buffer, &wLength);
                            break;

                        case 0x81:
                            // Get the current event log index and send it to the host.
                            if (wLength == 2)
                            {
                                temp = CyU3PUsbGetEventLogIndex ();
                                CyU3PMemCopy ((uint8_t *)glEp0Buffer, (uint8_t *)&temp, 2);
                                CyU3PUsbSendEP0Data (2, glEp0Buffer);
                            }
                            else
                                CyU3PUsbStall (0, CyTrue, CyFalse);
                            break;

                        case 0x82:
                            // Send the USB event log buffer content to the host.
                            if (wLength != 0)
                            {
                                if (wLength < USB_LOG_BUFFER_SIZE)
                                    CyU3PUsbSendEP0Data (wLength, gl_UsbLogBuffer);
                                else
                                    CyU3PUsbSendEP0Data (USB_LOG_BUFFER_SIZE, gl_UsbLogBuffer);
                            }
                            else
                                CyU3PUsbAckSetup ();
                            break;

                        case 0x83:
                        {
                            uint32_t addr = ((uint32_t)wValue << 16) | (uint32_t)wIndex;
                            CyU3PReadDeviceRegisters ((uvint32_t *)addr, 1, (uint32_t *)glEp0Buffer);
                            CyU3PUsbSendEP0Data (4, glEp0Buffer);
                        }
                        break;

                        case 0x84:
                        {
                            uint8_t major, minor, patch;

                            if (CyU3PUsbGetBooterVersion (&major, &minor, &patch) == CY_U3P_SUCCESS)
                            {
                                glEp0Buffer[0] = major;
                                glEp0Buffer[1] = minor;
                                glEp0Buffer[2] = patch;
                                CyU3PUsbSendEP0Data (3, glEp0Buffer);
                            }
                            else
                                CyU3PUsbStall (0, CyTrue, CyFalse);
                        }
                        break;

                        case 0x90:
                            // Request to switch control back to the boot firmware.

                            // Complete the control request.
                            CyU3PUsbAckSetup ();
                            CyU3PThreadSleep (10);

                            // Get rid of the DMA channels and EP configuration.
                            CyFxBulkSrcSinkApplnStop ();

                            // De-initialize the Debug and UART modules.
                            CyU3PDebugDeInit ();
                            CyU3PUartDeInit ();

                            // Now jump back to the boot firmware image.
                            CyU3PUsbSetBooterSwitch (CyTrue);
                            CyU3PUsbJumpBackToBooter (0x40078000);
                            while (1)
                                CyU3PThreadSleep (100);
                            break;

                        case 0xB1:
                            // Switch to a USB 2.0 Connection.
                            CyU3PUsbAckSetup ();
                            CyU3PThreadSleep (1000);
                            CyFxBulkSrcSinkApplnStop ();
                            CyU3PConnectState (CyFalse, CyTrue);
                            CyU3PThreadSleep (100);
                            CyU3PConnectState (CyTrue, CyFalse);
                            break;

                        case 0xB2:
                            // Switch to a USB 3.0 connection.
                            CyU3PUsbAckSetup ();
                            CyU3PThreadSleep (100);
                            CyFxBulkSrcSinkApplnStop ();
                            CyU3PConnectState (CyFalse, CyTrue);
                            CyU3PThreadSleep (10);
                            CyU3PConnectState (CyTrue, CyTrue);
                            break;

                        case 0xB3:
                            // Stop and restart the USB block.
                            CyU3PUsbAckSetup ();
                            CyU3PThreadSleep (100);
                            CyFxBulkSrcSinkApplnDeinit ();
                            CyFxBulkSrcSinkApplnInit ();
                            break;

                        case 0xE0:
                            // Request to reset the FX3 device.
                            CyU3PUsbAckSetup ();
                            CyU3PThreadSleep (2000);
                            CyU3PConnectState (CyFalse, CyTrue);
                            CyU3PThreadSleep (1000);
                            CyU3PDeviceReset (CyFalse);
                            CyU3PThreadSleep (1000);
                            break;

                        case 0xE1:
                            // Request to place FX3 in standby when VBus is next disconnected.
                            StandbyModeEnable = CyTrue;
                            CyU3PUsbAckSetup ();
                            break;

                        default:        // Unknown request. Stall EP0.
                            CyU3PUsbStall (0, CyTrue, CyFalse);
                            break;
                    }
                }
                else
                {
                    // Only vendor requests are to be handled here.
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                }
            }
        }

        if (glSrcEpFlush)
        {
            // Stall the endpoint, so that the host can reset the pipe and continue.
            glSrcEpFlush = CyFalse;
            CyU3PUsbStall (USB_EP_CONSUMER, CyTrue, CyFalse);
        }

        // Force the USB 3.0 link to U2.
        if (glForceLinkU2)
        {
            stat = CyU3PUsbGetLinkPowerState (&curState);
            while ((glForceLinkU2) && (stat == CY_U3P_SUCCESS) && (curState == CyU3PUsbLPM_U0))
            {
                // Repeatedly try to go into U2 state.
                CyU3PUsbSetLinkPowerState (CyU3PUsbLPM_U2);
                CyU3PThreadSleep (5);
                stat = CyU3PUsbGetLinkPowerState (&curState);
            }
        }

        if (TriggerStandbyMode)
        {
            TriggerStandbyMode = CyFalse;

            CyU3PConnectState (CyFalse, CyTrue);
            CyU3PUsbStop ();
            CyU3PDebugDeInit ();
            CyU3PUartDeInit ();

            // Add a delay to allow VBus to settle.
            CyU3PThreadSleep (STANDBY_VBUS_SETTLE_TIME);

            // VBus has been turned off. Go into standby mode and wait for VBus to be turned on again.
            // The I-TCM content and GPIO register state will be backed up in the memory area starting
            // at address 0x40060000.
            stat = CyU3PSysEnterStandbyMode (CY_U3P_SYS_USB_VBUS_WAKEUP_SRC, CY_U3P_SYS_USB_VBUS_WAKEUP_SRC,
                                            (uint8_t *)0x40060000);
            if (stat != CY_U3P_SUCCESS)
            {
                CyFxBulkSrcSinkApplnDebugInit ();
                CyU3PDebugPrint (4, "Enter standby returned %d\r\n", stat);
                CyFxAppErrorHandler (stat);
            }

            // If the entry into standby succeeds, the CyU3PSysEnterStandbyMode function never returns. The
            // firmware application starts running again from the main entry point. Therefore, this code
            // will never be executed.
            CyFxAppErrorHandler (1);
        }
        else
        {
            // Compare the current USB driver log index against the previous value.
            tmp1 = CyU3PUsbGetEventLogIndex ();
            if (tmp1 != prevUsbLogIndex)
            {
                tmp2 = prevUsbLogIndex;
                while (tmp2 != tmp1)
                {
                    CyU3PDebugPrint (4, "USB LOG: %x\r\n", gl_UsbLogBuffer[tmp2]);
                    tmp2++;
                    if (tmp2 == USB_LOG_BUFFER_SIZE)
                        tmp2 = 0;
                }
            }

            // Store the current log index.
            prevUsbLogIndex = tmp1;
        }
    }
}

// Application define function which creates the threads.
void
CyFxApplicationDefine (
        void)
{
    void *ptr = NULL;
    uint32_t ret = CY_U3P_SUCCESS;

    // Create an event flag group that will be used for signalling the application thread.
    ret = CyU3PEventCreate (&glBulkLpEvent);
    if (ret != 0)
    {
        // Loop indefinitely
        while (1);
    }

    // Allocate the memory for the threads
    ptr = CyU3PMemAlloc (BULK_APP_THREAD_STACK);

    // Create the thread for the application
    ret = CyU3PThreadCreate (&bulkSrcSinkAppThread,                // App thread structure
                            BULK_APP_THREAD_NAME,                    // Thread ID and thread name
                            BulkSrcSinkAppThread_Entry,              // App thread entry function
                            0,                                       // No input parameter to thread
                            ptr,                                     // Pointer to the allocated thread stack
                            BULK_APP_THREAD_STACK,          // App thread stack size
                            BULK_APP_THREAD_PRIORITY,       // App thread priority
                            BULK_APP_THREAD_PRIORITY,       // App thread priority
                            CYU3P_NO_TIME_SLICE,                     // No time slice for the application thread
                            CYU3P_AUTO_START                         // Start the thread immediately
    );

    // Check the return code
    if (ret != 0)
    {
        // Thread Creation failed with the error code retThrdCreate

        // Add custom recovery or debug actions here

        // Application cannot continue
        // Loop indefinitely
        while(1);
    }
}

// Main function
int
main (void)
{
    CyU3PIoMatrixConfig_t io_cfg = IO_MATRIX_CONFIG_DEFAULT;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    // Initialize the device
    CyU3PSysClockConfig_t clockConfig = SYS_CLOCK_CONFIG_DEFAULT;
    status = CyU3PDeviceInit (&clockConfig);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    // Initialize the caches. The D-Cache is kept disabled. Enabling this will cause performance to drop,
    // as the driver will start doing a lot of un-necessary cache clean/flush operations.
    // Enable the D-Cache only if there is a need to process the data being transferred by firmware code.
    status = CyU3PDeviceCacheControl (CACHE_CONFIG_I_ENABLE, CACHE_CONFIG_D_ENABLE, CACHE_CONFIG_DMA_ENABLE);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    // Configure the IO matrix for the device. On the FX3 DVK board, the COM port
    // is connected to the IO(53:56). This means that either DQ32 mode should be
    // selected or lppMode should be set to UART_ONLY. Here we are choosing
    // UART_ONLY configuration.
    status = CyU3PDeviceConfigureIOMatrix (&io_cfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    // This is a non returnable call for initializing the RTOS kernel
    CyU3PKernelEntry ();

    // Dummy return to make the compiler happy
    return 0;

handle_fatal_error:

    // Cannot recover from this error.
    while (1);
}