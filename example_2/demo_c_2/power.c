#include "cyu3usb.h"
#include "cyu3error.h"

#include "power.h"


// Static context pointer for SDK callbacks that don't support user data
static PowerContext_t *g_powerCallbackContext = NULL;

CyU3PReturnStatus_t Power_Init(PowerContext_t *ctx)
{
    if (!ctx) {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    // Initialize all power-related state
    ctx->standbyModeEnable = CyFalse;
    ctx->triggerStandbyMode = CyFalse;
    ctx->forceLinkU2 = CyFalse;

    // Initialize the LPM timer
    CyU3PReturnStatus_t status = CyU3PTimerCreate(&ctx->lpmTimer, TimerCb, 0, 
                                                  LPM_TIMER_TIMEOUT, LPM_TIMER_TIMEOUT, 
                                                  CYU3P_NO_ACTIVATE);
    if (status != CY_U3P_SUCCESS) {
        return status;
    }

    return CY_U3P_SUCCESS;
}

void Power_Deinit(PowerContext_t *ctx)
{
    if (!ctx) {
        return;
    }

    // Stop and destroy the timer
    CyU3PTimerStop(&ctx->lpmTimer);
    CyU3PTimerDestroy(&ctx->lpmTimer);

    // Clear context if it was the callback context
    if (g_powerCallbackContext == ctx) {
        g_powerCallbackContext = NULL;
    }
}

void Power_EnableStandbyMode(PowerContext_t *ctx)
{
    if (ctx) {
        ctx->standbyModeEnable = CyTrue;
    }
}

void Power_TriggerStandby(PowerContext_t *ctx)
{
    if (ctx) {
        ctx->triggerStandbyMode = CyTrue;
    }
}

void Power_ClearTriggerStandby(PowerContext_t *ctx)
{
    if (ctx) {
        ctx->triggerStandbyMode = CyFalse;
    }
}

void Power_SetForceLinkU2(PowerContext_t *ctx, CyBool_t force)
{
    if (ctx) {
        ctx->forceLinkU2 = force;
    }
}

CyBool_t Power_IsStandbyModeEnabled(const PowerContext_t *ctx)
{
    return ctx ? ctx->standbyModeEnable : CyFalse;
}

CyBool_t Power_ShouldTriggerStandby(const PowerContext_t *ctx)
{
    return ctx ? ctx->triggerStandbyMode : CyFalse;
}

CyBool_t Power_ShouldForceLinkU2(const PowerContext_t *ctx)
{
    return ctx ? ctx->forceLinkU2 : CyFalse;
}

void Power_SetCallbackContext(PowerContext_t *ctx)
{
    g_powerCallbackContext = ctx;
}

// SDK callback functions using the static context
CyBool_t CyFxApplnLPMRqtCB(CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}

void TimerCb(void)
{
    // Enable the low power mode transition on timer expiry
    CyU3PUsbLPMEnable();
}

CyBool_t CyFxBulkSrcSinkApplntCB(CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}