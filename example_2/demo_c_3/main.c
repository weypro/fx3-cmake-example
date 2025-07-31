#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyfxbulksrcsink.h"
#include "cyu3usb.h"
#include "cyu3uart.h"
#include "cyu3gpio.h"
#include "cyu3utils.h"

#include "debug.h"
#include "power.h"
#include "dma.h"
#include "usb_ctrl.h"

#include "app.h"

// Global application context instance
AppContext_t g_appContext;

// Forward declarations
void BulkSrcSinkAppThread_Entry(uint32_t input);
void CyFxBulkSrcSinkApplnStart(AppContext_t *appCtx);
void CyFxBulkSrcSinkApplnStop(AppContext_t *appCtx);
void CyFxBulkSrcSinkApplnUSBEventCB(CyU3PUsbEventType_t evtype, uint16_t evdata);
void CyFxBulkSrcSinkApplnInit(AppContext_t *appCtx);

// DMA event callback function to handle power management
static void OnDmaEvent(dma_evt_t evt, void *user)
{
    AppContext_t *appCtx = (AppContext_t *)user;

    if (!appCtx) {
        return;
    }

    // Handle power management tasks that were previously in DMA callback
    if (evt == DMA_EVT_PROD || evt == DMA_EVT_CONS) {
        // Start/restart the timer and disable LPM
        CyU3PUsbLPMDisable();
        CyU3PTimerStop(&appCtx->power.lpmTimer);
        CyU3PTimerModify(&appCtx->power.lpmTimer, LPM_TIMER_TIMEOUT, 0);
        CyU3PTimerStart(&appCtx->power.lpmTimer);
    }
}

// Function to start the application after USB configuration
void CyFxBulkSrcSinkApplnStart(AppContext_t *appCtx)
{
    if (!appCtx) return;

    uint16_t size = 0;
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();

    // Determine packet size based on USB speed
    size = usb_get_packet_size(usb_speed_from_cyu3p(usbSpeed));

    // Configure endpoints
    CyU3PMemSet((uint8_t *)&epCfg, 0, sizeof(epCfg));
    epCfg.enable = CyTrue;
    epCfg.epType = CY_U3P_USB_EP_BULK;
    epCfg.burstLen = (usbSpeed == CY_U3P_SUPER_SPEED) ? DMA_BURST_LENGTH : 1;
    epCfg.streams = 0;
    epCfg.pcktSize = size;

    // Producer endpoint configuration
    apiRetStatus = CyU3PSetEpConfig(USB_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        CyU3PDebugPrint(4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    // Consumer endpoint configuration
    apiRetStatus = CyU3PSetEpConfig(USB_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        CyU3PDebugPrint(4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    // Flush the endpoint memory
    CyU3PUsbFlushEp(USB_EP_PRODUCER);
    CyU3PUsbFlushEp(USB_EP_CONSUMER);

    // Setup DMA channels using the new modular API
    apiRetStatus = Dma_SetupChannels(&appCtx->dma, size, usbSpeed);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        CyU3PDebugPrint(4, "Dma_SetupChannels failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    // Register DMA event callback for power management
    Dma_RegisterListener(&appCtx->dma, OnDmaEvent, appCtx);

    // Start DMA transfers
    apiRetStatus = Dma_StartTransfer(&appCtx->dma);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        CyU3PDebugPrint(4, "Dma_StartTransfer failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    // Register endpoint event callback
    CyU3PUsbRegisterEpEvtCallback(CyFxBulkSrcSinkApplnEpEvtCB, CYU3P_USBEP_SS_RETRY_EVT, 0x00, 0x02);

    // Fill IN buffers with data
    Dma_FillInBuffers(&appCtx->dma);

    // Update the application active flag
    appCtx->isApplnActive = CyTrue;
}

// Function to stop the application
void CyFxBulkSrcSinkApplnStop(AppContext_t *appCtx)
{
    if (!appCtx) return;

    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    // Update the application active flag
    appCtx->isApplnActive = CyFalse;

    // Teardown DMA channels using the new modular API
    Dma_TeardownChannels(&appCtx->dma);

    // Flush the endpoint memory
    CyU3PUsbFlushEp(USB_EP_PRODUCER);
    CyU3PUsbFlushEp(USB_EP_CONSUMER);

    // Disable endpoints
    CyU3PMemSet((uint8_t *)&epCfg, 0, sizeof(epCfg));
    epCfg.enable = CyFalse;

    // Producer endpoint configuration
    apiRetStatus = CyU3PSetEpConfig(USB_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        CyU3PDebugPrint(4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    // Consumer endpoint configuration
    apiRetStatus = CyU3PSetEpConfig(USB_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS) {
        CyU3PDebugPrint(4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }
}

// USB event callback function
void CyFxBulkSrcSinkApplnUSBEventCB(CyU3PUsbEventType_t evtype, uint16_t evdata)
{
    CyU3PDebugPrint(2, "USB EVENT: %d %d\r\n", evtype, evdata);

    switch (evtype) {
        case CY_U3P_USB_EVENT_CONNECT:
            break;

        case CY_U3P_USB_EVENT_SETCONF:
            // If the application is already active, stop it before re-enabling
            if (g_appContext.isApplnActive) {
                CyFxBulkSrcSinkApplnStop(&g_appContext);
            }
            // Start the source sink function
            CyFxBulkSrcSinkApplnStart(&g_appContext);
            break;

        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_DISCONNECT:
            Power_SetForceLinkU2(&g_appContext.power, CyFalse);

            // Stop the source sink function
            if (g_appContext.isApplnActive) {
                CyFxBulkSrcSinkApplnStop(&g_appContext);
            }

            Dma_SetDataTransferStarted(&g_appContext.dma, CyFalse);
            break;

        case CY_U3P_USB_EVENT_EP0_STAT_CPLT:
            // Increment EP0 status count using USB Control module API
            UsbCtrl_IncrementEp0StatCount(&g_appContext.usbCtrl);
            break;

        case CY_U3P_USB_EVENT_VBUS_REMOVED:
            if (Power_IsStandbyModeEnabled(&g_appContext.power)) {
                Power_TriggerStandby(&g_appContext.power);
                Power_EnableStandbyMode(&g_appContext.power);  // Reset the enable flag
            }
            break;

        default:
            break;
    }
}

// Initialize the USB module and descriptors
void CyFxBulkSrcSinkApplnInit(AppContext_t *appCtx)
{
    if (!appCtx) return;

    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyBool_t no_renum = CyFalse;

    // Register USB callbacks using USB Control module
    if (UsbCtrl_RegisterCallbacks() != CY_U3P_SUCCESS) {
        CyU3PDebugPrint(4, "USB Control callback registration failed\n");
        CyFxAppErrorHandler(CY_U3P_ERROR_FAILURE);
    }

    // Setup the callback to handle USB events
    CyU3PUsbRegisterEventCallback(CyFxBulkSrcSinkApplnUSBEventCB);

    // Register LPM request callback using power module
    CyU3PUsbRegisterLPMRequestCallback(CyFxApplnLPMRqtCB);

    // Start the USB functionality
    apiRetStatus = CyU3PUsbStart();
    if (apiRetStatus == CY_U3P_ERROR_NO_REENUM_REQUIRED)
        no_renum = CyTrue;
    else if (apiRetStatus != CY_U3P_SUCCESS) {
        CyU3PDebugPrint(4, "CyU3PUsbStart failed to Start, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    // Change GPIO state
    CyU3PGpioSimpleSetValue(FX3_GPIO_TEST_OUT, CyTrue);

    // Set USB enumeration descriptors
    if (usb_descriptors_register_all() != FX3_SUCCESS) {
        CyU3PDebugPrint(4, "USB descriptor registration failed\n");
        CyFxAppErrorHandler(CY_U3P_ERROR_FAILURE);
    }

    // Allocate and register USB log buffer
    appCtx->usbLogBuffer = (uint8_t *)CyU3PDmaBufferAlloc(USB_LOG_BUFFER_SIZE);
    if (appCtx->usbLogBuffer)
        CyU3PUsbInitEventLog(appCtx->usbLogBuffer, USB_LOG_BUFFER_SIZE);

    CyU3PDebugPrint(4, "About to connect to USB host\r\n");

    // Connect the USB pins with super speed operation enabled
    if (!no_renum) {
        apiRetStatus = CyU3PConnectState(CyTrue, CyTrue);
        if (apiRetStatus != CY_U3P_SUCCESS) {
            CyU3PDebugPrint(4, "USB Connect failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }
    } else {
        // USB connection is already active. Configure the endpoints and DMA channels
        CyFxBulkSrcSinkApplnStart(appCtx);
    }

    CyU3PDebugPrint(8, "CyFxBulkSrcSinkApplnInit complete\r\n");
}

// De-initialize function for the USB block
static void CyFxBulkSrcSinkApplnDeinit(AppContext_t *appCtx)
{
    if (!appCtx) return;

    if (appCtx->isApplnActive)
        CyFxBulkSrcSinkApplnStop(appCtx);

    CyU3PConnectState(CyFalse, CyTrue);
    CyU3PThreadSleep(1000);
    CyU3PUsbStop();
    CyU3PThreadSleep(1000);
}

// Entry function for the BulkSrcSinkAppThread
void BulkSrcSinkAppThread_Entry(uint32_t input)
{
    CyU3PReturnStatus_t stat;
    uint32_t eventMask = CYFX_USB_CTRL_TASK | CYFX_USB_HOSTWAKE_TASK;
    uint32_t eventStat;
    uint8_t vendorRqtCnt = 0;

    uint16_t prevUsbLogIndex = 0, tmp1, tmp2;
    CyU3PUsbLinkPowerMode curState;

    // Initialize the debug module
    CyFxBulkSrcSinkApplnDebugInit();
    CyU3PDebugPrint(1, "\n\ndebug initialized\r\n");

    // Initialize module contexts
    if (Power_Init(&g_appContext.power) != CY_U3P_SUCCESS) {
        CyU3PDebugPrint(4, "Power module initialization failed\n");
        CyFxAppErrorHandler(CY_U3P_ERROR_FAILURE);
    }

    if (Dma_Init(&g_appContext.dma) != CY_U3P_SUCCESS) {
        CyU3PDebugPrint(4, "DMA module initialization failed\n");
        CyFxAppErrorHandler(CY_U3P_ERROR_FAILURE);
    }

    if (UsbCtrl_Init(&g_appContext.usbCtrl) != CY_U3P_SUCCESS) {
        CyU3PDebugPrint(4, "USB Control module initialization failed\n");
        CyFxAppErrorHandler(CY_U3P_ERROR_FAILURE);
    }

    // Set up cross-module references
    Power_SetCallbackContext(&g_appContext.power);
    UsbCtrl_SetAppContext(&g_appContext.usbCtrl, &g_appContext);
    UsbCtrl_SetCallbackContext(&g_appContext);

    // Initialize the application
    CyFxBulkSrcSinkApplnInit(&g_appContext);

    for (;;) {
        // Wait for events with timeout
        stat = CyU3PEventGet(&g_appContext.bulkLpEvent, eventMask, CYU3P_EVENT_OR_CLEAR,
                             &eventStat, APP_EVENT_WAIT_TIMEOUT);
        if (stat == CY_U3P_SUCCESS) {
            // Handle remote wakeup task
            if (eventStat & CYFX_USB_HOSTWAKE_TASK) {
                CyU3PThreadSleep(1000);
                if (CyU3PUsbGetSpeed() == CY_U3P_SUPER_SPEED)
                    stat = CyU3PUsbSendDevNotification(1, 0, 0);
                else
                    stat = CyU3PUsbDoRemoteWakeup();

                if (stat != CY_U3P_SUCCESS)
                    CyU3PDebugPrint(2, "Remote wake attempt failed with code: %d\r\n", stat);
            }

            // Handle control requests
            if (eventStat & CYFX_USB_CTRL_TASK) {
                uint8_t bRequest, bReqType;
                uint16_t wLength, temp;
                uint16_t wValue, wIndex;

                // Get setup data using USB Control module API
                uint32_t setupdat0 = UsbCtrl_GetSetupData0(&g_appContext.usbCtrl);
                uint32_t setupdat1 = UsbCtrl_GetSetupData1(&g_appContext.usbCtrl);
                uint8_t *ep0Buffer = UsbCtrl_GetEp0Buffer(&g_appContext.usbCtrl);

                // Decode the fields from the setup request
                bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
                bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
                wLength = ((setupdat1 & CY_U3P_USB_LENGTH_MASK) >> CY_U3P_USB_LENGTH_POS);
                wValue = ((setupdat0 & CY_U3P_USB_VALUE_MASK) >> CY_U3P_USB_VALUE_POS);
                wIndex = ((setupdat1 & CY_U3P_USB_INDEX_MASK) >> CY_U3P_USB_INDEX_POS);

                if ((bReqType & CY_U3P_USB_TYPE_MASK) == CY_U3P_USB_VENDOR_RQT) {
                    switch (bRequest) {
                        case 0x76:
                            ep0Buffer[0] = vendorRqtCnt;
                            ep0Buffer[1] = ~vendorRqtCnt;
                            ep0Buffer[2] = 1;
                            ep0Buffer[3] = 5;
                            CyU3PUsbSendEP0Data(wLength, ep0Buffer);
                            vendorRqtCnt++;
                            break;

                        case 0x77:  // Trigger remote wakeup
                            CyU3PUsbAckSetup();
                            CyU3PEventSet(&g_appContext.bulkLpEvent, CYFX_USB_HOSTWAKE_TASK, CYU3P_EVENT_OR);
                            break;

                        case 0x78:  // Get count of EP0 status events received
                        {
                            uint32_t ep0StatCount = UsbCtrl_GetEp0StatCount(&g_appContext.usbCtrl);
                            CyU3PMemCopy((uint8_t *)ep0Buffer, ((uint8_t *)&ep0StatCount), 4);
                            CyU3PUsbSendEP0Data(4, ep0Buffer);
                        }
                        break;

                        case 0x79:  // Request with no data phase
                            CyU3PThreadSleep(5);
                            CyU3PUsbAckSetup();
                            break;

                        case 0x80:  // Request with OUT data phase
                            CyU3PUsbGetEP0Data(VENDOR_CMD_BUFFER_ALIGN, ep0Buffer, &wLength);
                            break;

                        case 0x81:
                            // Get the current event log index and send it to the host
                            if (wLength == 2) {
                                temp = CyU3PUsbGetEventLogIndex();
                                CyU3PMemCopy((uint8_t *)ep0Buffer, (uint8_t *)&temp, 2);
                                CyU3PUsbSendEP0Data(2, ep0Buffer);
                            } else
                                CyU3PUsbStall(0, CyTrue, CyFalse);
                            break;

                        case 0x82:
                            // Send the USB event log buffer content to the host
                            if (wLength != 0) {
                                if (wLength < USB_LOG_BUFFER_SIZE)
                                    CyU3PUsbSendEP0Data(wLength, g_appContext.usbLogBuffer);
                                else
                                    CyU3PUsbSendEP0Data(USB_LOG_BUFFER_SIZE, g_appContext.usbLogBuffer);
                            } else
                                CyU3PUsbAckSetup();
                            break;

                        case 0x83: {
                            uint32_t addr = ((uint32_t)wValue << 16) | (uint32_t)wIndex;
                            CyU3PReadDeviceRegisters((uvint32_t *)addr, 1, (uint32_t *)ep0Buffer);
                            CyU3PUsbSendEP0Data(4, ep0Buffer);
                        }
                        break;

                        case 0x84: {
                            uint8_t major, minor, patch;
                            if (CyU3PUsbGetBooterVersion(&major, &minor, &patch) == CY_U3P_SUCCESS) {
                                ep0Buffer[0] = major;
                                ep0Buffer[1] = minor;
                                ep0Buffer[2] = patch;
                                CyU3PUsbSendEP0Data(3, ep0Buffer);
                            } else
                                CyU3PUsbStall(0, CyTrue, CyFalse);
                        }
                        break;

                        case 0x90:
                            // Request to switch control back to the boot firmware
                            CyU3PUsbAckSetup();
                            CyU3PThreadSleep(10);
                            CyFxBulkSrcSinkApplnStop(&g_appContext);
                            CyU3PDebugDeInit();
                            CyU3PUartDeInit();
                            CyU3PUsbSetBooterSwitch(CyTrue);
                            CyU3PUsbJumpBackToBooter(0x40078000);
                            while (1)
                                CyU3PThreadSleep(100);
                            break;

                        case 0xB1:
                            // Switch to a USB 2.0 Connection
                            CyU3PUsbAckSetup();
                            CyU3PThreadSleep(1000);
                            CyFxBulkSrcSinkApplnStop(&g_appContext);
                            CyU3PConnectState(CyFalse, CyTrue);
                            CyU3PThreadSleep(100);
                            CyU3PConnectState(CyTrue, CyFalse);
                            break;

                        case 0xB2:
                            // Switch to a USB 3.0 connection
                            CyU3PUsbAckSetup();
                            CyU3PThreadSleep(100);
                            CyFxBulkSrcSinkApplnStop(&g_appContext);
                            CyU3PConnectState(CyFalse, CyTrue);
                            CyU3PThreadSleep(10);
                            CyU3PConnectState(CyTrue, CyTrue);
                            break;

                        case 0xB3:
                            // Stop and restart the USB block
                            CyU3PUsbAckSetup();
                            CyU3PThreadSleep(100);
                            CyFxBulkSrcSinkApplnDeinit(&g_appContext);
                            CyFxBulkSrcSinkApplnInit(&g_appContext);
                            break;

                        case 0xE0:
                            // Request to reset the FX3 device
                            CyU3PUsbAckSetup();
                            CyU3PThreadSleep(2000);
                            CyU3PConnectState(CyFalse, CyTrue);
                            CyU3PThreadSleep(1000);
                            CyU3PDeviceReset(CyFalse);
                            CyU3PThreadSleep(1000);
                            break;

                        case 0xE1:
                            // Request to place FX3 in standby when VBus is next disconnected
                            Power_EnableStandbyMode(&g_appContext.power);
                            CyU3PUsbAckSetup();
                            break;

                        default:
                            CyU3PUsbStall(0, CyTrue, CyFalse);
                            break;
                    }
                } else {
                    CyU3PUsbStall(0, CyTrue, CyFalse);
                }
            }
        }

        // Handle source endpoint flush using DMA module API
        if (Dma_ShouldFlushSrcEp(&g_appContext.dma)) {
            Dma_ClearSrcEpFlush(&g_appContext.dma);
            CyU3PUsbStall(USB_EP_CONSUMER, CyTrue, CyFalse);
        }

        // Force the USB 3.0 link to U2 using power module API
        if (Power_ShouldForceLinkU2(&g_appContext.power)) {
            stat = CyU3PUsbGetLinkPowerState(&curState);
            while ((Power_ShouldForceLinkU2(&g_appContext.power)) && 
                   (stat == CY_U3P_SUCCESS) && (curState == CyU3PUsbLPM_U0)) {
                CyU3PUsbSetLinkPowerState(CyU3PUsbLPM_U2);
                CyU3PThreadSleep(5);
                stat = CyU3PUsbGetLinkPowerState(&curState);
            }
        }

        // Handle standby mode trigger using power module API
        if (Power_ShouldTriggerStandby(&g_appContext.power)) {
            Power_ClearTriggerStandby(&g_appContext.power);

            CyU3PConnectState(CyFalse, CyTrue);
            CyU3PUsbStop();
            CyU3PDebugDeInit();
            CyU3PUartDeInit();

            // Add a delay to allow VBus to settle
            CyU3PThreadSleep(STANDBY_VBUS_SETTLE_TIME);

            // Enter standby mode
            stat = CyU3PSysEnterStandbyMode(CY_U3P_SYS_USB_VBUS_WAKEUP_SRC, 
                                            CY_U3P_SYS_USB_VBUS_WAKEUP_SRC,
                                            (uint8_t *)0x40060000);
            if (stat != CY_U3P_SUCCESS) {
                CyFxBulkSrcSinkApplnDebugInit();
                CyU3PDebugPrint(4, "Enter standby returned %d\r\n", stat);
                CyFxAppErrorHandler(stat);
            }

            CyFxAppErrorHandler(1);
        } else {
            // Handle USB event logging
            tmp1 = CyU3PUsbGetEventLogIndex();
            if (tmp1 != prevUsbLogIndex) {
                tmp2 = prevUsbLogIndex;
                while (tmp2 != tmp1) {
                    CyU3PDebugPrint(4, "USB LOG: %x\r\n", g_appContext.usbLogBuffer[tmp2]);
                    tmp2++;
                    if (tmp2 == USB_LOG_BUFFER_SIZE)
                        tmp2 = 0;
                }
            }
            prevUsbLogIndex = tmp1;
        }
    }
}

// Application define function which creates the threads
void CyFxApplicationDefine()
{
    void *ptr = NULL;
    uint32_t ret = CY_U3P_SUCCESS;

    // Initialize the application context
    memset(&g_appContext, 0, sizeof(AppContext_t));

    // Create an event flag group for the application thread
    ret = CyU3PEventCreate(&g_appContext.bulkLpEvent);
    if (ret != 0) {
        while (1);  // Loop indefinitely on error
    }

    // Allocate memory for the thread stack
    ptr = CyU3PMemAlloc(BULK_APP_THREAD_STACK);

    // Create the application thread
    ret = CyU3PThreadCreate(&g_appContext.bulkSrcSinkAppThread,
                            BULK_APP_THREAD_NAME,
                            BulkSrcSinkAppThread_Entry,
                            0,
                            ptr,
                            BULK_APP_THREAD_STACK,
                            BULK_APP_THREAD_PRIORITY,
                            BULK_APP_THREAD_PRIORITY,
                            CYU3P_NO_TIME_SLICE,
                            CYU3P_AUTO_START);

    if (ret != 0) {
        while(1);  // Loop indefinitely on error
    }
}

// Main function
int main()
{
    CyU3PIoMatrixConfig_t io_cfg = IO_MATRIX_CONFIG_DEFAULT;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    // Initialize the device
    CyU3PSysClockConfig_t clockConfig = SYS_CLOCK_CONFIG_DEFAULT;
    status = CyU3PDeviceInit(&clockConfig);
    if (status != CY_U3P_SUCCESS) {
        goto handle_fatal_error;
    }

    // Initialize the caches
    status = CyU3PDeviceCacheControl(CACHE_CONFIG_I_ENABLE, CACHE_CONFIG_D_ENABLE, CACHE_CONFIG_DMA_ENABLE);
    if (status != CY_U3P_SUCCESS) {
        goto handle_fatal_error;
    }

    // Configure the IO matrix for the device
    status = CyU3PDeviceConfigureIOMatrix(&io_cfg);
    if (status != CY_U3P_SUCCESS) {
        goto handle_fatal_error;
    }

    // Initialize the RTOS kernel
    CyU3PKernelEntry();

    return 0;

handle_fatal_error:
    while (1);  // Cannot recover from this error
}
