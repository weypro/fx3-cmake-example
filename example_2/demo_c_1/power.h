#ifndef _POWER_H_
#define _POWER_H_

#include "cyu3types.h"
#include "cyu3usb.h"
#include "cyu3os.h"

// Global variables
extern CyBool_t StandbyModeEnable;   // Whether standby mode entry is enabled.
extern CyBool_t TriggerStandbyMode;  // Request to initiate standby entry.
extern CyBool_t glForceLinkU2;       // Whether the device should try to initiate U2 mode.
extern CyU3PTimer glLpmTimer;        // Timer Instance

// Function declarations
CyBool_t CyFxApplnLPMRqtCB(CyU3PUsbLinkPowerMode link_mode);
void TimerCb(void);
CyBool_t CyFxBulkSrcSinkApplntCB(CyU3PUsbLinkPowerMode link_mode);

#endif // _POWER_H_