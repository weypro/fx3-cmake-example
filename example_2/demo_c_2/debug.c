#include "debug.h"
#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3error.h"
#include "cyu3uart.h"
#include "cyu3gpio.h"
#include "cyu3utils.h"
#include "system_config.h"

// Application Error Handler
void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus    // API return status
)
{
    // Application failed with the error code apiRetStatus

    // Add custom debug or recovery actions here

    // Loop Indefinitely
    for (;;)
    {
        // Thread sleep : 100 ms
        CyU3PThreadSleep (100);
    }
}

// This function initializes the debug module. The debug prints
// are routed to the UART and can be seen using a UART console
// running at 115200 baud rate.
void
CyFxBulkSrcSinkApplnDebugInit (void)
{
    CyU3PGpioClock_t  gpioClock = GPIO_CLOCK_CONFIG_DEFAULT;
    CyU3PUartConfig_t uartConfig = UART_CONFIG_DEFAULT;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    // Initialize the GPIO block. If we are transitioning from the boot app, we can verify whether the GPIO
    // state is retained.
    apiRetStatus = CyU3PGpioInit (&gpioClock, NULL);

    // When FX3 is restarting from standby mode, the GPIO block would already be ON and need not be started
    // again.
    if ((apiRetStatus != 0) && (apiRetStatus != CY_U3P_ERROR_ALREADY_STARTED))
    {
        CyFxAppErrorHandler(apiRetStatus);
    }
    else
    {
        // Set the test GPIO as an output and update the value to 0.
        CyU3PGpioSimpleConfig_t testConf = {CyFalse, CyTrue, CyTrue, CyFalse, CY_U3P_GPIO_NO_INTR};

        apiRetStatus = CyU3PGpioSetSimpleConfig (FX3_GPIO_TEST_OUT, &testConf);
        if (apiRetStatus != 0)
            CyFxAppErrorHandler (apiRetStatus);
    }

    // Initialize the UART for printing debug messages
    apiRetStatus = CyU3PUartInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        // Error handling
        CyFxAppErrorHandler(apiRetStatus);
    }

    // Set UART configuration
    apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    // Set the UART transfer to a really large value.
    apiRetStatus = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    // Initialize the debug module.
    apiRetStatus = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    CyU3PDebugPreamble(CyFalse);
}