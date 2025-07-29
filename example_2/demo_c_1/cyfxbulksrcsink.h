#ifndef _INCLUDED_CYFXBULKSRCSINK_H_
#define _INCLUDED_CYFXBULKSRCSINK_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3externcstart.h"

#include "usb_descriptors.h"
#include "system_config.h"

// Endpoint and socket definitions for the bulk source sink application

// To change the producer and consumer EP enter the appropriate EP numbers for the #defines.
// In the case of IN endpoints enter EP number along with the direction bit.
// For eg. EP 6 IN endpoint is 0x86
//     and EP 6 OUT endpoint is 0x06.
// To change sockets mention the appropriate socket number in the #defines.

// Note: For USB 2.0 the endpoints and corresponding sockets are one-to-one mapped
//         i.e. EP 1 is mapped to UIB socket 1 and EP 2 to socket 2 so on


#define CY_FX_BULKSRCSINK_DMA_TX_SIZE        (DMA_TRANSFER_SIZE_INFINITE)    // DMA transfer size is set to infinite

#include <cyu3externcend.h>

#endif // _INCLUDED_CYFXBULKSRCSINK_H_
