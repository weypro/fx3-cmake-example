#ifndef _ERROR_CODES_H_
#define _ERROR_CODES_H_

#include "cyu3error.h"

// Unified error codes for FX3 firmware
typedef enum {
    FX3_SUCCESS = 0,                    // Operation completed successfully
    FX3_ERROR_INIT_FAILED = -1,         // Initialization failed
    FX3_ERROR_INVALID_PARAM = -2,       // Invalid parameter provided
    FX3_ERROR_NOT_READY = -3,           // System not ready for operation
    FX3_ERROR_TIMEOUT = -4,             // Operation timed out
    FX3_ERROR_RESOURCE = -5,            // Resource allocation failed
    FX3_ERROR_USB_FAILED = -6,          // USB operation failed
    FX3_ERROR_DMA_FAILED = -7,          // DMA operation failed
    FX3_ERROR_ENDPOINT_FAILED = -8,     // Endpoint configuration failed
    FX3_ERROR_CALLBACK_FAILED = -9,     // Callback registration failed
    FX3_ERROR_BUFFER_FAILED = -10,      // Buffer operation failed
    FX3_ERROR_STATE_INVALID = -11,      // Invalid state for operation
    FX3_ERROR_NOT_SUPPORTED = -12       // Operation not supported
} fx3_result_t;

// Error handling utility macros
#define FX3_CHECK_RESULT(expr) do { \
    fx3_result_t _ret = (expr); \
    if (_ret != FX3_SUCCESS) return _ret; \
} while(0)

#define FX3_CHECK_RESULT_GOTO(expr, label) do { \
    fx3_result_t _ret = (expr); \
    if (_ret != FX3_SUCCESS) { \
        result = _ret; \
        goto label; \
    } \
} while(0)

// Convert CyU3P return status to FX3 result
#define FX3_FROM_CYU3P(cy_status) \
    ((cy_status) == CY_U3P_SUCCESS ? FX3_SUCCESS : FX3_ERROR_INIT_FAILED)

// Check CyU3P status and return on error
#define FX3_CHECK_CYU3P(expr) do { \
    CyU3PReturnStatus_t _cy_ret = (expr); \
    if (_cy_ret != CY_U3P_SUCCESS) return FX3_FROM_CYU3P(_cy_ret); \
} while(0)

#endif // _ERROR_CODES_H_