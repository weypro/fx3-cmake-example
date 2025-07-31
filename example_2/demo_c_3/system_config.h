#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "cyu3usbconst.h"

// USB endpoint configuration
#define USB_EP_PRODUCER         0x01    // EP 1 OUT (host to device)
#define USB_EP_CONSUMER         0x81    // EP 1 IN (device to host)
#define USB_EP_DATA             0x82    // EP 2 IN (device to host) - Data channel
#define USB_EP_EVENT            0x83    // EP 3 IN (device to host) - Event channel
#define USB_EP_PRODUCER_SOCKET  CY_U3P_UIB_SOCKET_PROD_1
#define USB_EP_CONSUMER_SOCKET  CY_U3P_UIB_SOCKET_CONS_1
#define USB_EP_DATA_SOCKET      CY_U3P_UIB_SOCKET_CONS_2
#define USB_EP_EVENT_SOCKET     CY_U3P_UIB_SOCKET_CONS_3

// DMA configuration - optimized for different memory configurations
#ifdef CYMEM_256K
    // Configuration for CYUSB3011/CYUSB3012 with limited DMA buffer space
    #define DMA_BURST_LENGTH        4   // Burst length in 1KB packets (USB 3.0 only)
    #define DMA_SIZE_MULTIPLIER     1   // Buffer size multiplier for performance
    #define DMA_BUFFER_COUNT        2   // Number of DMA buffers
#else
    // Configuration for CYUSB3014 with more DMA buffer space
    #define DMA_BURST_LENGTH        16  // Burst length in 1KB packets (USB 3.0 only)
    #define DMA_SIZE_MULTIPLIER     2   // Buffer size multiplier for performance
    #define DMA_BUFFER_COUNT        3   // Number of DMA buffers
#endif

// DMA transfer configuration
#define DMA_TRANSFER_SIZE_INFINITE  0   // Infinite DMA transfer size
#define DMA_NO_WAIT_TIMEOUT         0   // No wait timeout for DMA operations

// Application thread configuration
#define BULK_APP_THREAD_STACK       0x1000  // 4KB stack size
#define BULK_APP_THREAD_PRIORITY    8       // Thread priority
#define BULK_APP_THREAD_NAME        "21:Bulk_src_sink"

// Data pattern for source endpoint
#define BULK_DATA_PATTERN           0xAA    // Pattern byte sent to host

// GPIO configuration
#define FX3_GPIO_TEST_OUT           50      // GPIO pin for testing
#define FX3_GPIO_TO_LOFLAG(gpio)    (1 << (gpio))
#define FX3_GPIO_TO_HIFLAG(gpio)    (1 << ((gpio) - 32))

// USB configuration
#define USB_VENDOR_ID               0x04B4  // Cypress vendor ID
#define USB_PRODUCT_ID              0x00F1  // Bulk source sink product ID
#define USB_DEVICE_VERSION          0x0000  // Device version

// Debug and logging configuration
#define DEBUG_UART_BAUDRATE         CY_U3P_UART_BAUDRATE_115200
#define USB_LOG_BUFFER_SIZE         0x1000  // 4KB USB event log buffer

// Power management configuration
#define LPM_TIMER_TIMEOUT           100     // LPM timer timeout in ms
#define STANDBY_VBUS_SETTLE_TIME    1000    // VBus settle time in ms

// System clock configuration
#define SYS_CLOCK_CONFIG_DEFAULT {  \
    .setSysClk400  = CyFalse,       \
    .cpuClkDiv     = 2,             \
    .dmaClkDiv     = 2,             \
    .mmioClkDiv    = 2,             \
    .useStandbyClk = CyFalse,       \
    .clkSrc        = CY_U3P_SYS_CLK \
}

// GPIO clock configuration
#define GPIO_CLOCK_CONFIG_DEFAULT { \
    .fastClkDiv = 2,                \
    .slowClkDiv = 32,               \
    .simpleDiv  = CY_U3P_GPIO_SIMPLE_DIV_BY_16, \
    .clkSrc     = CY_U3P_SYS_CLK_BY_2, \
    .halfDiv    = 0                 \
}

// UART configuration for debug
#define UART_CONFIG_DEFAULT {       \
    .baudRate = DEBUG_UART_BAUDRATE, \
    .stopBit  = CY_U3P_UART_ONE_STOP_BIT, \
    .parity   = CY_U3P_UART_NO_PARITY, \
    .txEnable = CyTrue,             \
    .rxEnable = CyFalse,            \
    .flowCtrl = CyFalse,            \
    .isDma    = CyTrue              \
}

// IO matrix configuration for FX3 DVK board
#define IO_MATRIX_CONFIG_DEFAULT {  \
    .isDQ32Bit = CyFalse,           \
    .s0Mode = CY_U3P_SPORT_INACTIVE, \
    .s1Mode = CY_U3P_SPORT_INACTIVE, \
    .useUart   = CyTrue,            \
    .useI2C    = CyFalse,           \
    .useI2S    = CyFalse,           \
    .useSpi    = CyFalse,           \
    .lppMode   = CY_U3P_IO_MATRIX_LPP_UART_ONLY, \
    .gpioSimpleEn[0]  = 0,          \
    .gpioSimpleEn[1]  = FX3_GPIO_TO_HIFLAG(FX3_GPIO_TEST_OUT), \
    .gpioComplexEn[0] = 0,          \
    .gpioComplexEn[1] = 0           \
}

// Cache configuration - D-Cache disabled for performance
#define CACHE_CONFIG_I_ENABLE       CyTrue   // Enable instruction cache
#define CACHE_CONFIG_D_ENABLE       CyFalse  // Disable data cache for DMA performance
#define CACHE_CONFIG_DMA_ENABLE     CyFalse  // Disable DMA cache coherency

// Event wait timeout configuration
#define APP_EVENT_WAIT_TIMEOUT      10      // Application event wait timeout in ms

// Vendor command configuration
#define VENDOR_CMD_MAX_DATA_SIZE    32      // Maximum data size for vendor commands
#define VENDOR_CMD_BUFFER_ALIGN     32      // Buffer alignment for vendor commands

#endif // SYSTEM_CONFIG_H