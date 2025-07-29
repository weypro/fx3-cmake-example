#ifndef APP_H
#define APP_H

#include "cyu3system.h"
#include "cyu3os.h"

#include "power.h"
#include "dma.h"
#include "usb_ctrl.h"

// Main application context structure
typedef struct AppContext_t {
    // Module contexts
    PowerContext_t power;
    DmaContext_t dma;
    UsbCtrlContext_t usbCtrl;

    // Application-level state
    CyBool_t isApplnActive;
    uint8_t *usbLogBuffer;
    CyU3PEvent bulkLpEvent;

    // Thread handle
    CyU3PThread bulkSrcSinkAppThread;
} AppContext_t;


#endif //APP_H
