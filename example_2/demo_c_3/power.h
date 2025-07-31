#ifndef POWER_H
#define POWER_H

#include "cyu3types.h"
#include "cyu3usb.h"
#include "cyu3os.h"
#include "system_config.h"

// Power management context structure
typedef struct PowerContext_t {
    CyBool_t standbyModeEnable;     // Whether standby mode entry is enabled
    CyBool_t triggerStandbyMode;    // Request to initiate standby entry
    CyBool_t forceLinkU2;           // Whether the device should try to initiate U2 mode
    CyU3PTimer lpmTimer;            // LPM timer instance
} PowerContext_t;

// Power module API functions
CyU3PReturnStatus_t Power_Init(PowerContext_t *ctx);
void Power_Deinit(PowerContext_t *ctx);

// State management functions
void Power_EnableStandbyMode(PowerContext_t *ctx);
void Power_TriggerStandby(PowerContext_t *ctx);
void Power_ClearTriggerStandby(PowerContext_t *ctx);
void Power_SetForceLinkU2(PowerContext_t *ctx, CyBool_t force);

// State query functions
CyBool_t Power_IsStandbyModeEnabled(const PowerContext_t *ctx);
CyBool_t Power_ShouldTriggerStandby(const PowerContext_t *ctx);
CyBool_t Power_ShouldForceLinkU2(const PowerContext_t *ctx);

// Timer callback (needs to access context via static pointer due to SDK limitations)
void Power_SetCallbackContext(PowerContext_t *ctx);

// SDK callback functions
CyBool_t CyFxApplnLPMRqtCB(CyU3PUsbLinkPowerMode link_mode);
void TimerCb();
CyBool_t CyFxBulkSrcSinkApplntCB(CyU3PUsbLinkPowerMode link_mode);

#endif // POWER_H