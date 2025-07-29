#include "power.h"
#include "cyu3usb.h"

CyBool_t StandbyModeEnable  = CyFalse;   // Whether standby mode entry is enabled.
CyBool_t TriggerStandbyMode = CyFalse;   // Request to initiate standby entry.
CyBool_t glForceLinkU2      = CyFalse;   // Whether the device should try to initiate U2 mode.

// Timer Instance
CyU3PTimer glLpmTimer;

CyBool_t
CyFxApplnLPMRqtCB (
        CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}

// Callback funtion for the timer expiry notification.
void TimerCb(void)
{
    // Enable the low power mode transition on timer expiry
    CyU3PUsbLPMEnable();
}

CyBool_t
CyFxBulkSrcSinkApplntCB (
        CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}